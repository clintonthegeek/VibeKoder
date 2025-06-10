#include "appconfig.h"
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QCoreApplication>

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
    QFile file(m_configFilePath);
    if (!file.exists()) {
        qDebug() << "Config file does not exist, creating default:" << m_configFilePath;
        createDefaultConfigFile();
        copyDefaultDocsAndTemplates();
        return true;
    }
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Failed to open config file for reading:" << m_configFilePath;
        return false;
    }
    QByteArray data = file.readAll();
    file.close();

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError) {
        qWarning() << "Failed to parse config JSON:" << err.errorString();
        return false;
    }
    if (!doc.isObject()) {
        qWarning() << "Config JSON root is not an object";
        return false;
    }
    QJsonObject root = doc.object();

    // Parse default_project_settings section
    if (root.contains("default_project_settings") && root["default_project_settings"].isObject()) {
        parseDefaultProjectSettings(root["default_project_settings"].toObject());
    } else {
        qWarning() << "No default_project_settings section found in config";
    }

    // Parse app_settings section
    if (root.contains("app_settings") && root["app_settings"].isObject()) {
        parseAppSettings(root["app_settings"].toObject());
    } else {
        qWarning() << "No app_settings section found in config";
    }

    return true;
}

void AppConfig::parseDefaultProjectSettings(const QJsonObject& obj)
{
    // Parse API settings
    if (obj.contains("api") && obj["api"].isObject()) {
        QJsonObject apiObj = obj["api"].toObject();
        m_apiSettings.clear();
        for (auto it = apiObj.begin(); it != apiObj.end(); ++it) {
            m_apiSettings[it.key()] = it.value().toVariant();
        }
    }

    // Parse folders
    if (obj.contains("folders") && obj["folders"].isObject()) {
        QJsonObject foldersObj = obj["folders"].toObject();
        m_folders.clear();
        for (auto it = foldersObj.begin(); it != foldersObj.end(); ++it) {
            m_folders[it.key()] = it.value().toVariant();
        }
    }

    // Parse filetypes
    if (obj.contains("filetypes") && obj["filetypes"].isObject()) {
        QJsonObject ftObj = obj["filetypes"].toObject();
        m_sourceFileTypes.clear();
        m_docFileTypes.clear();

        if (ftObj.contains("source") && ftObj["source"].isArray()) {
            for (const auto& val : ftObj["source"].toArray()) {
                if (val.isString())
                    m_sourceFileTypes.append(val.toString());
            }
        }
        if (ftObj.contains("docs") && ftObj["docs"].isArray()) {
            for (const auto& val : ftObj["docs"].toArray()) {
                if (val.isString())
                    m_docFileTypes.append(val.toString());
            }
        }
    }

    // Parse command pipes
    if (obj.contains("command_pipes") && obj["command_pipes"].isObject()) {
        QJsonObject cpObj = obj["command_pipes"].toObject();
        m_commandPipes.clear();
        for (auto it = cpObj.begin(); it != cpObj.end(); ++it) {
            if (it.value().isArray()) {
                QStringList cmds;
                for (const auto& val : it.value().toArray()) {
                    if (val.isString())
                        cmds.append(val.toString());
                }
                m_commandPipes[it.key()] = cmds;
            }
        }
    }
}

void AppConfig::parseAppSettings(const QJsonObject& obj)
{
    m_appSettings.clear();
    for (auto it = obj.begin(); it != obj.end(); ++it) {
        m_appSettings[it.key()] = it.value().toVariant();
    }
}

bool AppConfig::save() const
{
    QJsonObject rootObj;

    // default_project_settings section
    QJsonObject defaultProjObj;

    // API settings
    QJsonObject apiObj;
    for (auto it = m_apiSettings.begin(); it != m_apiSettings.end(); ++it) {
        apiObj[it.key()] = QJsonValue::fromVariant(it.value());
    }
    defaultProjObj["api"] = apiObj;

    // Folders
    QJsonObject foldersObj;
    for (auto it = m_folders.begin(); it != m_folders.end(); ++it) {
        foldersObj[it.key()] = QJsonValue::fromVariant(it.value());
    }
    defaultProjObj["folders"] = foldersObj;

    // Filetypes
    QJsonObject filetypesObj;
    QJsonArray sourceArr;
    for (const QString &s : m_sourceFileTypes)
        sourceArr.append(s);
    filetypesObj["source"] = sourceArr;

    QJsonArray docsArr;
    for (const QString &s : m_docFileTypes)
        docsArr.append(s);
    filetypesObj["docs"] = docsArr;

    defaultProjObj["filetypes"] = filetypesObj;

    // Command pipes
    QJsonObject cmdPipesObj;
    for (auto it = m_commandPipes.begin(); it != m_commandPipes.end(); ++it) {
        QJsonArray cmdsArr;
        for (const QString &cmd : it.value())
            cmdsArr.append(cmd);
        cmdPipesObj[it.key()] = cmdsArr;
    }
    defaultProjObj["command_pipes"] = cmdPipesObj;

    rootObj["default_project_settings"] = defaultProjObj;

    // app_settings section
    QJsonObject appSettingsObj;
    for (auto it = m_appSettings.begin(); it != m_appSettings.end(); ++it) {
        appSettingsObj[it.key()] = QJsonValue::fromVariant(it.value());
    }
    rootObj["app_settings"] = appSettingsObj;

    // Write JSON to file
    QJsonDocument doc(rootObj);
    QFile file(m_configFilePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Failed to open config file for writing:" << m_configFilePath;
        return false;
    }
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
    qDebug() << "AppConfig saved to" << m_configFilePath;
    return true;
}

