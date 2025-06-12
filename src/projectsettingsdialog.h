#ifndef PROJECTSETTINGSDIALOG_H
#define PROJECTSETTINGSDIALOG_H

#include <QDialog>

class ProjectSettingsWidget;

class ProjectSettingsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ProjectSettingsDialog(QWidget *parent = nullptr);

    // Load project settings into the widget
    void loadSettings(const QVariantMap &settings);

    // Retrieve current settings from the widget
    QVariantMap getSettings() const;

private:
    ProjectSettingsWidget *m_settingsWidget = nullptr;
};

#endif // PROJECTSETTINGSDIALOG_H
