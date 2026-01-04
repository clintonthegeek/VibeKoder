#include "appconfig.h"

#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>

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
}

bool AppConfig::load()
{
    initializeConfigFolder();

    if (!m_data.loadFromFile(m_configFilePath)) {
        qWarning() << "Failed to load app config from" << m_configFilePath;
        qDebug() << "Creating default config";
        m_data = AppConfigData::createDefault();
        if (!m_data.saveToFile(m_configFilePath)) {
            qWarning() << "Failed to save default app config";
            return false;
        }
    }

    emit configChanged();
    return true;
}

bool AppConfig::save() const
{
    if (!m_data.saveToFile(m_configFilePath)) {
        qWarning() << "Failed to save app config to" << m_configFilePath;
        return false;
    }
    return true;
}

QString AppConfig::configFolder() const
{
    return m_configFolder;
}

QString AppConfig::configFilePath() const
{
    return m_configFilePath;
}

// DEPRECATED compatibility wrappers - to be removed in Phase 5
QVariant AppConfig::getValue(const QString& keyPath, const QVariant& defaultValue) const
{
    // Parse path and return corresponding value
    if (keyPath.startsWith("default_project_settings.")) {
        QString subPath = keyPath.mid(QString("default_project_settings.").length());

        if (subPath == "api") {
            QVariantMap api;
            api["access_token"] = m_data.defaultProjectSettings.apiAccessToken;
            api["model"] = m_data.defaultProjectSettings.apiModel;
            api["max_tokens"] = m_data.defaultProjectSettings.apiMaxTokens;
            api["temperature"] = m_data.defaultProjectSettings.apiTemperature;
            api["top_p"] = m_data.defaultProjectSettings.apiTopP;
            api["frequency_penalty"] = m_data.defaultProjectSettings.apiFrequencyPenalty;
            api["presence_penalty"] = m_data.defaultProjectSettings.apiPresencePenalty;
            return api;
        }

        if (subPath == "folders") {
            QVariantMap folders;
            folders["root"] = m_data.defaultProjectSettings.rootFolder;
            folders["docs"] = m_data.defaultProjectSettings.docsFolder;
            folders["src"] = m_data.defaultProjectSettings.srcFolder;
            folders["sessions"] = m_data.defaultProjectSettings.sessionsFolder;
            folders["templates"] = m_data.defaultProjectSettings.templatesFolder;
            folders["include_docs"] = m_data.defaultProjectSettings.includeDocFolders;
            return folders;
        }

        if (subPath == "filetypes.source") return m_data.defaultProjectSettings.sourceFileTypes;
        if (subPath == "filetypes.docs") return m_data.defaultProjectSettings.docFileTypes;

        if (subPath == "command_pipes") {
            QVariantMap pipes;
            for (auto it = m_data.defaultProjectSettings.commandPipes.constBegin();
                 it != m_data.defaultProjectSettings.commandPipes.constEnd(); ++it) {
                pipes.insert(it.key(), it.value());
            }
            return pipes;
        }
    }

    if (keyPath == "app_settings.timezone" || keyPath == "timezone") {
        return m_data.timezone;
    }

    qWarning() << "AppConfig::getValue: Unknown key path:" << keyPath;
    return defaultValue;
}

void AppConfig::setValue(const QString& keyPath, const QVariant& value)
{
    // This method is complex and not worth implementing fully
    // We'll handle it in Phase 5 when we remove compatibility layers
    qWarning() << "AppConfig::setValue is deprecated, use data() instead:" << keyPath;
}

QVariantMap AppConfig::getConfigMap() const
{
    QJsonObject json = m_data.toJson();
    return json.toVariantMap();
}

void AppConfig::setConfigMap(const QVariantMap &map)
{
    QJsonObject json = QJsonObject::fromVariantMap(map);
    m_data = AppConfigData::fromJson(json);
    emit configChanged();
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

    // Copy default resources (templates) if missing
    copyDefaultResources();
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
