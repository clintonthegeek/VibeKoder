#include "session.h"
#include "project.h"
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QDateTime>
#include <QDir>
#include <QRegularExpression>
#include <QDebug>


// Helper to copy files to the cache folder with overwrite
static bool copyFileToCacheFolder(const QString &srcPath, const QString &cacheFolder, const QString &relPath)
{
    QFileInfo srcInfo(srcPath);
    if (!srcInfo.exists()) {
        qWarning() << "Source file does not exist:" << srcPath;
        return false;
    }

    QString destPath = QDir(cacheFolder).filePath(relPath);
    QFileInfo destInfo(destPath);
    QDir destDir = destInfo.dir();
    if (!destDir.exists()) {
        if (!destDir.mkpath(".")) {
            qWarning() << "Failed to create cache folder:" << destDir.absolutePath();
            return false;
        }
    }
    if (QFile::exists(destPath)) {
        if (!QFile::remove(destPath)) {
            qWarning() << "Cannot remove existing cached file:" << destPath;
            return false;
        }
    }
    if (!QFile::copy(srcPath, destPath)) {
        qWarning() << "Failed copying source file to cache:" << srcPath << "->" << destPath;
        return false;
    }
    qDebug() << "Cached file:" << srcPath << "->" << destPath;
    return true;
}


Session::Session(Project *project)
    : m_project(project)
{
}

bool Session::load(const QString &filepath)
{
    QFile file(filepath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Failed to open session file:" << filepath;
        return false;
    }

    m_filepath = filepath;

    QTextStream in(&file);
    QString data = in.readAll();

    m_slices.clear();

    return parseSessionMarkdown(data);
}

bool Session::refreshCacheAndSave()
{
    if (m_slices.isEmpty()) {
        qWarning() << "No prompt slices to refresh cache for.";
        return false;
    }

    QVector<PromptSlice> updatedSlices;
    for (auto &slice : m_slices) {
        // caching includes rewrites include->cached and copies files
        QString cachedContent = cacheIncludesInContent(slice.content);
        updatedSlices.append({slice.role, cachedContent, slice.timestamp});
    }
    m_slices = updatedSlices;

    if (!save()) {
        qWarning() << "Failed to save session file during cache refresh.";
        return false;
    }
    qDebug() << "Session cache refreshed and saved successfully.";
    return true;
}

bool Session::save(const QString &filepath)
{
    QString savePath = filepath.isEmpty() ? m_filepath : filepath;

    QFile file(savePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Failed to write session file:" << savePath;
        return false;
    }

    QTextStream out(&file);

    out << compiledRawMarkdown();

    file.flush();

    return true;
}

QString Session::sessionFolder() const
{
    QFileInfo fi(m_filepath);
    return fi.absolutePath();
}

QString Session::sessionCacheBaseFolder() const
{
    QFileInfo fi(m_filepath);
    QString baseName = fi.completeBaseName(); // e.g., "001"
    QDir sessionsDir = fi.dir();
    QString cacheFolderPath = sessionsDir.filePath(baseName);
    if (!QDir(cacheFolderPath).exists()) {
        QDir().mkpath(cacheFolderPath);
    }
    return cacheFolderPath;
}

QString Session::sessionDocCacheFolder() const
{
    QString docCache = QDir(sessionCacheBaseFolder()).filePath("docs");
    if (!QDir(docCache).exists()) {
        QDir().mkpath(docCache);
    }
    return docCache;
}

QString Session::sessionSrcCacheFolder() const
{
    QString srcCache = QDir(sessionCacheBaseFolder()).filePath("src");
    if (!QDir(srcCache).exists()) {
        QDir().mkpath(srcCache);
    }
    return srcCache;
}

