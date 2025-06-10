#include "project.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QDir>
#include <QFileInfo>
#include <QDebug>
#include <QDirIterator>


Project::Project(QObject *parent,
                 AppConfig* appConfig)
    : QObject(parent)
    , m_appConfig(appConfig ? appConfig : &AppConfig::instance())
    , m_rootFolder()
    , m_docsFolder()
    , m_srcFolder()
    , m_sessionsFolder()
    , m_templatesFolder()
    , m_accessToken()
    , m_model("gpt-4.1-mini")
    , m_maxTokens(800)
    , m_temperature(0.3)
    , m_topP(1.0)
    , m_frequencyPenalty(0.0)
    , m_presencePenalty(0.0)
{
}

bool Project::load(const QString &filepath)
{
    QFile file(filepath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Could not open project file:" << filepath;
        return false;
    }

    m_projectFilePath = filepath;

    QByteArray data = file.readAll();
    file.close();

    return parseJson(data);
}

bool Project::save(const QString &filepath)
{
    QString savePath = filepath.isEmpty() ? m_projectFilePath : filepath;
    if (savePath.isEmpty()) {
        qWarning() << "No project file path specified for saving.";
        return false;
    }
    return true;
}

QString Project::resolveFolderPath(const QString& folder) const
{
    if (folder.isEmpty())
        return QString();

    QFileInfo fi(folder);
    if (fi.isAbsolute())
        return QDir::cleanPath(folder);

    // Relative path: prepend root folder
    return QDir(m_rootFolder).filePath(folder);
}

bool Project::parseJson(const QByteArray &data)
{
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "JSON parse error in project file:" << parseError.errorString();
        return false;
    }
    if (!doc.isObject()) {
        qWarning() << "Project file root is not a JSON object";
        return false;
    }

    QJsonObject root = doc.object();

    // Parse folders
    if (root.contains("folders") && root["folders"].isObject()) {
        QJsonObject foldersObj = root["folders"].toObject();

        auto getString = [&](const QString &key) -> QString {
            return foldersObj.contains(key) && foldersObj[key].isString() ? foldersObj[key].toString() : QString();
        };

        m_rootFolder = getString("root");
        m_docsFolder = getString("docs");
        m_srcFolder = getString("src");
        m_sessionsFolder = getString("sessions");
        m_templatesFolder = getString("templates");

        // include_docs can be string or array
        if (foldersObj.contains("include_docs")) {
            if (foldersObj["include_docs"].isString()) {
                m_includeDocFolders = QStringList() << foldersObj["include_docs"].toString();
            } else if (foldersObj["include_docs"].isArray()) {
                m_includeDocFolders.clear();
                for (const QJsonValue &val : foldersObj["include_docs"].toArray()) {
                    if (val.isString())
                        m_includeDocFolders.append(val.toString());
                }
            }
        }

        //set relative paths for folders
        m_docsFolder = resolveFolderPath(m_docsFolder);
        m_srcFolder = resolveFolderPath(m_srcFolder);
        m_sessionsFolder = resolveFolderPath(m_sessionsFolder);
        m_templatesFolder = resolveFolderPath(m_templatesFolder);

        // For includeDocFolders, resolve each entry similarly:
        QStringList resolvedIncludeDocs;
        for (const QString& incFolder : m_includeDocFolders) {
            resolvedIncludeDocs.append(resolveFolderPath(incFolder));
        }
        m_includeDocFolders = resolvedIncludeDocs;
    }

    // Parse filetypes
    if (root.contains("filetypes") && root["filetypes"].isObject()) {
        QJsonObject ftObj = root["filetypes"].toObject();

        auto readStringList = [&](const QString &key) -> QStringList {
            QStringList list;
            if (ftObj.contains(key) && ftObj[key].isArray()) {
                for (const QJsonValue &val : ftObj[key].toArray()) {
                    if (val.isString())
                        list.append(val.toString());
                }
            }
            return list;
        };

        m_sourceFileTypes = readStringList("source");
        m_docFileTypes = readStringList("docs");
    }

    // Parse command pipes
    if (root.contains("command_pipes") && root["command_pipes"].isObject()) {
        QJsonObject cpObj = root["command_pipes"].toObject();
        m_commandPipes.clear();
        for (const QString &key : cpObj.keys()) {
            if (cpObj[key].isArray()) {
                QStringList cmds;
                for (const QJsonValue &val : cpObj[key].toArray()) {
                    if (val.isString())
                        cmds.append(val.toString());
                }
                m_commandPipes.insert(key, cmds);
            }
        }
    }

    // Parse API section
    if (root.contains("api") && root["api"].isObject()) {
        QJsonObject apiObj = root["api"].toObject();

        auto getString = [&](const QString &key) -> QString {
            return apiObj.contains(key) && apiObj[key].isString() ? apiObj[key].toString() : QString();
        };

        auto getInt = [&](const QString &key, int def) -> int {
            return apiObj.contains(key) && apiObj[key].isDouble() ? apiObj[key].toInt(def) : def;
        };

        auto getDouble = [&](const QString &key, double def) -> double {
            return apiObj.contains(key) && apiObj[key].isDouble() ? apiObj[key].toDouble(def) : def;
        };

        m_accessToken = getString("access_token");
        m_model = getString("model");
        if (m_model.isEmpty())
            m_model = "gpt-4.1-mini";

        m_maxTokens = getInt("max_tokens", 800);
        m_temperature = getDouble("temperature", 0.3);
        m_topP = getDouble("top_p", 1.0);
        m_frequencyPenalty = getDouble("frequency_penalty", 0.0);
        m_presencePenalty = getDouble("presence_penalty", 0.0);
    }

    return true;
}

