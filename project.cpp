#include "project.h"

#include <QFile>
#include <fstream>  // For std::ofstream
#include <QDir>
#include <QFileInfo>
#include <QDebug>
#include <QDirIterator>

#include "toml.hpp"

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

    const QString content = QString::fromUtf8(file.readAll());

    bool ok = parseToml(content, filepath);
    if (ok)
        qDebug() << "Project loaded with root folder:" << m_rootFolder;
    return ok;
}

bool Project::save(const QString &filepath)
{
    QString savePath = filepath.isEmpty() ? m_projectFilePath : filepath;
    if (savePath.isEmpty()) {
        qWarning() << "No project file path specified for saving.";
        return false;
    }

    // try {
    //     toml::table root;

    //     // [api] section
    //     toml::table apiTable;
    //     apiTable.insert("access_token", m_accessToken);
    //     apiTable.insert("model", m_model);
    //     apiTable.insert("max_tokens", m_maxTokens);
    //     apiTable.insert("temperature", m_temperature);
    //     apiTable.insert("top_p", m_topP);
    //     apiTable.insert("frequency_penalty", m_frequencyPenalty);
    //     apiTable.insert("presence_penalty", m_presencePenalty);
    //     root.insert("api", apiTable);

    //     // [folders] section
    //     toml::table foldersTable;
    //     foldersTable.insert("root", m_rootFolder);
    //     foldersTable.insert("docs", m_docsFolder);
    //     foldersTable.insert("src", m_srcFolder);
    //     foldersTable.insert("sessions", m_sessionsFolder);
    //     foldersTable.insert("templates", m_templatesFolder);

    //     // include_docs as array
    //     toml::array includeDocsArray;
    //     for (const QString &folder : m_includeDocFolders) {
    //         includeDocsArray.push_back(folder.toStdString());
    //     }
    //     foldersTable.insert("include_docs", includeDocsArray);

    //     root.insert("folders", foldersTable);

    //     // [filetypes] section
    //     toml::table filetypesTable;
    //     toml::array sourceArray;
    //     for (const QString &ext : m_sourceFileTypes) {
    //         sourceArray.push_back(ext.toStdString());
    //     }
    //     filetypesTable.insert("source", sourceArray);

    //     toml::array docsArray;
    //     for (const QString &ext : m_docFileTypes) {
    //         docsArray.push_back(ext.toStdString());
    //     }
    //     filetypesTable.insert("docs", docsArray);

    //     root.insert("filetypes", filetypesTable);

    //     // [command_pipes] section
    //     toml::table commandPipesTable;
    //     for (auto it = m_commandPipes.constBegin(); it != m_commandPipes.constEnd(); ++it) {
    //         toml::array cmdArray;
    //         for (const QString &cmd : it.value()) {
    //             cmdArray.push_back(cmd.toStdString());
    //         }
    //         commandPipesTable.insert(it.key().toStdString(), cmdArray);
    //     }
    //     root.insert("command_pipes", commandPipesTable);

    //     // Write to file
    //     std::ofstream ofs(savePath.toStdString());
    //     if (!ofs.is_open()) {
    //         qWarning() << "Failed to open project file for writing:" << savePath;
    //         return false;
    //     }
    //     ofs << root;
    //     ofs.close();

    //     m_projectFilePath = savePath; // Update current project file path if needed

    //     qDebug() << "Project saved to" << savePath;
    //     return true;
    // } catch (const std::exception &ex) {
    //     qWarning() << "Exception saving project file:" << ex.what();
    //     return false;
    // }
}

