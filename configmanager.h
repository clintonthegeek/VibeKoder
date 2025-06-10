#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

#include <QObject>
#include <QString>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonValue>
#include <QVariant>
#include <QFile>
#include <QDebug>
#include <QJsonArray>
#include <QJsonParseError>
#include <QStringList>

class ConfigManager : public QObject
{
    Q_OBJECT
public:
    explicit ConfigManager(const QString &configFilePath,
                           const QString &schemaFilePath,
                           QObject *parent = nullptr);

    // Load config and schema from files, apply defaults
    bool load();

    // Save current config to config file
    bool save() const;

    // Get value by dot-separated key path, returns defaultValue if not found
    QVariant getValue(const QString &keyPath, const QVariant &defaultValue = QVariant()) const;

    // Set value by dot-separated key path
    void setValue(const QString &keyPath, const QVariant &value);

    // Validate current config against schema, returns true if valid
    bool validate() const;

    // Access raw config as QJsonObject (read-only)
    QJsonObject configObject() const { return m_configObject; }

signals:
    void configChanged();

private:
    QString m_configFilePath;
    QString m_schemaFilePath;

    QJsonObject m_configObject;
    QJsonObject m_schemaObject;

    // Recursive helpers for get/set
    QVariant getValueRecursive(const QJsonObject &obj, const QStringList &keys, int index) const;
    void setValueRecursive(QJsonObject &obj, const QStringList &keys, int index, const QVariant &value);

    // Apply defaults from schema recursively
    void applyDefaults(QJsonObject &obj, const QJsonObject &schema);

    // Validate recursively, returns true if valid
    bool validateRecursive(const QJsonObject &obj, const QJsonObject &schema, QString *errorMsg = nullptr) const;

    // Helper to convert QVariant to QJsonValue
    QJsonValue variantToJsonValue(const QVariant &var) const;

    // Helper to convert QJsonValue to QVariant
    QVariant jsonValueToVariant(const QJsonValue &val) const;
};

#endif // CONFIGMANAGER_H