// Expand doc files in all includeDocFolders recursively and return matched file paths
QStringList Project::scanDocsRecursive() const
{
    QStringList results;
    for (const QString &folder : qAsConst(m_includeDocFolders)) {
        QDir dir(folder);
        if (!dir.exists())
            continue;

        // Recursive directory iterator
        QDirIterator it(dir.absolutePath(), m_docFileTypes, QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            results << it.next();
        }
    }
    return results;
}

// Expand source files recursively in m_srcFolder and return matched file paths
QStringList Project::scanSourceRecursive() const
{
    QStringList results;
    QDir dir(m_srcFolder);
    if (!dir.exists())
        return results;

    QDirIterator it(dir.absolutePath(), m_sourceFileTypes, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        results << it.next();
    }
    return results;
}

// Getters for folders and filetypes
QString Project::rootFolder() const
{
    if (!m_rootFolder.isEmpty())
        return m_rootFolder;
    if (m_appConfig)
        return m_appConfig->defaultFolders().value("root").toString();
    return QString();
}

QString Project::docsFolder() const
{
    if (!m_docsFolder.isEmpty())
        return m_docsFolder;
    if (m_appConfig)
        return m_appConfig->defaultFolders().value("docs").toString();
    return QString();
}

QString Project::srcFolder() const
{
    if (!m_srcFolder.isEmpty())
        return m_srcFolder;
    if (m_appConfig)
        return m_appConfig->defaultFolders().value("src").toString();
    return QString();
}

QString Project::sessionsFolder() const
{
    if (!m_sessionsFolder.isEmpty())
        return m_sessionsFolder;
    if (m_appConfig)
        return m_appConfig->defaultFolders().value("sessions").toString();
    return QString();
}

QString Project::templatesFolder() const
{
    if (!m_templatesFolder.isEmpty())
        return m_templatesFolder;
    if (m_appConfig)
        return m_appConfig->defaultFolders().value("templates").toString();
    return QString();
}

QStringList Project::includeDocFolders() const
{
    if (!m_includeDocFolders.isEmpty())
        return m_includeDocFolders;
    if (m_appConfig) {
        QString inc = m_appConfig->defaultFolders().value("include_docs").toString();
        if (!inc.isEmpty())
            return QStringList{inc};
    }
    return QStringList();
}

