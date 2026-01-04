#include "projectconfig.h"
#include <QFile>
#include <QJsonParseError>
#include <QDebug>

ProjectConfig ProjectConfig::fromJson(const QJsonObject &obj)
{
    ProjectConfig config;

    // API Settings
    if (obj.contains("api") && obj["api"].isObject()) {
        QJsonObject api = obj["api"].toObject();
        config.apiAccessToken = api.value("access_token").toString(config.apiAccessToken);
        config.apiModel = api.value("model").toString(config.apiModel);
        config.apiMaxTokens = api.value("max_tokens").toInt(config.apiMaxTokens);
        config.apiTemperature = api.value("temperature").toDouble(config.apiTemperature);
        config.apiTopP = api.value("top_p").toDouble(config.apiTopP);
        config.apiFrequencyPenalty = api.value("frequency_penalty").toDouble(config.apiFrequencyPenalty);
        config.apiPresencePenalty = api.value("presence_penalty").toDouble(config.apiPresencePenalty);
        config.apiStream = api.value("stream").toBool(config.apiStream);
        config.apiProprietary = api.value("proprietary").toBool(config.apiProprietary);
    }

    // Folder Settings
    if (obj.contains("folders") && obj["folders"].isObject()) {
        QJsonObject folders = obj["folders"].toObject();
        config.rootFolder = folders.value("root").toString(config.rootFolder);
        config.docsFolder = folders.value("docs").toString(config.docsFolder);
        config.srcFolder = folders.value("src").toString(config.srcFolder);
        config.sessionsFolder = folders.value("sessions").toString(config.sessionsFolder);
        config.templatesFolder = folders.value("templates").toString(config.templatesFolder);

        if (folders.contains("include_docs") && folders["include_docs"].isArray()) {
            config.includeDocFolders.clear();
            for (const QJsonValue &val : folders["include_docs"].toArray()) {
                config.includeDocFolders.append(val.toString());
            }
        }
    }

    // File Type Settings
    if (obj.contains("filetypes") && obj["filetypes"].isObject()) {
        QJsonObject filetypes = obj["filetypes"].toObject();

        if (filetypes.contains("source") && filetypes["source"].isArray()) {
            config.sourceFileTypes.clear();
            for (const QJsonValue &val : filetypes["source"].toArray()) {
                config.sourceFileTypes.append(val.toString());
            }
        }

        if (filetypes.contains("docs") && filetypes["docs"].isArray()) {
            config.docFileTypes.clear();
            for (const QJsonValue &val : filetypes["docs"].toArray()) {
                config.docFileTypes.append(val.toString());
            }
        }
    }

    // Command Pipes
    if (obj.contains("command_pipes") && obj["command_pipes"].isObject()) {
        config.commandPipes.clear();
        QJsonObject pipes = obj["command_pipes"].toObject();
        for (auto it = pipes.constBegin(); it != pipes.constEnd(); ++it) {
            if (it.value().isArray()) {
                QStringList cmdList;
                for (const QJsonValue &val : it.value().toArray()) {
                    cmdList.append(val.toString());
                }
                config.commandPipes.insert(it.key(), cmdList);
            }
        }
    }

    return config;
}

QJsonObject ProjectConfig::toJson() const
{
    QJsonObject obj;

    // API Settings
    QJsonObject api;
    api["access_token"] = apiAccessToken;
    api["model"] = apiModel;
    api["max_tokens"] = apiMaxTokens;
    api["temperature"] = apiTemperature;
    api["top_p"] = apiTopP;
    api["frequency_penalty"] = apiFrequencyPenalty;
    api["presence_penalty"] = apiPresencePenalty;
    api["stream"] = apiStream;
    api["proprietary"] = apiProprietary;
    obj["api"] = api;

    // Folder Settings
    QJsonObject folders;
    folders["root"] = rootFolder;
    folders["docs"] = docsFolder;
    folders["src"] = srcFolder;
    folders["sessions"] = sessionsFolder;
    folders["templates"] = templatesFolder;

    QJsonArray includeDocs;
    for (const QString &doc : includeDocFolders) {
        includeDocs.append(doc);
    }
    folders["include_docs"] = includeDocs;
    obj["folders"] = folders;

    // File Type Settings
    QJsonObject filetypes;

    QJsonArray source;
    for (const QString &type : sourceFileTypes) {
        source.append(type);
    }
    filetypes["source"] = source;

    QJsonArray docs;
    for (const QString &type : docFileTypes) {
        docs.append(type);
    }
    filetypes["docs"] = docs;
    obj["filetypes"] = filetypes;

    // Command Pipes
    QJsonObject pipes;
    for (auto it = commandPipes.constBegin(); it != commandPipes.constEnd(); ++it) {
        QJsonArray cmdArray;
        for (const QString &cmd : it.value()) {
            cmdArray.append(cmd);
        }
        pipes[it.key()] = cmdArray;
    }
    obj["command_pipes"] = pipes;

    return obj;
}

bool ProjectConfig::loadFromFile(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "ProjectConfig: Failed to open file for reading:" << path;
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "ProjectConfig: JSON parse error:" << parseError.errorString();
        return false;
    }

    if (!doc.isObject()) {
        qWarning() << "ProjectConfig: JSON root is not an object";
        return false;
    }

    *this = fromJson(doc.object());
    return true;
}

bool ProjectConfig::saveToFile(const QString &path) const
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "ProjectConfig: Failed to open file for writing:" << path;
        return false;
    }

    QJsonDocument doc(toJson());
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();

    return true;
}

void ProjectConfig::mergeFrom(const ProjectConfig &other)
{
    // Only override non-default values
    // For strings, if other is non-empty, use it
    if (!other.apiAccessToken.isEmpty()) apiAccessToken = other.apiAccessToken;
    if (!other.apiModel.isEmpty()) apiModel = other.apiModel;

    // For numbers, always use other's value (can't distinguish "default" from "set to default")
    apiMaxTokens = other.apiMaxTokens;
    apiTemperature = other.apiTemperature;
    apiTopP = other.apiTopP;
    apiFrequencyPenalty = other.apiFrequencyPenalty;
    apiPresencePenalty = other.apiPresencePenalty;
    apiStream = other.apiStream;
    apiProprietary = other.apiProprietary;

    if (!other.rootFolder.isEmpty()) rootFolder = other.rootFolder;
    if (!other.docsFolder.isEmpty()) docsFolder = other.docsFolder;
    if (!other.srcFolder.isEmpty()) srcFolder = other.srcFolder;
    if (!other.sessionsFolder.isEmpty()) sessionsFolder = other.sessionsFolder;
    if (!other.templatesFolder.isEmpty()) templatesFolder = other.templatesFolder;

    if (!other.includeDocFolders.isEmpty()) includeDocFolders = other.includeDocFolders;
    if (!other.sourceFileTypes.isEmpty()) sourceFileTypes = other.sourceFileTypes;
    if (!other.docFileTypes.isEmpty()) docFileTypes = other.docFileTypes;
    if (!other.commandPipes.isEmpty()) commandPipes = other.commandPipes;
}

ProjectConfig ProjectConfig::createDefault()
{
    return ProjectConfig(); // Uses default member initializers
}
