#include "project.h"

#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QDebug>
#include <QDirIterator>

#include "toml.hpp"

Project::Project(QObject *parent)
    : QObject(parent)
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

bool Project::save(const QString & /*filepath*/)
{
    // Save not implemented yet
    return false;
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
QString Project::rootFolder() const { return m_rootFolder; }
QString Project::docsFolder() const { return m_docsFolder; }
QString Project::srcFolder() const { return m_srcFolder; }
QString Project::sessionsFolder() const { return m_sessionsFolder; }
QString Project::templatesFolder() const { return m_templatesFolder; }
QStringList Project::includeDocFolders() const { return m_includeDocFolders; }

QStringList Project::sourceFileTypes() const { return m_sourceFileTypes; }
QStringList Project::docFileTypes() const { return m_docFileTypes; }
QMap<QString, QStringList> Project::commandPipes() const { return m_commandPipes; }

QString Project::accessToken() const { return m_accessToken; }
QString Project::model() const { return m_model; }
int Project::maxTokens() const { return m_maxTokens; }
double Project::temperature() const { return m_temperature; }
double Project::topP() const { return m_topP; }
double Project::frequencyPenalty() const { return m_frequencyPenalty; }
double Project::presencePenalty() const { return m_presencePenalty; }