QString Session::cacheIncludesInContent(const QString &content)
{
    QString result = content;
    if (!m_project) {
        qWarning() << "[cacheIncludesInContent] No project; skipping include caching step";
        return result;
    }

    QString projectRoot = m_project->rootFolder();
    QString docCacheRoot = sessionDocCacheFolder();
    QString srcCacheRoot = sessionSrcCacheFolder();

    static const QRegularExpression markerRe(R"(<!--\s*(include):\s*(.*?)\s*-->)", QRegularExpression::CaseInsensitiveOption);

    int offset = 0;
    while (true) {
        QRegularExpressionMatch m = markerRe.match(result, offset);
        if (!m.hasMatch())
            break;

        // We only match "include", never "cached" here
        QString fullMatch = m.captured(0);
        QString includePath = m.captured(2).trimmed();

        qDebug() << "[cacheIncludesInContent] Found include marker:" << fullMatch
                 << "| include path:" << includePath;

        // Resolve absolute source file path
        QString absSrcFile;
        QFileInfo fi(includePath);
        if (fi.isAbsolute()) {
            absSrcFile = includePath;
        } else {
            if (includePath.contains('/') || includePath.contains('\\')) {
                absSrcFile = QDir(projectRoot).filePath(includePath);
            } else {
                const QString suffix = QFileInfo(includePath).suffix().toLower();
                if (suffix == "h" || suffix == "cpp" || suffix == "hpp")
                    absSrcFile = QDir(m_project->srcFolder()).filePath(includePath);
                else
                    absSrcFile = QDir(m_project->docsFolder()).filePath(includePath);
            }
        }

        QFileInfo absFi(absSrcFile);
        if (!absFi.exists() || !absFi.isFile()) {
            qWarning() << "[cacheIncludesInContent] Source file missing:" << absSrcFile;
            // Do not replace the marker — move forward to avoid infinite loop
            offset = m.capturedEnd(0);
            continue;
        }

        // Compute relative path inside cache folder (remove leading docs/src folder names)
        QString relPath = includePath;
        const QString docsFolderName = QFileInfo(m_project->docsFolder()).fileName();
        const QString srcFolderName = QFileInfo(m_project->srcFolder()).fileName();

        qDebug() << "[cacheIncludesInContent] includePath =" << includePath;
        qDebug() << "[cacheIncludesInContent] docsFolderName =" << docsFolderName;
        qDebug() << "[cacheIncludesInContent] srcFolderName =" << srcFolderName;

        if (relPath.startsWith(docsFolderName + "/") || relPath.startsWith(docsFolderName + "\\"))
            relPath = relPath.mid(docsFolderName.length() + 1);
        else if (relPath.startsWith(srcFolderName + "/") || relPath.startsWith(srcFolderName + "\\"))
            relPath = relPath.mid(srcFolderName.length() + 1);

        qDebug() << "[cacheIncludesInContent] relPath after prefix removal =" << relPath;

        // Determine cache base folder (docs cache or src cache)
        QString cacheBaseFolder;
        const QString ext = QFileInfo(relPath).suffix().toLower();
        if (ext == "h" || ext == "cpp" || ext == "hpp")
            cacheBaseFolder = srcCacheRoot;
        else
            cacheBaseFolder = docCacheRoot;

        // Copy file to cache folder (overwrite)
        if (!copyFileToCacheFolder(absSrcFile, cacheBaseFolder, relPath)) {
            qWarning() << "[cacheIncludesInContent] Failed to cache file:" << absSrcFile;
            offset = m.capturedEnd(0);
            continue;
        }

        // Compute relative path from session folder to cached file for replacement
        QString cachedAbsolutePath = QDir(cacheBaseFolder).filePath(relPath);
        QString relativeCachedPath = QDir(cacheBaseFolder).relativeFilePath(cachedAbsolutePath);
        if (relativeCachedPath.startsWith("./"))
            relativeCachedPath = relativeCachedPath.mid(2);

        // Replacement marker: cached include
        qDebug() << "[cacheIncludesInContent] relativeCachedPath =" << relativeCachedPath;
        QString replacement = QString("<!-- cached: %1 -->").arg(relativeCachedPath);
        qDebug() << "[cacheIncludesInContent] replacement string =" << replacement;

        // Replace include marker in content
        int start = m.capturedStart(0);
        int length = m.capturedLength(0);
        result.replace(start, length, replacement);

        // Move offset forward past replacement to continue searching
        offset = start + replacement.length();
    }

    return result;
}

QString Session::promptSliceContent(int index) const
{
    if (index < 0 || index >= m_slices.size())
        return QString();
    return m_slices[index].content;
}

void Session::setPromptSliceContent(int index, const QString &content)
{
    if (index < 0 || index >= m_slices.size())
        return;
    m_slices[index].content = content;
}

