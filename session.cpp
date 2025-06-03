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
    m_metadata.clear();

    return parseSessionFile(data);
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

    out << serializeSessionFile();

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
        QString replacement = QString("<!-- cached: %1 -->").arg(relativeCachedPath);

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

        // Recursively expand includes inside includedContent
        // For "include" markers, pass expandIncludeMarkers flag as true recursively
        // For cached, always expand includes inside recursively (expandIncludeMarkers = true)
        bool recursiveExpandIncludeMark = (markerType == "include") ? expandIncludeMarkers : true;

        // Replace marker in result with expanded content
        int start = m.capturedStart(0);
        int length = m.capturedLength(0);

        visitedFiles.remove(absPath);

        // Update code block ranges since result changed:
        // (optional, for complex edits this is better but can be skipped if replacements don't contain fences)
        // For simplicity, you can break and re-call function if needed.
    }

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

void Session::appendSystemSlice(const QString &markdownContent)
{
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    m_slices.append({MessageRole::System, markdownContent, timestamp});
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

        // Compose delimiter line for slice role + timestamp
        QString header = QString("=={ %1").arg(roleStr);
        if (!slice.timestamp.isEmpty()) {
            header += QString(" | %1").arg(slice.timestamp);
        }
        header += " }==";

        // Use fixed 3-backtick fenced block without sanitizing content
        const QString outerFence = "```";

        result += QString("%1\n%2markdown\n%3\n%2\n\n")
                      .arg(header)
                      .arg(outerFence)
                      .arg(slice.content.trimmed());
    }

    return result.trimmed();
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


