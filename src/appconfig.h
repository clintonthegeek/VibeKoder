#ifndef APPCONFIG_H
#define APPCONFIG_h

#include <QObject>
#include <QString>
#include <QVariantMap>

#include "appconfigtypes.h"

class AppConfig : public QObject
{
    Q_OBJECT
public:
    static AppConfig& instance();

    // Load config from disk or create defaults if missing
    bool load();

    // Save current config to disk
    bool save() const;

    // Direct access to config data
    AppConfigData& data() { return m_data; }
    const AppConfigData& data() const { return m_data; }

    // DEPRECATED: Temporary compatibility wrappers - to be removed in Phase 5
    QVariant getValue(const QString& keyPath, const QVariant& defaultValue = QVariant()) const;
    void setValue(const QString& keyPath, const QVariant& value);
    void setConfigMap(const QVariantMap &map);
    QVariantMap getConfigMap() const;

    // Path to config folder and config file
    QString configFolder() const;
    QString configFilePath() const;

signals:
    void configChanged();

private:
    explicit AppConfig(QObject* parent = nullptr);

    // Initialize config folder and copy default resources if needed
    void initializeConfigFolder();

    // Copy schema and templates from resource to user config folder
    void copyDefaultResources();

    QString m_configFolder;
    QString m_configFilePath;
    AppConfigData m_data;
};

#endif // APPCONFIG_H