QString Session::compilePrompt()
{
    QStringList parts;
    QSet<QString> visitedFiles;

    for (const auto &slice : m_slices) {
        // Expand only cached includes; skip expanding original includes (prevent recursion)
        //QString expanded = expandIncludesRecursive(slice.content, visitedFiles, 0, true, /*expandIncludeMarkers=*/false);
        QString expanded = expandIncludesOnce(slice.content);

        QString intro;
        switch (slice.role) {
        case MessageRole::User:      intro = "[User Prompt]\n"; break;
        case MessageRole::Assistant: intro = "[Assistant Response]\n"; break;
        case MessageRole::System:    intro = "[System Message]\n"; break;
        }

        parts << intro + expanded;
    }

    return parts.join("\n\n---\n\n");
}

QString Session::expandIncludesRecursive(const QString &content,
                                         QSet<QString> &visitedFiles,
                                         int parentHeaderLevel,
                                         bool promoteHeaders,
                                         bool expandIncludeMarkers /*= true*/)
{
    // Regex to find fenced code blocks: ``` or ~~~ with optional lang
    static const QRegularExpression fencedBlockRe(
        R"((^|\n)([`~]{3,})[ \t]*[a-zA-Z0-9_.+-]*[ \t]*\n.*?\n\2[ \t]*(?=\n|$))",
        QRegularExpression::DotMatchesEverythingOption | QRegularExpression::MultilineOption);

    // Find all fenced code blocks and save their start/end positions
    struct Range { int start, end; };
    QVector<Range> codeBlockRanges;

    auto it = fencedBlockRe.globalMatch(content);
    while (it.hasNext()) {
        auto match = it.next();
        int startPos = match.capturedStart(0);
        int endPos = match.capturedEnd(0);
        codeBlockRanges.append({startPos, endPos});
    }

    // Helper lambda to check if pos inside any code block
    auto insideCodeBlock = [&](int pos) {
        for (const auto& r : codeBlockRanges) {
            if (pos >= r.start && pos < r.end)
                return true;
        }
        return false;
    };

    // Regex for include/cached markers: <!-- include: path --> or <!-- cached: path -->
    static const QRegularExpression markerRe(R"(<!--\s*(include|cached):\s*(.*?)\s*-->)", QRegularExpression::CaseInsensitiveOption);

    QString result = content;
    int offset = 0;

    while (true) {
        QRegularExpressionMatch m = markerRe.match(result, offset);
        if (!m.hasMatch())
            break;

        int markerPos = m.capturedStart(0);
        if (insideCodeBlock(markerPos)) {
            // Skip expanding marker inside code block by moving offset past it
            offset = m.capturedEnd(0);
            continue;
        }

        QString markerType = m.captured(1).toLower();
        QString includePath = m.captured(2).trimmed();

        qDebug() << "[expandIncludesRecursive] markerType:" << markerType << ", includePath:" << includePath;


        // Avoid infinite recursion by skipping if already visited this exact file path
        QString absPath;

        if (!m_project) {
            qWarning() << "[expandIncludesRecursive] No project set, returning content as is.";
            return result;
        }

        QString rootFolder = m_project->rootFolder();
        if (rootFolder.isEmpty())
            rootFolder = QDir::currentPath();

        if (markerType == "include") {
            if (!expandIncludeMarkers) {
                // Skip expanding original include markers if flag is false
                offset = m.capturedEnd(0);
                continue;
            }
            absPath = QDir(rootFolder).filePath(includePath);
        } else if (markerType == "cached") {
            QString cacheBaseFolder;
            const QString ext = QFileInfo(includePath).suffix().toLower();
            if (ext == "h" || ext == "cpp" || ext == "hpp") {
                cacheBaseFolder = sessionSrcCacheFolder();
            } else {
                cacheBaseFolder = sessionDocCacheFolder();
            }
            absPath = QDir(cacheBaseFolder).filePath(includePath);
        } else {
            // Unknown marker, skip
            offset = m.capturedEnd(0);
            continue;
        }

        if (visitedFiles.contains(absPath)) {
            qWarning() << "[expandIncludesRecursive] Circular include detected:" << absPath;
            offset = m.capturedEnd(0);
            continue;
        }

        visitedFiles.insert(absPath);

        QString includedContent;
        QFile incFile(absPath);
        if (incFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            includedContent = incFile.readAll();
            incFile.close();
        } else {
            includedContent = QString("[Could not read include file: %1]").arg(absPath);
            qWarning() << "[expandIncludesRecursive] Could not open include file:" << absPath;
        }

        // Determine enclosing header level before include marker in result
        int includePos = m.capturedStart(0);
        int enclosingLevel = findEnclosingHeaderLevel(result, includePos);
        if (enclosingLevel == 0)
            enclosingLevel = parentHeaderLevel;

        // Recursively expand includes inside includedContent
        // For "include" markers, pass expandIncludeMarkers flag as true recursively
        // For cached, always expand includes inside recursively (expandIncludeMarkers = true)
        bool recursiveExpandIncludeMark = (markerType == "include") ? expandIncludeMarkers : true;
        QString recExpanded = expandIncludesRecursive(includedContent, visitedFiles, enclosingLevel + 1, false, recursiveExpandIncludeMark);

        // Promote headers if requested
        if (promoteHeaders)
            recExpanded = promoteMarkdownHeaders(recExpanded, enclosingLevel + 1);

        // Replace marker in result with expanded content
        int start = m.capturedStart(0);
        int length = m.capturedLength(0);
        result.replace(start, length, recExpanded);

        // Move offset past replaced content
        offset = start + recExpanded.length();

        visitedFiles.remove(absPath);

        // Update code block ranges since result changed:
        // (optional, for complex edits this is better but can be skipped if replacements don't contain fences)
        // For simplicity, you can break and re-call function if needed.
    }

    // Finally, if promoteHeaders requested, promote headers at top level too:
    if (promoteHeaders)
        return promoteMarkdownHeaders(result, parentHeaderLevel);
    else
        return result;
}