bool Project::parseToml(const QString &content, const QString &projectFilePath)
{
    try {
        toml::table root = toml::parse(content.toStdString());

        // **** Determine m_rootFolder ****
        // If [folders] root="." then set root as folder containing the project file
        if (auto foldersNode = root.get("folders")) {
            if (auto foldersTable = foldersNode->as_table()) {
                if (auto rootNode = foldersTable->get("root")) {
                    if (auto valStr = rootNode->as_string()) {
                        QString rootPath = QString::fromStdString(valStr->get()).trimmed();
                        if (rootPath == ".") {
                            QFileInfo fi(projectFilePath);
                            m_rootFolder = fi.absolutePath();
                        } else {
                            m_rootFolder = QDir::cleanPath(QDir(m_rootFolder).filePath(rootPath));
                        }
                    }
                } else {
                    // Default root folder - directory of project file
                    QFileInfo fi(projectFilePath);
                    m_rootFolder = fi.absolutePath();
                }

                // Parse subfolders relative to root
                auto getSubfolder = [&](const char* name, QString &memberVar) {
                    if (auto node = foldersTable->get(name)) {
                        if (auto strVal = node->as_string()) {
                            memberVar = QDir(m_rootFolder).filePath(QString::fromStdString(strVal->get()).trimmed());
                        }
                    }
                };
                getSubfolder("docs", m_docsFolder);
                getSubfolder("src", m_srcFolder);
                getSubfolder("sessions", m_sessionsFolder);
                getSubfolder("templates", m_templatesFolder);

                // include_docs as array or single string with ~ expansion
                if (auto inclNode = foldersTable->get("include_docs")) {
                    m_includeDocFolders.clear();
                    if (inclNode->is_array()) {
                        for (const auto& el : *inclNode->as_array()) {
                            if (auto strVal = el.as_string()) {
                                QString folder = QString::fromStdString(strVal->get()).trimmed();
                                if (folder.startsWith("~")) {
                                    folder.replace(0, 1, QDir::homePath());
                                }
                                if (QDir::isAbsolutePath(folder))
                                    m_includeDocFolders.append(QDir::cleanPath(folder));
                                else
                                    m_includeDocFolders.append(QDir(m_rootFolder).filePath(folder));
                            }
                        }
                    } else if (auto strVal = inclNode->as_string()) {
                        QString folder = QString::fromStdString(strVal->get()).trimmed();
                        if (folder.startsWith("~")) {
                            folder.replace(0, 1, QDir::homePath());
                        }
                        if (QDir::isAbsolutePath(folder))
                            m_includeDocFolders.append(QDir::cleanPath(folder));
                        else
                            m_includeDocFolders.append(QDir(m_rootFolder).filePath(folder));
                    }
                }
            }
        }

        // **** Filetypes ****
        if (auto ftNode = root.get("filetypes")) {
            if (auto ftTable = ftNode->as_table()) {
                auto readStrList = [](const toml::node* node) -> QStringList {
                    QStringList list;
                    if (!node) return list;
                    if (node->is_array()) {
                        for (const auto& el : *node->as_array()) {
                            if (auto strVal = el.as_string()) {
                                list.append(QString::fromStdString(strVal->get()));
                            }
                        }
                    } else if (auto strVal = node->as_string()) {
                        list.append(QString::fromStdString(strVal->get()));
                    }
                    return list;
                };

                m_sourceFileTypes = readStrList(ftTable->get("source"));
                m_docFileTypes = readStrList(ftTable->get("docs"));
            }
        }

        // **** Command Pipes ****
        if (auto cmdNode = root.get("command_pipes")) {
            if (auto cmdTable = cmdNode->as_table()) {
                m_commandPipes.clear();
                for (const auto& [keyObj, valNode] : *cmdTable) {
                    QString key = QString::fromStdString(std::string(keyObj.str()));
                    QStringList cmdList;
                    if (auto arrPtr = valNode.as_array()) {
                        for (const auto& el : *arrPtr) {
                            if (auto strVal = el.as_string()) {
                                cmdList << QString::fromStdString(strVal->get());
                            }
                        }
                    } else if (auto strVal = valNode.as_string()) {
                        cmdList << QString::fromStdString(strVal->get());
                    }
                    m_commandPipes.insert(key, cmdList);
                }
            }
        }

        // **** API section ****
        if (auto apiNode = root.get("api")) {
            if (auto apiTable = apiNode->as_table()) {
                auto readStr = [&](const char* name) -> QString {
                    if (auto node = apiTable->get(name)) {
                        if (auto strVal = node->as_string())
                            return QString::fromStdString(strVal->get());
                    }
                    return QString();
                };

                m_accessToken = readStr("access_token");
                m_model = readStr("model");
                if (m_model.isEmpty())
                    m_model = "gpt-4.1-mini";

                auto readInt = [&](const char* name, int def) {
                    if (auto node = apiTable->get(name)) {
                        if (auto intVal = node->as_integer())
                            return static_cast<int>(intVal->get());
                    }
                    return def;
                };

                m_maxTokens = readInt("max_tokens", 800);

                auto readDouble = [&](const char* name, double def) {
                    if (auto node = apiTable->get(name)) {
                        if (auto dblVal = node->as_floating_point())
                            return dblVal->get();
                    }
                    return def;
                };

                m_temperature = readDouble("temperature", 0.3);
                m_topP = readDouble("top_p", 1.0);
                m_frequencyPenalty = readDouble("frequency_penalty", 0.0);
                m_presencePenalty = readDouble("presence_penalty", 0.0);
            }
        }

        return true;
    } catch (const std::exception& ex) {
        qWarning() << "Exception during TOML parse:" << ex.what();
        return false;
    }
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
