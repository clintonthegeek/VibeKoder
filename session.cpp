#include "session.h"
#include "project.h"
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QDateTime>
#include <QDir>
#include <QRegularExpression>
#include <QDebug>



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

bool Session::save(const QString &filepath)
{
    QString savePath = filepath.isEmpty() ? m_filepath : filepath;

    QFile file(savePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Failed to write session file:" << savePath;
        return false;
    }

    QTextStream out(&file);

    // Serialize whole session with dynamic fences safely
    out << compiledRawMarkdown();

    file.flush();

    return true;
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

QString Session::compilePrompt()
{
    QStringList parts;
    QSet<QString> visitedFiles;

    for (const auto &slice : m_slices) {
        // Pass headingLevelOffset = 1 because prompt slices are now level 1 headers (# User etc).
        // Included files inside slices are promoted starting at level 2.
        QString expanded = expandIncludesRecursive(slice.content, visitedFiles, 0);

        QString intro;
        switch (slice.role) {
        case MessageRole::User: intro = "[User Prompt]\n"; break;
        case MessageRole::Assistant: intro = "[Assistant Response]\n"; break;
        case MessageRole::System: intro = "[System Message]\n"; break;
        }

        parts << intro + expanded;
    }

    return parts.join("\n\n---\n\n");
}

#include <QRegularExpression>
#include <QFile>
#include <QDir>
#include <QDebug>

// Promote all markdown headers by a given level (caps at 6)
QString Session::promoteMarkdownHeaders(const QString &md, int promoteBy)
{
    qDebug() << "promoteMarkdownHeaders called with promoteBy =" << promoteBy
             << "input length =" << md.length();
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

        qDebug() << "Promoting header from level" << currentLevel << "to level" << newLevel;

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

// Recursive include expansion with heading level context
QString Session::expandIncludesRecursive(const QString &content,
                                         QSet<QString> &visitedFiles,
                                         int parentHeaderLevel)
{
    static const QRegularExpression includeRe(R"(<!--\s*include:\s*(.*?)\s*-->)");

    qDebug() << "expandIncludesRecursive called with parentHeaderLevel =" << parentHeaderLevel
             << ", visitedFiles size =" << visitedFiles.size();

    QString result = content;

    if (!m_project) {
        qWarning() << "No project assigned to session; cannot resolve includes";
        // Promote current block headers based on parentHeaderLevel and return
        QString promotedResult = promoteMarkdownHeaders(result, parentHeaderLevel);
        qDebug() << "No project: after promotion headers length =" << promotedResult.length();
        return promotedResult;
    }

    QString rootFolder = m_project->rootFolder();
    if (rootFolder.isEmpty())
        rootFolder = QDir::currentPath();

    int offset = 0;
    while (true) {
        QRegularExpressionMatch m = includeRe.match(result, offset);
        if (!m.hasMatch())
            break;

        QString includePath = m.captured(1).trimmed();
        QString absPath = QDir(rootFolder).filePath(includePath);

        qDebug() << "Include found:" << includePath << "resolved to" << absPath;

        if (visitedFiles.contains(absPath)) {
            qWarning() << "Circular include detected:" << absPath;
            offset = m.capturedEnd(0);
            continue;
        }

        visitedFiles.insert(absPath);

        int includePos = m.capturedStart(0);

        // Find the header level enclosing the include marker within current content
        int enclosingLevel = findEnclosingHeaderLevel(result, includePos);
        if (enclosingLevel == 0) {
            enclosingLevel = parentHeaderLevel; // fallback to passed header level
        }

        qDebug() << "Enclosing header level at include position:" << enclosingLevel;

        QString includedContent;
        QFile incFile(absPath);
        if (incFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            includedContent = incFile.readAll();
            incFile.close();

            // Recursively expand includes inside the included content, increasing header level +1
            includedContent = expandIncludesRecursiveWithoutPromotion(includedContent, visitedFiles, enclosingLevel + 1);
            // Promote headers in the included content ONLY ONCE here accordingly
            includedContent = promoteMarkdownHeaders(includedContent, enclosingLevel + 1);

            qDebug() << "Included content after recursive expansion and promotion, length =" << includedContent.length();

        } else {
            includedContent = QString("[Could not include file: %1]").arg(absPath);
            qWarning() << "Failed to open include file:" << absPath;
        }

        int start = m.capturedStart(0);
        int length = m.capturedLength(0);

        // Replace include marker with expanded and promoted included content
        result.replace(start, length, includedContent);

        offset = start + includedContent.length();

        visitedFiles.remove(absPath);
        qDebug() << "Include replaced at position" << start << ", new offset =" << offset;
    }

    // Finally, promote headers in the current block only based on parentHeaderLevel
    QString finalResult = promoteMarkdownHeaders(result, parentHeaderLevel);
    qDebug() << "Final promotion with parentHeaderLevel =" << parentHeaderLevel
             << ", result length =" << finalResult.length();

    return finalResult;
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
    QString result = markdown;

    // Normalize line endings for consistent matching
    result.replace("\r\n", "\n").replace("\r", "\n");

    // Regex to match fenced code blocks:
    // 1: fence chars (``` or ~~~), 2: language tag (optional),
    // 3: fenced content, then closing fence equal to opening
    static const QRegularExpression fenceRe(
        R"(^([`~]{3,})[ \t]*([a-zA-Z0-9_.+-]*)[ \t]*\n([\s\S]*?)\n\1[ \t]*$)",
        QRegularExpression::MultilineOption | QRegularExpression::CaseInsensitiveOption);

    int offset = 0;

    while (true)
    {
        QRegularExpressionMatch match = fenceRe.match(result, offset);
        if (!match.hasMatch())
            break;

        int start = match.capturedStart(0);
        int length = match.capturedLength(0);

        int fenceLen = match.captured(1).length();
        QChar fenceChar = match.captured(1)[0];
        QString lang = match.captured(2).trimmed();
        QString innerContent = match.captured(3);

        qDebug() << "Sanitize fences at offset" << offset
                 << "| outerFenceLength:" << outerFenceLength
                 << "| fenceLen:" << fenceLen
                 << "| fenceChar:" << fenceChar
                 << "| lang:[" << lang << "]";

        // Recurse with strictly incremented fence length to ensure nesting
        QString sanitizedInner = sanitizeFencesRecursive(innerContent, outerFenceLength + 1);

        // Current block fence length must be outerFenceLength (strictly increasing)
        int newFenceLen = outerFenceLength;
        QString newFence(newFenceLen, fenceChar);

        // Build replacement fenced block string
        QString replacement = QString("%1%2\n%3\n%1\n")
                                  .arg(newFence)
                                  .arg(lang)
                                  .arg(sanitizedInner);

        QString originalBlock = result.mid(start, length);

        if (originalBlock == replacement) {
            // No change; advance offset to avoid infinite loop
            qDebug() << "No replacement needed, advancing offset from" << offset << "to" << (start + length);
            offset = start + length;
            continue;
        }

        // Replace fenced block in result string with sanitized version
        result.replace(start, length, replacement);

        // Set offset to just after replacement to continue scanning
        offset = start + replacement.length();

        qDebug() << "Replaced fences at" << start << "length" << length << "new offset" << offset
                 << "sanitized length" << result.length();
    }

    qDebug() << "Sanitize recursion complete at fence length" << outerFenceLength
             << "final length" << result.length();

    return result;
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

QString Session::expandIncludesRecursiveWithoutPromotion(const QString &content,
                                                         QSet<QString> &visitedFiles,
                                                         int parentHeaderLevel)
{
    static const QRegularExpression includeRe(R"(<!--\s*include:\s*(.*?)\s*-->)");


    qDebug() << "expandIncludesRecursiveWithoutPromotion called with parentHeaderLevel ="
             << parentHeaderLevel << ", visitedFiles size =" << visitedFiles.size();

    QString result = content;

    if (!m_project) {
        qWarning() << "No project assigned to session; cannot resolve includes";
        // Return content as-is; no promotion here (caller responsible)
        return result;
    }

    QString rootFolder = m_project->rootFolder();
    if (rootFolder.isEmpty())
        rootFolder = QDir::currentPath();

    int offset = 0;
    while (true) {
        QRegularExpressionMatch m = includeRe.match(result, offset);
        if (!m.hasMatch())
            break;

        QString includePath = m.captured(1).trimmed();
        QString absPath = QDir(rootFolder).filePath(includePath);

        qDebug() << "Include found:" << includePath << "resolved to" << absPath;

        if (visitedFiles.contains(absPath)) {
            qWarning() << "Circular include detected:" << absPath;
            offset = m.capturedEnd(0);
            continue;
        }

        visitedFiles.insert(absPath);

        int includePos = m.capturedStart(0);

        // Find the header level enclosing the include marker within current content
        int enclosingLevel = findEnclosingHeaderLevel(result, includePos);
        if (enclosingLevel == 0) {
            enclosingLevel = parentHeaderLevel; // fallback to passed header level
        }

        qDebug() << "Enclosing header level at include position:" << enclosingLevel;

        QString includedContent;
        QFile incFile(absPath);
        if (incFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            includedContent = incFile.readAll();
            incFile.close();

            // Recursive expand includes inside the included content, without promotion
            includedContent = expandIncludesRecursiveWithoutPromotion(includedContent, visitedFiles, enclosingLevel + 1);

            // **Do NOT promote headers here**
            // Leave header promotion to the caller after recursion returns

            qDebug() << "Included content after recursive expansion (no promotion) length =" << includedContent.length();

        } else {
            includedContent = QString("[Could not include file: %1]").arg(absPath);
            qWarning() << "Failed to open include file:" << absPath;
        }

        int start = m.capturedStart(0);
        int length = m.capturedLength(0);

        // Replace include marker with expanded included content (no promoted headers yet)
        result.replace(start, length, includedContent);

        offset = start + includedContent.length();

        visitedFiles.remove(absPath);
        qDebug() << "Include replaced at position" << start << ", new offset =" << offset;
    }

    // **DO NOT promote headers here**; caller handles promotion after recursion

    return result;
}


// Return directory where session file resides, used for placing cached folders
QString Session::sessionFolder() const
{
    QFileInfo fi(m_filepath);
    return fi.absolutePath();
}

// Cache folder for docs: e.g. sessions/001-doc
QString Session::sessionDocCacheFolder() const
{
    QFileInfo fi(m_filepath);
    QString base = fi.baseName(); // e.g., "001"
    QString folder = sessionFolder();
    QString cacheFolder = QDir(folder).filePath(base + "-doc");
    QDir().mkpath(cacheFolder);
    return cacheFolder;
}

// Cache folder for sources: e.g. sessions/001-src
QString Session::sessionSrcCacheFolder() const
{
    QFileInfo fi(m_filepath);
    QString base = fi.baseName();
    QString folder = sessionFolder();
    QString cacheFolder = QDir(folder).filePath(base + "-src");
    QDir().mkpath(cacheFolder);
    return cacheFolder;
}

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

// Main method to scan content and cache all includes, rewriting markers to cached references
QString Session::cacheIncludesInContent(const QString &content)
{
    QString result = content;
    if (!m_project) {
        qWarning() << "No project available to resolve includes in caching";
        return result;
    }

    QString projectRoot = m_project->rootFolder();
    QString docCacheFolder = sessionDocCacheFolder();
    QString srcCacheFolder = sessionSrcCacheFolder();

    // Regex to match both include and cached markers
    static QRegularExpression includeOrCachedRe(R"(<!--\s*(include|cached):\s*(.*?)\s*-->)");

    int offset = 0;
    while (true) {
        QRegularExpressionMatch m = includeOrCachedRe.match(result, offset);
        if (!m.hasMatch())
            break;

        QString type = m.captured(1).trimmed().toLower(); // "include" or "cached"
        QString includePath = m.captured(2).trimmed();

        // Determine absolute source file path (relative to project root)
        QString absSrcFile = QDir(projectRoot).filePath(includePath);

        // Determine cache folder by heuristic: docs in docs cache, src in src cache
        QString lowerPath = includePath.toLower();
        QString cacheFolder = lowerPath.endsWith(".h") || lowerPath.endsWith(".cpp") || lowerPath.endsWith(".hpp")
                                  ? srcCacheFolder : docCacheFolder;

        // Copy source file to cache folder, keep relative sub-paths intact
        bool copied = copyFileToCacheFolder(absSrcFile, cacheFolder, includePath);

        // Rewrite include line if it is "include", keep as cached if already "cached"
        QString replacement;
        if (type == "include") {
            replacement = QString("<!-- cached: %1 -->").arg(includePath);
        } else {
            replacement = m.captured(0); // keep cached as is
        }

        int start = m.capturedStart(0);
        int length = m.capturedLength(0);

        result.replace(start, length, replacement);
        offset = start + replacement.length();

        qDebug() << "Processed include:" << includePath << "Type:" << type << "Copied:" << copied;
    }

    return result;
}