QString Session::expandIncludesOnce(const QString &content)
{
    static const QRegularExpression cachedMarkerRe(R"(<!--\s*cached:\s*(.*?)\s*-->)", QRegularExpression::CaseInsensitiveOption);

    QString result = content;
    int offset = 0;

    while (true) {
        QRegularExpressionMatch m = cachedMarkerRe.match(result, offset);
        if (!m.hasMatch())
            break;

        QString includePath = m.captured(1).trimmed();
        // Resolve absolute path to cached included file (docs or src cache folder)
        QString absPath;
        const QString ext = QFileInfo(includePath).suffix().toLower();
        if (ext == "h" || ext == "cpp" || ext == "hpp") {
            absPath = QDir(sessionSrcCacheFolder()).filePath(includePath);
        } else {
            absPath = QDir(sessionDocCacheFolder()).filePath(includePath);
        }

        QString includedContent;
        QFile incFile(absPath);
        if (incFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            includedContent = incFile.readAll();
            incFile.close();
        } else {
            includedContent = QString("[Could not read cached include file: %1]").arg(absPath);
            qWarning() << "[expandIncludesOnce] Could not open cached include file:" << absPath;
        }

        int start = m.capturedStart(0);
        int length = m.capturedLength(0);
        result.replace(start, length, includedContent);
        offset = start + includedContent.length();
    }

    return result;
}

QVector<PromptSlice> Session::slices() const
{
    return m_slices;
}

void Session::appendUserSlice(const QString &markdownContent)
{
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    m_slices.append({MessageRole::User, markdownContent, timestamp});
}

void Session::appendAssistantSlice(const QString &markdownContent)
{
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    m_slices.append({MessageRole::Assistant, markdownContent, timestamp});
}

void Session::appendSystemSlice(const QString &markdownContent)
{
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    m_slices.append({MessageRole::System, markdownContent, timestamp});
}

bool Session::parseSessionMarkdown(const QString &data)
{
    // Expect pattern:
    // ### Role (optional timestamp)
    // ```
    // markdown fenced block follows
    // ```
    // Repeated.

    static QRegularExpression headerRe(R"(#\s*(User|Assistant|System)(?:\s*\((.*?)\))?\s*)");
    static QRegularExpression fenceRe(R"(```markdown\s*([\s\S]*?)```)", QRegularExpression::MultilineOption);

    int pos = 0;
    int len = data.length();

    m_slices.clear();

    while (pos < len) {
        QRegularExpressionMatch headerMatch = headerRe.match(data, pos);
        if (!headerMatch.hasMatch()) {
            break;
        }

        QString roleStr = headerMatch.captured(1);
        QString timestamp = headerMatch.captured(2);

        MessageRole role = MessageRole::User;
        if (roleStr == "Assistant")
            role = MessageRole::Assistant;
        else if (roleStr == "System")
            role = MessageRole::System;

        int afterHeaderPos = headerMatch.capturedEnd(0);

        QRegularExpressionMatch fenceMatch = fenceRe.match(data, afterHeaderPos);

        if (!fenceMatch.hasMatch()) {
            qWarning() << "Failed to find fenced markdown block after header at" << afterHeaderPos;
            return false;
        }

        QString content = fenceMatch.captured(1);

        m_slices.append({role, content, timestamp});

        pos = fenceMatch.capturedEnd(0);
    }

    return true;
}

