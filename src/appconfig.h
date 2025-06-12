#ifndef APPCONFIG_H
#define APPCONFIG_H

#include <QObject>
#include <QString>
#include <QVariantMap>

#include "configmanager.h"

class AppConfig : public QObject
{
    Q_OBJECT
public:
    static AppConfig& instance();

    // Load config from disk or create defaults if missing
    bool load();

    // Save current config to disk
    bool save() const;

    // Access config values dynamically
    QVariant getValue(const QString& keyPath, const QVariant& defaultValue = QVariant()) const;
    void setValue(const QString& keyPath, const QVariant& value);

    // Path to config folder and config file
    QString configFolder() const;
    QString configFilePath() const;

    void setConfigMap(const QVariantMap &map);
    QVariantMap getConfigMap() const;

signals:
    void configChanged();

private:
    explicit AppConfig(QObject* parent = nullptr);

    // Initialize config folder and copy default resources if needed
    void initializeConfigFolder();

    // Copy schema and templates from resource to user config folder
    void copyDefaultResources();

    // Generate config.xml from schema.json (optional)
    void generateConfigJsonFromSchema();

    QString m_configFolder;
    QString m_configFilePath;

    ConfigManager* m_configManager = nullptr;
};

#endif // APPCONFIG_H
