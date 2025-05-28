#ifndef SESSION_H
#define SESSION_H

#include <QString>
#include <QVector>
#include <QMap>

enum class MessageRole {
    User,
    Assistant,
    System
};

struct PromptSlice {
    MessageRole role;
    QString content;      // Raw markdown content inside fenced block
    QString timestamp;    // ISO8601 or simple local timestamp string

    PromptSlice() = default;
    PromptSlice(MessageRole r, const QString& c, const QString& t)
        : role(r), content(c), timestamp(t) {}
};

class Project;

class Session
{
public:
    explicit Session(Project *project = nullptr);

    bool load(const QString &filepath);
    bool save(const QString &filepath);

    QString filePath() const;

    QVector<PromptSlice> slices() const;

    // Appends a new user prompt slice with current time
    void appendUserSlice(const QString &markdownContent);
    void appendAssistantSlice(const QString &markdownContent);
    void appendSystemSlice(const QString &markdownContent);

    // Compiles the prompt to a single markdown string, resolving includes recursively.
    // command pipe tokens are not expanded in Phase 1.
    QString compilePrompt();

    void setCommandPipeOutput(const QString& key, const QString& output);
    QString commandPipeOutput(const QString& key) const;

private:
    QString m_filepath;
    Project* m_project;

    QVector<PromptSlice> m_slices;
    QMap<QString, QString> m_commandPipeOutputs; // @diff, @make outputs...

    bool parseSessionMarkdown(const QString &data);
    QString expandIncludesRecursive(const QString &content, QSet<QString> &visitedFiles);
};

#endif // SESSION_H