// Promote all markdown headers by a given level (caps at 6)
QString Session::promoteMarkdownHeaders(const QString &md, int promoteBy)
{
    // qDebug() << "promoteMarkdownHeaders called with promoteBy =" << promoteBy
    //          << "input length =" << md.length();
    if (promoteBy <= 0) return md;

    static const QRegularExpression headerRe(R"(^(\s{0,3})(#{1,6})(\s.*)$)", QRegularExpression::MultilineOption);

    QString promoted = md;
    int offset = 0;

    auto matchIt = headerRe.globalMatch(md);
    while (matchIt.hasNext()) {
        QRegularExpressionMatch match = matchIt.next();
        int start = match.capturedStart(2) + offset;
        int length = match.capturedLength(2);

        QString hashes = match.captured(2);
        int currentLevel = hashes.length();
        int newLevel = currentLevel + promoteBy;
        if (newLevel > 6) newLevel = 6;

        // qDebug() << "Promoting header from level" << currentLevel << "to level" << newLevel;

        QString newHashes = QString(newLevel, '#');

        promoted.replace(start, length, newHashes);

        offset += newHashes.length() - length;
    }

    return promoted;
}

// Find the heading level just before 'includePos' in 'text'
// Returns 0 if no heading found (treated as document root)
int Session::findEnclosingHeaderLevel(const QString &text, int includePos)
{
    static const QRegularExpression headerRe(R"(^\s{0,3}(#{1,6})\s+.+$)", QRegularExpression::MultilineOption);

    QString precedingText = text.left(includePos);

    QList<QRegularExpressionMatch> matches;

    auto it = headerRe.globalMatch(precedingText);
    while (it.hasNext())
        matches.append(it.next());

    if (matches.isEmpty())
        return 0; // No header found before include

    QRegularExpressionMatch lastHeader = matches.last();

    return lastHeader.captured(1).length();
}


void Session::setCommandPipeOutput(const QString &key, const QString &output)
{
    m_commandPipeOutputs[key] = output;
}

QString Session::commandPipeOutput(const QString &key) const
{
    return m_commandPipeOutputs.value(key, QString());
}



// Find maximum continuous backtick sequence in the entire text
static int maxBacktickRun(const QString &text)
{
    static QRegularExpression regex(R"(`+)");
    QRegularExpressionMatchIterator i = regex.globalMatch(text);

    int maxRun = 0;
    while (i.hasNext()) {
        auto match = i.next();
        int length = match.capturedLength();
        if (length > maxRun)
            maxRun = length;
    }
    return maxRun;
}

// Replace any inner fences of length >= minFenceLength with fences at least one longer.
// This avoids conflicts with our outer fence.
static QString sanitizeInnerFences(const QString &content, int minFenceLength)
{
    QString sanitized = content;

    // Regex to find fenced blocks: fences of backticks ```...```
    // We focus on fences with length >= minFenceLength - 1 to replace them.
    // This is a heuristic — complicated nested markdown is hard to parse perfectly.

    // Pattern matches fences with >= 3 backticks:
    static QRegularExpression fenceRe(R"((`{3,})(.*?)(`{3,}))", QRegularExpression::DotMatchesEverythingOption);

    int pos = 0;
    while (true) {
        QRegularExpressionMatch m = fenceRe.match(sanitized, pos);
        if (!m.hasMatch())
            break;

        QString fenceStart = m.captured(1);
        QString fenceEnd = m.captured(3);

        int fenceLen = fenceStart.length();

        if (fenceLen >= minFenceLength) {
            // Bump fence length by 1 to avoid conflict with outer fence
            int newFenceLen = fenceLen + 1;
            QString newFence = QString(newFenceLen, '`');

            // Replace start and end fences in the match with newFence of longer length
            int startPos = m.capturedStart(1);
            int endPos = m.capturedStart(3);

            sanitized.replace(endPos, fenceLen, newFence);
            sanitized.replace(startPos, fenceLen, newFence);

            // Move pos forward to avoid infinite loop on same match:
            pos = endPos + newFenceLen;
        } else {
            // Fence is shorter, safe to skip
            pos = m.capturedEnd(0);
        }
    }

    return sanitized;
}


