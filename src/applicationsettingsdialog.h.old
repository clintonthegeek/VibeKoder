#ifndef APPLICATIONSETTINGSDIALOG_H
#define APPLICATIONSETTINGSDIALOG_H

#include <QDialog>
#include <QVariantMap>

class ProjectSettingsWidget;
class QComboBox;

class ApplicationSettingsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ApplicationSettingsDialog(QWidget *parent = nullptr);

    // Load app-wide settings from config.json
    void loadSettings(const QVariantMap &settings);

    // Get updated app-wide settings
    QVariantMap getSettings() const;

private:
    ProjectSettingsWidget *m_defaultProjectSettingsWidget = nullptr;
    QComboBox *m_timezoneCombo = nullptr;

    QVariantMap m_appSettings; // e.g. timezone
};

#endif // APPLICATIONSETTINGSDIALOG_H
