#ifndef APPCONFIGTYPES_H
#define APPCONFIGTYPES_H

#include "projectconfig.h"
#include <QString>
#include <QJsonObject>

/**
 * @brief Application-wide configuration structure.
 *
 * Contains default project settings and app-specific settings.
 */
struct AppConfigData
{
    // Default settings for new projects
    ProjectConfig defaultProjectSettings;

    // App-specific settings
    QString timezone = "UTC";

    /**
     * @brief Load from JSON object
     */
    static AppConfigData fromJson(const QJsonObject &obj);

    /**
     * @brief Convert to JSON object
     */
    QJsonObject toJson() const;

    /**
     * @brief Load from file
     */
    bool loadFromFile(const QString &path);

    /**
     * @brief Save to file
     */
    bool saveToFile(const QString &path) const;

    /**
     * @brief Create default app config
     */
    static AppConfigData createDefault();
};

#endif // APPCONFIGTYPES_H
