#include "session.h"
#include "project.h"
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QRegularExpression>
#include <QDir>
#include <QSet>
#include <QDebug>

Session::Session(Project *project)
    : m_project(project)
{
}

bool Session::load(const QString &filepath)
{
    QFile file(filepath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Failed to open session file" << filepath;
        return false;
    }

    m_filepath = filepath;

    QTextStream in(&file);
    const QString data = in.readAll();

    m_slices.clear();

    return parseSessionMarkdown(data);
}

bool Session::save(const QString &filepath)
{
    QFile file(filepath.isEmpty() ? m_filepath : filepath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Failed to write session file" << filepath;
        return false;
    }

    QTextStream out(&file);

    for (const auto &slice : m_slices) {
        // Header with role and timestamp
        QString roleStr;
        switch (slice.role) {
        case MessageRole::User: roleStr = "User"; break;
        case MessageRole::Assistant: roleStr = "Assistant"; break;
        case MessageRole::System: roleStr = "System"; break;
        }

        out << "### " << roleStr;
        if (!slice.timestamp.isEmpty()) {
            out << " (" << slice.timestamp << ")";
        }
        out << "\n\n";

        // fenced markdown block with content
        out << "```markdown\n";
        out << slice.content.trimmed() << "\n";
        out << "```\n\n";
    }

    file.flush();
    return true;
}

QString Session::filePath() const
{
    return m_filepath;
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
    // Very simple markdown parser:
    // For each heading ### Role(optional timestamp)
    // followed by ```markdown fenced block

    static const QRegularExpression headerRe(R"(###\s*(User|Assistant|System)(?:\s*\((.*?)\))?\s*)");
    static const QRegularExpression fenceRe(R"(```markdown\s*([\s\S]*?)```)");
    // The idea: find all heading occurrences, then their fenced block following

    int pos = 0;
    int len = data.length();

    while (pos < len) {
        QRegularExpressionMatch headerMatch = headerRe.match(data, pos);
        if (!headerMatch.hasMatch())
            break;

        auto roleStr = headerMatch.captured(1);
        auto timestamp = headerMatch.captured(2);
        MessageRole role = MessageRole::User;
        if (roleStr == "User") {
            role = MessageRole::User;
        } else if (roleStr == "Assistant") {
            role = MessageRole::Assistant;
        } else if (roleStr == "System") {
            role = MessageRole::System;
        }

        int fenceStart = headerMatch.capturedEnd(0);
        QRegularExpressionMatch fenceMatch = fenceRe.match(data, fenceStart);

        if (!fenceMatch.hasMatch()) {
            qWarning() << "Session file parse error: expected fenced block after header at position" << fenceStart;
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
    // Concatenate expand includes for all User, System, and Assistant slices
    // For now, output a simple concatenated markdown document:
    // System messages inserted wherever they appear,
    // Command pipes like @diff remain as-is

    QStringList parts;

    QSet<QString> visitedIncludes;

    for (const auto &slice : m_slices) {
        // Expand includes in content recursively
        QString expandedContent = expandIncludesRecursive(slice.content, visitedIncludes);

        QString roleIntro;
        switch (slice.role) {
        case MessageRole::System:
            roleIntro = "[System message]\n";
            break;
        case MessageRole::User:
            roleIntro = "[User prompt]\n";
            break;
        case MessageRole::Assistant:
            roleIntro = "[Assistant response]\n";
            break;
        }

        parts << roleIntro + expandedContent;
    }

    return parts.join("\n\n---\n\n");
}

QString Session::expandIncludesRecursive(const QString &content, QSet<QString> &visitedFiles)
{
    // Scan for <!-- include: path -->
    // Replace with content of that file, recursively expanding includes.
    // Relative paths interpreted relative to project root folder.

    static const QRegularExpression includeRe(R"(<!--\s*include:\s*(.+?)\s*-->)");

    QString result = content;

    auto projectRoot = (m_project ? m_project->rootFolder() : QString());
    if (projectRoot.isEmpty())
        projectRoot = QDir::currentPath();

    int offset = 0;
    QRegularExpressionMatch match;
    while ((match = includeRe.match(result, offset)).hasMatch()) {
        QString includePath = match.captured(1);
        QString absPath = QDir(projectRoot).filePath(includePath);

        // To avoid circular includes:
        if (visitedFiles.contains(absPath)) {
            qWarning() << "Circular include detected: " << absPath;
            offset = match.capturedEnd(0);
            continue; // skip expanding this include again
        }

        visitedFiles.insert(absPath);

        QString replacement;
        QFile incFile(absPath);
        if (incFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            replacement = incFile.readAll();
            incFile.close();

            // Recursive expansion of includes in this file's content
            replacement = expandIncludesRecursive(replacement, visitedFiles);
        } else {
            replacement = QString("[Could not include file: %1]").arg(absPath);
        }

        // Replace the include marker with the file content (recursive)
        int start = match.capturedStart(0);
        int end = match.capturedEnd(0);
        result.replace(start, end - start, replacement);

        offset = start + replacement.length();

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
