#include "project.h"
#include "appconfig.h"  // For schema path or config folder
#include <QFileInfo>
#include <QDir>
#include <QDebug>

Project::Project(QObject *parent)
    : QObject(parent)
{
}

Project::~Project()
{
    if (m_configManager) {
        delete m_configManager;
        m_configManager = nullptr;
    }
}

bool Project::load(const QString &filepath)
{
    if (m_configManager) {
        delete m_configManager;
        m_configManager = nullptr;
    }

    m_projectFilePath = filepath;

    // Determine schema path - here we get it from AppConfig's config folder
    QString schemaPath = schemaFilePath();

    m_configManager = new ConfigManager(filepath, schemaPath, this);

    bool ok = m_configManager->load();
    if (!ok) {
        qWarning() << "Failed to load project config via ConfigManager";
        return false;
    }

    if (!m_configManager->validate()) {
        qWarning() << "Project config validation failed";
        // You may choose to handle validation failure here (e.g., reset to defaults)
    }

    return true;
}

bool Project::save(const QString &filepath)
{
    if (!m_configManager)
        return false;

    QString savePath = filepath.isEmpty() ? m_projectFilePath : filepath;
    if (savePath.isEmpty()) {
        qWarning() << "No file path specified for saving project";
        return false;
    }

    m_projectFilePath = savePath;

    return m_configManager->save();
}

QVariant Project::getValue(const QString &keyPath, const QVariant &defaultValue) const
{
    if (!m_configManager)
        return defaultValue;
    return m_configManager->getValue(keyPath, defaultValue);
}

void Project::setValue(const QString &keyPath, const QVariant &value)
{
    if (!m_configManager)
        return;
    m_configManager->setValue(keyPath, value);
}

QString Project::schemaFilePath() const
{
    // Example: get schema.json path from AppConfig config folder
    QString configFolder = AppConfig::instance().configFolder();
    QString schemaPath = QDir(configFolder).filePath("schema.json");
    return schemaPath;
}
