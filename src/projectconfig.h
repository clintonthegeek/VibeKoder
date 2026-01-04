#ifndef PROJECTCONFIG_H
#define PROJECTCONFIG_H

#include <QString>
#include <QStringList>
#include <QMap>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QVariant>

/**
 * @brief Configuration structure for a VibeKoder project.
 *
 * Replaces the complex ConfigManager system with a simple, type-safe struct.
 * Provides JSON serialization/deserialization for persistence.
 */
struct ProjectConfig
{
    // === API Settings ===
    QString apiAccessToken;
    QString apiModel = "gpt-4.1-mini";
    int apiMaxTokens = 20000;
    double apiTemperature = 0.3;
    double apiTopP = 1.0;
    double apiFrequencyPenalty = 0.0;
    double apiPresencePenalty = 0.0;
    bool apiStream = false;
    bool apiProprietary = true;

    // === Folder Settings ===
    QString rootFolder;
    QString docsFolder = "docs";
    QString srcFolder = ".";
    QString sessionsFolder = "sessions";
    QString templatesFolder = "templates";
    QStringList includeDocFolders = {"docs"};

    // === File Type Settings ===
    QStringList sourceFileTypes = {"*.cpp", "*.h", "CMakeLists.txt"};
    QStringList docFileTypes = {"md", "txt"};

    // === Command Pipes ===
    QMap<QString, QStringList> commandPipes = {
        {"git_diff", {"git", "diff", "."}},
        {"make_output", {"make", "build"}},
        {"execute", {"VibeKoder", "build"}}
    };

    /**
     * @brief Load configuration from a JSON object
     */
    static ProjectConfig fromJson(const QJsonObject &obj);

    /**
     * @brief Convert configuration to a JSON object
     */
    QJsonObject toJson() const;

    /**
     * @brief Load configuration from a JSON file
     * @param path File path to load from
     * @return true if successful, false otherwise
     */
    bool loadFromFile(const QString &path);

    /**
     * @brief Save configuration to a JSON file
     * @param path File path to save to
     * @return true if successful, false otherwise
     */
    bool saveToFile(const QString &path) const;

    /**
     * @brief Merge another config into this one (for applying defaults)
     * Fields in 'other' will override fields in 'this' if they differ from defaults.
     */
    void mergeFrom(const ProjectConfig &other);

    /**
     * @brief Create default configuration
     */
    static ProjectConfig createDefault();
};

#endif // PROJECTCONFIG_H
