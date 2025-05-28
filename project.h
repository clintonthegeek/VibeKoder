#ifndef PROJECT_H
#define PROJECT_H

#include <QString>
#include <QStringList>
#include <QMap>

class Project
{
public:
    Project();

    bool load(const QString &filepath);
    bool save(const QString &filepath);

    // Getters for config fields
    QString rootFolder() const;
    QString docsFolder() const;
    QString srcFolder() const;
    QString sessionsFolder() const;
    QString templatesFolder() const;
    QStringList includeDocFolders() const; // absolute paths

    QStringList sourceFileTypes() const;
    QStringList docFileTypes() const;

    // command pipe name -> [command, working_dir (relative to root)]
    QMap<QString, QStringList> commandPipes() const;

    // API getters
    QString accessToken() const;
    QString model() const;
    int maxTokens() const;
    double temperature() const;
    double topP() const;
    double frequencyPenalty() const;
    double presencePenalty() const;

    // Recursive scanning stubs
    QStringList scanDocsRecursive() const;
    QStringList scanSourceRecursive() const;

private:
    bool parseToml(const QString &content, const QString &projectFilePath);

    // Member variables representing config
    QString m_projectFilePath;

    QString m_rootFolder;
    QString m_docsFolder;
    QString m_srcFolder;
    QString m_sessionsFolder;
    QString m_templatesFolder;
    QStringList m_includeDocFolders;

    QStringList m_sourceFileTypes;
    QStringList m_docFileTypes;

    QMap<QString, QStringList> m_commandPipes;

    // API keys and params
    QString m_accessToken;
    QString m_model;
    int m_maxTokens;
    double m_temperature;
    double m_topP;
    double m_frequencyPenalty;
    double m_presencePenalty;
};

#endif // PROJECT_H