void AppConfig::createDefaultConfigFile()
{
    // Set sane defaults for default_project_settings
    m_apiSettings = {
        {"access_token", ""},
        {"model", "gpt-4.1-mini"},
        {"max_tokens", 20000},
        {"temperature", 0.3},
        {"top_p", 1.0},
        {"frequency_penalty", 0.0},
        {"presence_penalty", 0.0},
        {"stream", false},
        {"stop_sequences", QVariantList()},
        {"user", ""},
        {"logit_bias", QVariantMap()}
    };

    m_folders = {
        {"root", QDir::homePath() + "/VibeKoderProjects"},
        {"docs", "docs"},
        {"src", "."},
        {"sessions", "sessions"},
        {"templates", "templates"},
        {"include_docs", "docs"}
    };

    m_sourceFileTypes = QStringList() << "*.cpp" << "*.h";
    m_docFileTypes = QStringList() << "md" << "txt";

    m_commandPipes = {
        {"git_diff", {"git diff", "."}},
        {"make_output", {"make", "build"}},
        {"execute", {"VibeKoder", "build"}}
    };

    // Set sane defaults for app_settings
    m_appSettings = {
        {"timezone", "UTC"}
    };

    // Save immediately
    save();
}

void AppConfig::copyDefaultDocsAndTemplates()
{
    // Copy default docs and templates from a known folder inside app installation directory
    // For example, assume app installs defaults under:
    // <app_install_dir>/defaults/docs and <app_install_dir>/defaults/templates
    // We copy them to m_configFolder/docs and m_configFolder/templates if those folders don't exist.

    // Determine app install directory (example using QCoreApplication)
    QString appDir = QCoreApplication::applicationDirPath();

    QString defaultDocsSrc = QDir(appDir).filePath("defaults/docs");
    QString defaultTemplatesSrc = QDir(appDir).filePath("defaults/templates");

    QString docsDest = QDir(m_configFolder).filePath("docs");
    QString templatesDest = QDir(m_configFolder).filePath("templates");

    QDir destDir;

    if (!QDir(docsDest).exists() && QDir(defaultDocsSrc).exists()) {
        destDir.mkpath(docsDest);
        QDir srcDir(defaultDocsSrc);
        for (const QString& fileName : srcDir.entryList(QDir::Files)) {
            QFile::copy(srcDir.filePath(fileName), QDir(docsDest).filePath(fileName));
        }
    }

    if (!QDir(templatesDest).exists() && QDir(defaultTemplatesSrc).exists()) {
        destDir.mkpath(templatesDest);
        QDir srcDir(defaultTemplatesSrc);
        for (const QString& fileName : srcDir.entryList(QDir::Files)) {
            QFile::copy(srcDir.filePath(fileName), QDir(templatesDest).filePath(fileName));
        }
    }
}

// Getters and setters

QVariantMap AppConfig::defaultApiSettings() const { return m_apiSettings; }
QVariantMap AppConfig::defaultFolders() const { return m_folders; }
QStringList AppConfig::defaultSourceFileTypes() const { return m_sourceFileTypes; }
QStringList AppConfig::defaultDocFileTypes() const { return m_docFileTypes; }
QMap<QString, QStringList> AppConfig::defaultCommandPipes() const { return m_commandPipes; }

void AppConfig::setDefaultApiSettings(const QVariantMap& apiSettings) { m_apiSettings = apiSettings; }
void AppConfig::setDefaultFolders(const QVariantMap& folders) { m_folders = folders; }
void AppConfig::setDefaultSourceFileTypes(const QStringList& types) { m_sourceFileTypes = types; }
void AppConfig::setDefaultDocFileTypes(const QStringList& types) { m_docFileTypes = types; }
void AppConfig::setDefaultCommandPipes(const QMap<QString, QStringList>& pipes) { m_commandPipes = pipes; }

QString AppConfig::configFolder() const { return m_configFolder; }
QString AppConfig::configFilePath() const { return m_configFilePath; }
