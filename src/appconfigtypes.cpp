#include "appconfigtypes.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QDebug>

AppConfigData AppConfigData::fromJson(const QJsonObject &obj)
{
    AppConfigData data;

    // Load default project settings
    if (obj.contains("default_project_settings") && obj["default_project_settings"].isObject()) {
        data.defaultProjectSettings = ProjectConfig::fromJson(obj["default_project_settings"].toObject());
    }

    // Load app settings
    if (obj.contains("app_settings") && obj["app_settings"].isObject()) {
        QJsonObject appSettings = obj["app_settings"].toObject();
        data.timezone = appSettings.value("timezone").toString(data.timezone);
    }

    return data;
}

QJsonObject AppConfigData::toJson() const
{
    QJsonObject obj;

    // Save default project settings
    obj["default_project_settings"] = defaultProjectSettings.toJson();

    // Save app settings
    QJsonObject appSettings;
    appSettings["timezone"] = timezone;
    obj["app_settings"] = appSettings;

    return obj;
}

bool AppConfigData::loadFromFile(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "AppConfigData: Failed to open file for reading:" << path;
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "AppConfigData: JSON parse error:" << parseError.errorString();
        return false;
    }

    if (!doc.isObject()) {
        qWarning() << "AppConfigData: JSON root is not an object";
        return false;
    }

    *this = fromJson(doc.object());
    return true;
}

bool AppConfigData::saveToFile(const QString &path) const
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "AppConfigData: Failed to open file for writing:" << path;
        return false;
    }

    QJsonDocument doc(toJson());
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();

    return true;
}

AppConfigData AppConfigData::createDefault()
{
    AppConfigData data;
    data.defaultProjectSettings = ProjectConfig::createDefault();
    data.timezone = "UTC";
    return data;
}
