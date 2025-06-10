#ifndef APPCONFIG_H
#define APPCONFIG_H

#include <QObject>
#include <QString>
#include <QMap>
#include <QStringList>
#include <QVariantMap>

class AppConfig : public QObject
{
    Q_OBJECT
public:
    static AppConfig& instance();

    // Load config from disk; returns true if successful or created defaults
    bool load();

    // Save config to disk
    bool save() const;

    // Getters for default project settings
    QVariantMap defaultApiSettings() const;
    QVariantMap defaultFolders() const;
    QStringList defaultSourceFileTypes() const;
    QStringList defaultDocFileTypes() const;
    QMap<QString, QStringList> defaultCommandPipes() const;

    // Setters for default project settings
    void setDefaultApiSettings(const QVariantMap& apiSettings);
    void setDefaultFolders(const QVariantMap& folders);
    void setDefaultSourceFileTypes(const QStringList& types);
    void setDefaultDocFileTypes(const QStringList& types);
    void setDefaultCommandPipes(const QMap<QString, QStringList>& pipes);

    // Path to config folder and config file
    QString configFolder() const;
    QString configFilePath() const;

signals:
    void configChanged();

private:
    explicit AppConfig(QObject* parent = nullptr);
    void createDefaultConfigFile();
    void copyDefaultDocsAndTemplates();

    // Parsing helpers
    void parseDefaultProjectSettings(const QJsonObject& obj);
    void parseAppSettings(const QJsonObject& obj);

    QVariantMap m_apiSettings;
    QVariantMap m_folders;
    QStringList m_sourceFileTypes;
    QStringList m_docFileTypes;
    QMap<QString, QStringList> m_commandPipes;

    QVariantMap m_appSettings;

    QString m_configFolder;
    QString m_configFilePath;
};

#endif // APPCONFIG_H
