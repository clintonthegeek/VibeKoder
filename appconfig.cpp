#include "appconfig.h"

#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QXmlStreamWriter>

AppConfig& AppConfig::instance()
{
    static AppConfig instance;
    return instance;
}

AppConfig::AppConfig(QObject* parent)
    : QObject(parent)
{
    m_configFolder = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    if (m_configFolder.isEmpty()) {
        m_configFolder = QDir::home().filePath(".config/VibeKoder");
    }
    QDir dir(m_configFolder);
    if (!dir.exists()) {
        if (!dir.mkpath(".")) {
            qWarning() << "Failed to create config folder:" << m_configFolder;
        }
    }
    m_configFilePath = dir.filePath("config.json");

    // Initialize ConfigManager with config and schema paths
    m_configManager = new ConfigManager(m_configFilePath, QDir(m_configFolder).filePath("schema.json"), this);

    // Connect configChanged signal to propagate
    connect(m_configManager, &ConfigManager::configChanged, this, &AppConfig::configChanged);
}

bool AppConfig::load()
{
    initializeConfigFolder();

    bool loaded = m_configManager->load();
    if (!loaded) {
        qWarning() << "Failed to load config via ConfigManager";
        return false;
    }

    // Optionally validate config here
    if (!m_configManager->validate()) {
        qWarning() << "Config validation failed";
        // You may choose to handle validation failure (e.g., reset to defaults)
    }

    return true;
}

bool AppConfig::save() const
{
    return m_configManager->save();
}

QVariant AppConfig::getValue(const QString& keyPath, const QVariant& defaultValue) const
{
    if (!m_configManager)
        return defaultValue;
    return m_configManager->getValue(keyPath, defaultValue);
}

void AppConfig::setValue(const QString& keyPath, const QVariant& value)
{
    if (!m_configManager)
        return;
    m_configManager->setValue(keyPath, value);
}

QString AppConfig::configFolder() const
{
    return m_configFolder;
}

QString AppConfig::configFilePath() const
{
    return m_configFilePath;
}

void AppConfig::initializeConfigFolder()
{
    // Ensure config folder exists
    QDir dir(m_configFolder);
    if (!dir.exists()) {
        if (!dir.mkpath(".")) {
            qWarning() << "Failed to create config folder:" << m_configFolder;
            return;
        }
    }

    // Copy default resources (schema.json, templates) if missing
    copyDefaultResources();

    // Generate config.xml from schema.json if missing
    generateConfigJsonFromSchema();
}

void AppConfig::setConfigMap(const QVariantMap &map)
{
    if (!m_configManager)
        return;
    // Assuming ConfigManager has a method to set the entire config object
    m_configManager->setConfigObject(QJsonObject::fromVariantMap(map));
    emit configChanged();
}

QVariantMap AppConfig::getConfigMap() const
{
    if (!m_configManager)
        return QVariantMap();
    return m_configManager->configObject().toVariantMap();
}

void AppConfig::copyDefaultResources()
{
    // Copy templates from resource, renaming .VKTemplate to .md
    QString templatesDestDir = QDir(m_configFolder).filePath("templates");
    QDir templatesDir(templatesDestDir);
    if (!templatesDir.exists()) {
        if (!QDir(m_configFolder).mkpath("templates")) {
            qWarning() << "Failed to create templates folder:" << templatesDestDir;
            return;
        }
    }

    const QStringList templateResources = {
        ":/templates/Development.VKTemplate",
        ":/templates/SimpleQuery.VKTemplate"
    };

    for (const QString& resPath : templateResources) {
        QFileInfo fi(resPath);
        QString baseName = fi.completeBaseName(); // e.g. "Development"
        QString destFileName = baseName + ".md";
        QString destFilePath = templatesDir.filePath(destFileName);

        if (!QFile::exists(destFilePath)) {
            QFile resFile(resPath);
            if (resFile.open(QIODevice::ReadOnly)) {
                QFile outFile(destFilePath);
                if (outFile.open(QIODevice::WriteOnly)) {
                    outFile.write(resFile.readAll());
                    outFile.close();
                    qDebug() << "Copied template" << resPath << "to" << destFilePath;
                } else {
                    qWarning() << "Failed to open template destination for writing:" << destFilePath;
                }
                resFile.close();
            } else {
                qWarning() << "Failed to open template resource:" << resPath;
            }
        }
    }
}

void AppConfig::generateConfigJsonFromSchema()
{
    QString configJsonPath = QDir(m_configFolder).filePath("config.json");
    if (QFile::exists(configJsonPath))
        return; // Don't overwrite existing config.json

    // Load schema from resource instead of file
    QFile schemaFile(":/config/schema.json");
    if (!schemaFile.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open schema resource for generating config.json";
        return;
    }
    QByteArray schemaData = schemaFile.readAll();
    schemaFile.close();

    QJsonParseError err;
    QJsonDocument schemaDoc = QJsonDocument::fromJson(schemaData, &err);
    if (err.error != QJsonParseError::NoError) {
        qWarning() << "Failed to parse schema.json for generating config.json:" << err.errorString();
        return;
    }
    if (!schemaDoc.isObject()) {
        qWarning() << "schema.json root is not an object for generating config.json";
        return;
    }
    QJsonObject schemaObj = schemaDoc.object();

    // Recursive lambda to extract defaults from a schema object
    std::function<QJsonObject(const QJsonObject&)> buildDefaults;
    buildDefaults = [&](const QJsonObject& schema) -> QJsonObject {
        QJsonObject result;
        if (!schema.contains("properties") || !schema.value("properties").isObject())
            return result;
        QJsonObject props = schema.value("properties").toObject();
        for (const QString& key : props.keys()) {
            QJsonObject propSchema = props.value(key).toObject();
            if (propSchema.contains("default")) {
                result.insert(key, propSchema.value("default"));
            } else if (propSchema.value("type") == "object") {
                result.insert(key, buildDefaults(propSchema));
            }
            // else skip keys without defaults
        }
        return result;
    };

    QJsonObject defaultConfig;

    // Extract defaults for default_project_settings
    if (schemaObj.contains("default_project_settings") && schemaObj.value("default_project_settings").isObject()) {
        QJsonObject dpsSchema = schemaObj.value("default_project_settings").toObject();
        QJsonObject dpsDefaults = buildDefaults(dpsSchema);
        defaultConfig.insert("default_project_settings", dpsDefaults);
    } else {
        qWarning() << "Schema missing 'default_project_settings' object";
    }

    // Extract defaults for app_settings
    if (schemaObj.contains("app_settings") && schemaObj.value("app_settings").isObject()) {
        QJsonObject appSchema = schemaObj.value("app_settings").toObject();
        QJsonObject appDefaults = buildDefaults(appSchema);
        defaultConfig.insert("app_settings", appDefaults);
    } else {
        qWarning() << "Schema missing 'app_settings' object";
    }

    QFile configFile(configJsonPath);
    if (!configFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Failed to open config.json for writing";
        return;
    }

    QJsonDocument configDoc(defaultConfig);
    configFile.write(configDoc.toJson(QJsonDocument::Indented));
    configFile.close();

    qDebug() << "Generated config.json with defaults from schema at" << configJsonPath;
}