QStringList Project::sourceFileTypes() const
{
    if (!m_sourceFileTypes.isEmpty())
        return m_sourceFileTypes;
    if (m_appConfig)
        return m_appConfig->defaultSourceFileTypes();
    return QStringList();
}

QStringList Project::docFileTypes() const
{
    if (!m_docFileTypes.isEmpty())
        return m_docFileTypes;
    if (m_appConfig)
        return m_appConfig->defaultDocFileTypes();
    return QStringList();
}

QMap<QString, QStringList> Project::commandPipes() const
{
    if (!m_commandPipes.isEmpty())
        return m_commandPipes;
    if (m_appConfig)
        return m_appConfig->defaultCommandPipes();
    return QMap<QString, QStringList>();
}

// API getters with fallback

QString Project::accessToken() const
{
    if (!m_accessToken.isEmpty())
        return m_accessToken;
    if (m_appConfig)
        return m_appConfig->defaultApiSettings().value("access_token").toString();
    return QString();
}

QString Project::model() const
{
    if (!m_model.isEmpty())
        return m_model;
    if (m_appConfig)
        return m_appConfig->defaultApiSettings().value("model").toString();
    return QString("gpt-4.1-mini");
}

int Project::maxTokens() const
{
    if (m_maxTokens > 0)
        return m_maxTokens;
    if (m_appConfig)
        return m_appConfig->defaultApiSettings().value("max_tokens", 800).toInt();
    return 800;
}

double Project::temperature() const
{
    if (m_temperature >= 0.0)
        return m_temperature;
    if (m_appConfig)
        return m_appConfig->defaultApiSettings().value("temperature", 0.3).toDouble();
    return 0.3;
}

double Project::topP() const
{
    if (m_topP >= 0.0)
        return m_topP;
    if (m_appConfig)
        return m_appConfig->defaultApiSettings().value("top_p", 1.0).toDouble();
    return 1.0;
}

double Project::frequencyPenalty() const
{
    if (m_frequencyPenalty >= 0.0)
        return m_frequencyPenalty;
    if (m_appConfig)
        return m_appConfig->defaultApiSettings().value("frequency_penalty", 0.0).toDouble();
    return 0.0;
}

double Project::presencePenalty() const
{
    if (m_presencePenalty >= 0.0)
        return m_presencePenalty;
    if (m_appConfig)
        return m_appConfig->defaultApiSettings().value("presence_penalty", 0.0).toDouble();
    return 0.0;
}

// Optional setters (if you want to allow setting these values)

void Project::setRootFolder(const QString &folder) { m_rootFolder = folder; }
void Project::setDocsFolder(const QString &folder) { m_docsFolder = folder; }
void Project::setSrcFolder(const QString &folder) { m_srcFolder = folder; }
void Project::setSessionsFolder(const QString &folder) { m_sessionsFolder = folder; }
void Project::setTemplatesFolder(const QString &folder) { m_templatesFolder = folder; }
void Project::setIncludeDocFolders(const QStringList &folders) { m_includeDocFolders = folders; }

void Project::setSourceFileTypes(const QStringList &types) { m_sourceFileTypes = types; }
void Project::setDocFileTypes(const QStringList &types) { m_docFileTypes = types; }
void Project::setCommandPipes(const QMap<QString, QStringList> &pipes) { m_commandPipes = pipes; }

void Project::setAccessToken(const QString &token) { m_accessToken = token; }
void Project::setModel(const QString &model) { m_model = model; }
void Project::setMaxTokens(int maxTokens) { m_maxTokens = maxTokens; }
void Project::setTemperature(double temperature) { m_temperature = temperature; }
void Project::setTopP(double topP) { m_topP = topP; }
void Project::setFrequencyPenalty(double penalty) { m_frequencyPenalty = penalty; }
void Project::setPresencePenalty(double penalty) { m_presencePenalty = penalty; }
