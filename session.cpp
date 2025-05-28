#include "session.h"
#include "project.h"
#include <QFile>
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

    static QRegularExpression headerRe(R"(###\s*(User|Assistant|System)(?:\s*\((.*?)\))?\s*)");
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
        // Expand includes recursively in slice content
        QString expanded = expandIncludesRecursive(slice.content, visitedFiles);

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

QString Session::expandIncludesRecursive(const QString &content, QSet<QString> &visitedFiles)
{
    // Looks for <!-- include: path --> markers and includes contents recursively.
    // Paths relative to project root folder.

    static QRegularExpression includeRe(R"(<!--\s*include:\s*(.*?)\s*-->)");

    QString result = content;

    if (!m_project) {
        qWarning() << "No project associated with session, cannot resolve includes";
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

        if (visitedFiles.contains(absPath)) {
            qWarning() << "Circular include detected:" << absPath;
            offset = m.capturedEnd(0);
            continue; // skip to avoid infinite loop
        }

        visitedFiles.insert(absPath);

        QString includedContent;
        QFile incFile(absPath);
        if (incFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            includedContent = incFile.readAll();
            incFile.close();

            // Recursive expansion in included content
            includedContent = expandIncludesRecursive(includedContent, visitedFiles);
        } else {
            includedContent = QString("[Could not include file: %1]").arg(absPath);
        }

        int start = m.capturedStart(0);
        int length = m.capturedLength(0);

        result.replace(start, length, includedContent);

        offset = start + includedContent.length();

        visitedFiles.remove(absPath);
    }

    return result;
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

        // Recursively sanitize slice content fences here:
        QString sanitizedContent = sanitizeFencesRecursive(slice.content);

        // Fence length chosen automatically in sanitizer; just use 3 backticks here to open block
        // because sanitizedContent includes fences of varying length inside already.
        // But to be safe, let's pick an outer fence at least length 3:
        // We can reuse sanitizeFencesRecursive to pick the correct fence length, or just hardcode 3 for outer

        // Using 3 backticks as outer fence is safe here because inner fences are all length >= 3+1
        QString outerFence(3, '`');

        result += QString("### %1 (%2)\n%3markdown\n%4\n%3\n\n")
                      .arg(roleStr)
                      .arg(slice.timestamp)
                      .arg(outerFence)
                      .arg(sanitizedContent.trimmed());
    }
    return result;
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
