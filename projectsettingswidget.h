#ifndef PROJECTSETTINGSWIDGET_H
#define PROJECTSETTINGSWIDGET_H

#include <QWidget>
#include <QVariantMap>
#include <QMap>
#include <QStringList>

class QTabWidget;
class QFormLayout;
class QLineEdit;
class QCheckBox;
class QTableWidget;
class QLabel;

class ProjectSettingsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ProjectSettingsWidget(QWidget *parent = nullptr);

    // Load partial or full project settings; empty or missing keys mean inherit defaults
    void loadSettings(const QVariantMap &settings);

    // Retrieve current settings; empty widgets omitted to indicate inheritance
    QVariantMap getSettings() const;

private:
    struct PropertyInfo {
        QString key; // full key path, e.g. "api.access_token"
        QString type; // "string", "integer", "number", "boolean", "array", "object"
        QVariant defaultValue;
        QWidget *widget = nullptr; // pointer to input widget
    };

    // Recursive UI builder for a schema object
    QWidget* buildFormForSchema(const QVariantMap &schemaObj, const QString &parentKey = QString());

    // Helpers to create widgets for different types
    QWidget* createWidgetForProperty(const QString &key, const QVariantMap &propSchema);

    // Populate a form layout from schema properties
    void populateFormLayout(QFormLayout *formLayout, const QVariantMap &properties, const QString &parentKey);

    // Load values into widgets recursively
    void loadValues(const QVariantMap &values, const QString &parentKey = QString());

    // Extract values from widgets recursively
    QVariantMap extractValues(const QString &parentKey = QString()) const;

    // Helper to prettify keys for labels
    QString prettifyKey(const QString &key) const;

    // Command pipes tab UI
    QWidget* createCommandPipesTab();
    void loadCommandPipes(const QVariantMap &commandPipes);
    QVariantMap getCommandPipes() const;

    QTabWidget *m_tabWidget = nullptr;
    QLabel *m_bannerLabel = nullptr;

    // Map full key path to PropertyInfo for easy access
    QMap<QString, PropertyInfo> m_properties;

    // Command pipes widgets
    QTableWidget *m_commandPipesTable = nullptr;

    // Cached schema object for default_project_settings
    QVariantMap m_schemaDefaultProjectSettings;
};

#endif // PROJECTSETTINGSWIDGET_H
