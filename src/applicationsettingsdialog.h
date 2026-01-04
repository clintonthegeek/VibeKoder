#ifndef APPLICATIONSETTINGSDIALOG_NEW_H
#define APPLICATIONSETTINGSDIALOG_NEW_H

#include <QDialog>
#include "appconfigtypes.h"

class QTabWidget;
class QComboBox;
class ProjectSettingsDialog;

class ApplicationSettingsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ApplicationSettingsDialog(QWidget *parent = nullptr);

    // Load application settings
    void loadSettings(const AppConfigData &data);

    // Retrieve current settings
    AppConfigData getSettings() const;

private:
    QTabWidget* m_tabWidget;
    ProjectSettingsDialog* m_defaultProjectSettingsDialog;
    QComboBox* m_timezoneCombo;
};

#endif // APPLICATIONSETTINGSDIALOG_NEW_H
