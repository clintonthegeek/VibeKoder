#ifndef PROJECT_H
#define PROJECT_H

#include <QString>
#include <QMap>

class Project
{
public:
    Project();
    bool load(const QString &filepath);
    bool save(const QString &filepath);

    QString accessToken() const;
    QString model() const;
    int maxTokens() const;
    double temperature() const;
    double topP() const;
    double frequencyPenalty() const;
    double presencePenalty() const;

    QString rootFolder() const;
    QStringList includeDocs() const;
    QStringList sourceFolders() const;
    QStringList sourceFileTypes() const;
    QStringList docFileTypes() const;
    QMap<QString, QString> commandPipes() const;

private:
    QString m_accessToken;
    QString m_model;
    int m_maxTokens;
    double m_temperature;
    double m_topP;
    double m_frequencyPenalty;
    double m_presencePenalty;

    QString m_rootFolder;
    QStringList m_includeDocs;
    QStringList m_sourceFolders;
    QStringList m_sourceFileTypes;
    QStringList m_docFileTypes;
    QMap<QString, QString> m_commandPipes; // key=command name, val=command line

    QString m_rawTomlPath;

    bool parseToml(const QString &content);
};

#endif // PROJECT_H