QString Session::sanitizeFencesRecursive(const QString &markdown, int outerFenceLength)
{
    QString input = markdown;
    QString output;

    // Normalize line endings to \n for consistent matching
    input.replace("\r\n", "\n").replace("\r", "\n");

    // Regex to match fenced code blocks:
    //  - group 1: opening fence (backticks or tildes, length >= 3)
    //  - group 2: optional language id
    //  - group 3: content inside fenced block
    //  - followed by matching closing fence same as opening fence
    static const QRegularExpression fenceRe(
        R"(([`~]{3,})[ \t]*([a-zA-Z0-9_.+-]*)[ \t]*\n(.*?)(\n\1[ \t]*)+)",
        QRegularExpression::DotMatchesEverythingOption | QRegularExpression::MultilineOption);

    int lastMatchEnd = 0;

    QRegularExpressionMatchIterator it = fenceRe.globalMatch(input);

    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();

        int matchStart = match.capturedStart(0);
        int matchEnd = match.capturedEnd(0);

        // Append text before match as-is
        output += input.midRef(lastMatchEnd, matchStart - lastMatchEnd);

        QString fence = match.captured(1);
        int fenceLen = fence.length();
        QChar fenceChar = fence[0];
        QString lang = match.captured(2);
        QString innerContent = match.captured(3);

        //qDebug() << "Sanitize fences:" << "outer Fence Length:" << outerFenceLength
        //         << "found fence length:" << fenceLen
        //         << "lang:" << lang;

        // Recursively process inner content increasing outerFenceLength by 1
        QString sanitizedInner = sanitizeFencesRecursive(innerContent, outerFenceLength + 1);

        // Decide new fence length:
        int newFenceLen = outerFenceLength;
        if (newFenceLen < fenceLen) {
            newFenceLen = fenceLen;  // never shorten fences
        }

        QString newFence(newFenceLen, fenceChar);

        // Build replacement fenced block
        QString replacementBlock = QStringLiteral("%1%2\n%3\n%1\n").arg(newFence, lang, sanitizedInner);

        output += replacementBlock;

        lastMatchEnd = matchEnd;
    }

    // Append remaining content after the last match
    output += input.midRef(lastMatchEnd);

    // qDebug() << "Sanitize recursion complete for fence length" << outerFenceLength
    //          << "output length:" << output.length();

    return output;
}

QString Session::compiledRawMarkdown() const
{
    QString result;
    for (const auto &slice : m_slices) {
        QString roleStr;
        switch (slice.role) {
        case MessageRole::User: roleStr = "User"; break;
        case MessageRole::Assistant: roleStr = "Assistant"; break;
        case MessageRole::System: roleStr = "System"; break;
        }

        // Sanitize fences recursively as you already have implemented
        QString sanitizedContent = sanitizeFencesRecursive(slice.content);

        // Compose top-level heading for slice role + timestamp
        QString header = QString("# %1").arg(roleStr);
        if (!slice.timestamp.isEmpty()) {
            header += QString(" (%1)").arg(slice.timestamp);
        }

        // Use fixed 3-backtick fenced block (assuming sanitizeFencesRecursive ensures inner fences longer than 3)
        const QString outerFence = "```";

        result += QString("%1\n%2markdown\n%3\n%2\n\n")
                      .arg(header)
                      .arg(outerFence)
                      .arg(sanitizedContent.trimmed());
    }

    return result;
}

QString Session::sessionCacheFolder() const
{
    QFileInfo fi(m_filepath);
    QString baseName = fi.completeBaseName();  // e.g. "001"
    QString sessionsFolder = fi.dir().absolutePath(); // sessions folder path

    QString sessionCacheDir = QDir(sessionsFolder).filePath(baseName);

    if (!QDir(sessionCacheDir).exists()) {
        QDir().mkpath(sessionCacheDir);
    }

    return sessionCacheDir;
}







