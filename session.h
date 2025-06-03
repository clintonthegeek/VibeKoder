#ifndef SESSION_H
#define SESSION_H

#include <QString>
#include <QVector>
#include <QMap>
#include <QSet>
#include <QVariantMap>


enum class MessageRole {
    User,
    Assistant,
    System
};

struct PromptSlice {
    MessageRole role;
    QString content;      // Raw markdown inside fenced block
    QString timestamp;    // e.g., "2025-05-28 14:30:00"

    PromptSlice() = default;
    PromptSlice(MessageRole r, const QString& c, const QString& t)
        : role(r), content(c), timestamp(t) {}
};

class Project; // forward decl

class Session
{
public:
    explicit Session(Project *project = nullptr);

    bool load(const QString &filepath);
    bool save(const QString &filepath = QString());
    bool refreshCacheAndSave();


    // Accessors
    QVector<PromptSlice> slices() const;

    // Append new prompt slices with current timestamp
    void appendUserSlice(const QString &markdownContent);
    void appendAssistantSlice(const QString &markdownContent);
    void appendSystemSlice(const QString &markdownContent);

    QString promptSliceContent(int index) const;
    void setPromptSliceContent(int index, const QString &content);

    // Parse markdown session data into slices (public for your editor save slot)
    bool parseSessionMarkdown(const QString &data);

    // Compile the prompt into a single markdown string expanded with recursive includes
    // Command pipe tokens (@diff etc.) remain as-is.
    QString compilePrompt();

    // For your command pipe integration later
    void setCommandPipeOutput(const QString& key, const QString& output);
    QString commandPipeOutput(const QString& key) const;

    QString compiledRawMarkdown() const;

private:
    QString m_filepath;
    Project* m_project; // used for base folder path to resolve includes

    QVector<PromptSlice> m_slices;
    QMap<QString, QString> m_commandPipeOutputs;

    bool parseSessionFile(const QString &data);
    QString serializeSessionFile() const;

    // Internal helper to recursively expand includes in content
    // Updated expandIncludesRecursive with additional parameter for header level context
    QString expandIncludesRecursive(const QString &content,
                                             QSet<QString> &visitedFiles,
                                             bool expandIncludeMarkers);

    QString expandIncludesOnce(const QString &content);



    QString cacheIncludesInContent(const QString& content);
    QString sessionFolder() const;
    QString sessionCacheFolder() const;
    QString sessionDocCacheFolder() const;
    QString sessionSrcCacheFolder() const;
    QString sessionCacheBaseFolder() const;
    QVariantMap m_metadata;
};

#endif // SESSION_H
