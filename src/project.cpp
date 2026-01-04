#include "project.h"
#include "appconfig.h"
#include <QFileInfo>
#include <QDir>
#include <QDebug>

Project::Project(QObject *parent)
    : QObject(parent)
{
}

Project::~Project()
{
}

bool Project::load(const QString &filepath)
{
    m_projectFilePath = filepath;

    if (!m_config.loadFromFile(filepath)) {
        qWarning() << "Failed to load project config from" << filepath;
        return false;
    }

    return true;
}

bool Project::save(const QString &filepath)
{
    QString savePath = filepath.isEmpty() ? m_projectFilePath : filepath;
    if (savePath.isEmpty()) {
        qWarning() << "No file path specified for saving project";
        return false;
    }

    m_projectFilePath = savePath;

    if (!m_config.saveToFile(savePath)) {
        qWarning() << "Failed to save project config to" << savePath;
        return false;
    }

    return true;
}

// DEPRECATED compatibility wrappers - to be removed in Phase 5
QVariant Project::getValue(const QString &keyPath, const QVariant &defaultValue) const
{
    // Parse dot-separated path and return corresponding value
    if (keyPath == "api.access_token") return m_config.apiAccessToken;
    if (keyPath == "api.model") return m_config.apiModel;
    if (keyPath == "api.max_tokens") return m_config.apiMaxTokens;
    if (keyPath == "api.temperature") return m_config.apiTemperature;
    if (keyPath == "api.top_p") return m_config.apiTopP;
    if (keyPath == "api.frequency_penalty") return m_config.apiFrequencyPenalty;
    if (keyPath == "api.presence_penalty") return m_config.apiPresencePenalty;

    if (keyPath == "folders.root") return m_config.rootFolder;
    if (keyPath == "folders.docs") return m_config.docsFolder;
    if (keyPath == "folders.src") return m_config.srcFolder;
    if (keyPath == "folders.sessions") return m_config.sessionsFolder;
    if (keyPath == "folders.templates") return m_config.templatesFolder;
    if (keyPath == "folders.include_docs") return m_config.includeDocFolders;

    if (keyPath == "filetypes.source") return m_config.sourceFileTypes;
    if (keyPath == "filetypes.docs") return m_config.docFileTypes;

    if (keyPath == "command_pipes") {
        // Convert QMap to QVariantMap
        QVariantMap result;
        for (auto it = m_config.commandPipes.constBegin(); it != m_config.commandPipes.constEnd(); ++it) {
            result.insert(it.key(), it.value());
        }
        return result;
    }

    qWarning() << "Project::getValue: Unknown key path:" << keyPath;
    return defaultValue;
}

void Project::setValue(const QString &keyPath, const QVariant &value)
{
    // Parse dot-separated path and set corresponding value
    if (keyPath == "api.access_token") { m_config.apiAccessToken = value.toString(); return; }
    if (keyPath == "api.model") { m_config.apiModel = value.toString(); return; }
    if (keyPath == "api.max_tokens") { m_config.apiMaxTokens = value.toInt(); return; }
    if (keyPath == "api.temperature") { m_config.apiTemperature = value.toDouble(); return; }
    if (keyPath == "api.top_p") { m_config.apiTopP = value.toDouble(); return; }
    if (keyPath == "api.frequency_penalty") { m_config.apiFrequencyPenalty = value.toDouble(); return; }
    if (keyPath == "api.presence_penalty") { m_config.apiPresencePenalty = value.toDouble(); return; }

    if (keyPath == "folders.root") { m_config.rootFolder = value.toString(); return; }
    if (keyPath == "folders.docs") { m_config.docsFolder = value.toString(); return; }
    if (keyPath == "folders.src") { m_config.srcFolder = value.toString(); return; }
    if (keyPath == "folders.sessions") { m_config.sessionsFolder = value.toString(); return; }
    if (keyPath == "folders.templates") { m_config.templatesFolder = value.toString(); return; }
    if (keyPath == "folders.include_docs") { m_config.includeDocFolders = value.toStringList(); return; }

    if (keyPath == "filetypes.source") { m_config.sourceFileTypes = value.toStringList(); return; }
    if (keyPath == "filetypes.docs") { m_config.docFileTypes = value.toStringList(); return; }

    if (keyPath == "command_pipes") {
        // Convert QVariantMap to QMap
        QVariantMap varMap = value.toMap();
        m_config.commandPipes.clear();
        for (auto it = varMap.constBegin(); it != varMap.constEnd(); ++it) {
            m_config.commandPipes.insert(it.key(), it.value().toStringList());
        }
        return;
    }

    qWarning() << "Project::setValue: Unknown key path:" << keyPath;
}
