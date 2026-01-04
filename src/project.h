#ifndef PROJECT_H
#define PROJECT_H

#include <QObject>
#include <QString>
#include <QVariant>
#include <QStringList>
#include "projectconfig.h"

class Project : public QObject
{
    Q_OBJECT
public:
    explicit Project(QObject *parent = nullptr);
    ~Project() override;

    // Load project config JSON file
    bool load(const QString &filepath);

    // Save project config JSON file
    bool save(const QString &filepath = QString());

    // Direct access to config structure
    ProjectConfig& config() { return m_config; }
    const ProjectConfig& config() const { return m_config; }

    // DEPRECATED: Temporary compatibility wrappers for getValue/setValue
    // These will be removed once MainWindow is updated
    QVariant getValue(const QString &keyPath, const QVariant &defaultValue = QVariant()) const;
    void setValue(const QString &keyPath, const QVariant &value);

    // Convenience getters for common project settings
    QString rootFolder() const { return m_config.rootFolder; }
    QString docsFolder() const { return m_config.docsFolder; }
    QString srcFolder() const { return m_config.srcFolder; }
    QString sessionsFolder() const { return m_config.sessionsFolder; }
    QString templatesFolder() const { return m_config.templatesFolder; }
    QStringList includeDocFolders() const { return m_config.includeDocFolders; }

    QString accessToken() const { return m_config.apiAccessToken; }
    QString model() const { return m_config.apiModel; }
    int maxTokens() const { return m_config.apiMaxTokens; }
    double temperature() const { return m_config.apiTemperature; }
    double topP() const { return m_config.apiTopP; }
    double frequencyPenalty() const { return m_config.apiFrequencyPenalty; }
    double presencePenalty() const { return m_config.apiPresencePenalty; }

    // Get project config file path
    QString projectFilePath() const { return m_projectFilePath; }

private:
    QString m_projectFilePath;
    ProjectConfig m_config;
};

#endif // PROJECT_H