bool Session::parseSessionFile(const QString &data)
{
    m_slices.clear();
    m_metadata.clear();

    // Step 1: Parse YAML-like metadata block at the top (optional)
    // Format:
    // ---
    // key: value
    // key2: value2
    // ---
    // (then the rest)

    int pos = 0;
    QString trimmedData = data.trimmed();

    if (trimmedData.startsWith("---")) {
        int metaStart = data.indexOf("---", pos);
        if (metaStart == -1)
            return false; // malformed

        int metaEnd = data.indexOf("\n---", metaStart + 3);
        if (metaEnd == -1)
            metaEnd = data.indexOf("\r\n---", metaStart + 3);
        if (metaEnd == -1) {
            // Try just "---" on a line by itself
            QRegularExpression metaEndRe(R"(^---\s*$)", QRegularExpression::MultilineOption);
            QRegularExpressionMatch match = metaEndRe.match(data, metaStart + 3);
            if (match.hasMatch())
                metaEnd = match.capturedStart(0);
        }

        if (metaEnd == -1) {
            // No closing --- found, treat as no metadata block
            metaEnd = -1;
        } else {
            // Extract metadata block text
            int metaContentStart = metaStart + 3;
            int metaContentLength = metaEnd - metaContentStart;
            QString metaBlock = data.mid(metaContentStart, metaContentLength).trimmed();

            // Parse simple key: value lines
            QTextStream metaStream(&metaBlock);
            while (!metaStream.atEnd()) {
                QString line = metaStream.readLine().trimmed();
                if (line.isEmpty() || line.startsWith("#"))
                    continue;

                int colonIndex = line.indexOf(':');
                if (colonIndex == -1)
                    continue; // skip malformed

                QString key = line.left(colonIndex).trimmed();
                QString value = line.mid(colonIndex + 1).trimmed();

                // Remove optional quotes around value
                if ((value.startsWith('"') && value.endsWith('"')) ||
                    (value.startsWith('\'') && value.endsWith('\''))) {
                    value = value.mid(1, value.length() - 2);
                }

                m_metadata.insert(key, value);
            }

            // Move past closing --- and trailing newlines
            pos = metaEnd + 4; // Skip \n--- (4 chars: \n + ---)
            while (pos < data.length() && (data[pos] == '\n' || data[pos] == '\r')) {
                ++pos;
            }
        }
    }
    // Step 2: Parse slices separated by delimiter lines
    // Delimiter line format:
    // =={ Role | yyyy-MM-dd HH:mm:ss }==
    // or
    // =={ Role }==  (timestamp missing, add current time)

    // We'll parse line by line, detecting delimiter lines outside fenced code blocks.

    // Regex for delimiter line:
    static const QRegularExpression delimiterRe(
        R"(^==\{\s*(System|User|Assistant)\s*(?:\|\s*([0-9]{4}-[0-9]{2}-[0-9]{2} [0-9]{2}:[0-9]{2}:[0-9]{2}))?\s*\}==\s*$)",
        QRegularExpression::CaseInsensitiveOption);

    // We'll split data into lines starting from pos
    QStringList lines = data.mid(pos).split(QRegularExpression("[\r\n]"));

    // We need to track fenced code blocks to ignore delimiter lines inside them
    bool inFence = false;
    QString fenceMarker; // e.g. ``` or ~~~

    QVector<PromptSlice> slices;
    QString currentRole;
    QString currentTimestamp;
    QStringList currentContentLines;

    auto addSlice = [&]() {
        if (currentRole.isEmpty())
            return;

        for (int i = 0; i < currentContentLines.size(); ++i) {
            QString line = currentContentLines[i];
            if (line.length() > 120) {
                line = line.left(120) + "...";
            }
        }

        QString content = currentContentLines.join("\n");

        // If timestamp missing, add current time
        if (currentTimestamp.isEmpty()) {
            currentTimestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
        }

        MessageRole role = MessageRole::User;
        QString roleLower = currentRole.toLower();
        if (roleLower == "system")
            role = MessageRole::System;
        else if (roleLower == "assistant")
            role = MessageRole::Assistant;
        else if (roleLower == "user")
            role = MessageRole::User;

        slices.append({role, content, currentTimestamp});

        currentRole.clear();
        currentTimestamp.clear();
        currentContentLines.clear();
    };

    for (int i = 0; i < lines.size(); ++i) {
        QString line = lines[i];

        // Detect fenced code block start/end
        // Fence start: line starts with at least 3 backticks or tildes
        QRegularExpression fenceStartRe(R"(^([`~]{3,}).*)");
        QRegularExpressionMatch fenceMatch = fenceStartRe.match(line);

        if (fenceMatch.hasMatch()) {
            QString fence = fenceMatch.captured(1);
            if (!inFence) {
                inFence = true;
                fenceMarker = fence;
            } else {
                // Only close fence if matches opening fence exactly
                if (line.startsWith(fenceMarker)) {
                    inFence = false;
                    fenceMarker.clear();
                }
            }
            currentContentLines.append(line);
            continue;
        }

        if (!inFence) {
            // Check if line is delimiter line
            QRegularExpressionMatch delimMatch = delimiterRe.match(line);
            if (delimMatch.hasMatch()) {
                // If we have a current slice, add it before starting new one
                addSlice();

                currentRole = delimMatch.captured(1);
                currentTimestamp = delimMatch.captured(2); // may be empty
                continue; // delimiter line not part of content
            }
        }

        // Otherwise, add line to current content
        currentContentLines.append(line);
    }

    // Add last slice if any
    addSlice();

    if (slices.isEmpty()) {
        qWarning() << "No prompt slices found in session file.";
        return false;
    }

    m_slices = slices;

    return true;
}

QString Session::serializeSessionFile() const
{
    QString result;

    // Step 1: Serialize metadata block if any
    if (!m_metadata.isEmpty()) {
        result += "---\n";
        for (auto it = m_metadata.constBegin(); it != m_metadata.constEnd(); ++it) {
            // Serialize as key: value
            QString key = it.key();
            QString val = it.value().toString();

            // Escape value if contains spaces or special chars
            if (val.contains(QRegExp(R"(\s)")) || val.contains(":")) {
                val = "\"" + val.replace("\"", "\\\"") + "\"";
            }

            result += QString("%1: %2\n").arg(key, val);
        }
        result += "---\n\n";
    }

    // Step 2: Serialize slices with delimiter lines and content

    for (const PromptSlice &slice : m_slices) {
        QString roleStr;
        switch (slice.role) {
        case MessageRole::User: roleStr = "User"; break;
        case MessageRole::Assistant: roleStr = "Assistant"; break;
        case MessageRole::System: roleStr = "System"; break;
        }

        QString timestampStr = slice.timestamp;
        if (timestampStr.isEmpty()) {
            timestampStr = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
        }

        // Delimiter line
        result += QString("=={ %1 | %2 }==\n").arg(roleStr, timestampStr);

        // Slice content verbatim
        result += slice.content;
        result += "\n\n";
    }

    return result.trimmed();
}



