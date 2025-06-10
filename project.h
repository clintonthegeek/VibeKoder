#ifndef PROJECT_H
#define PROJECT_H

#include <QObject>
#include <QString>
#include <QVariant>
#include <QStringList>
#include "configmanager.h"

class Project : public QObject
{
    Q_OBJECT
public:
    explicit Project(QObject *parent = nullptr);
    ~Project() override;

    // Load project config JSON file and schema, initialize ConfigManager
    bool load(const QString &filepath);

    // Save project config JSON file
    bool save(const QString &filepath = QString());

    // Dynamic accessors for any config key by dot-separated path
    QVariant getValue(const QString &keyPath, const QVariant &defaultValue = QVariant()) const;
    void setValue(const QString &keyPath, const QVariant &value);

    // Convenience getters for common project settings
    QString rootFolder() const { return getValue("folders.root").toString(); }
    QString docsFolder() const { return getValue("folders.docs").toString(); }
    QString srcFolder() const { return getValue("folders.src").toString(); }
    QString sessionsFolder() const { return getValue("folders.sessions").toString(); }
    QString templatesFolder() const { return getValue("folders.templates").toString(); }
    QStringList includeDocFolders() const { return getValue("folders.include_docs").toStringList(); }

    QString accessToken() const { return getValue("api.access_token").toString(); }
    QString model() const { return getValue("api.model").toString(); }
    int maxTokens() const { return getValue("api.max_tokens").toInt(); }
    double temperature() const { return getValue("api.temperature").toDouble(); }
    double topP() const { return getValue("api.top_p").toDouble(); }
    double frequencyPenalty() const { return getValue("api.frequency_penalty").toDouble(); }
    double presencePenalty() const { return getValue("api.presence_penalty").toDouble(); }

    // Access underlying ConfigManager (optional)
    ConfigManager* configManager() const { return m_configManager; }

    // Get project config file path
    QString projectFilePath() const { return m_projectFilePath; }

private:
    QString m_projectFilePath;
    ConfigManager* m_configManager = nullptr;

    // Helper to determine schema path (could be from AppConfig or fixed location)
    QString schemaFilePath() const;
};

#endif // PROJECT_H
