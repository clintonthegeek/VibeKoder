#ifndef APPLICATIONSETTINGSDIALOG_H
#define APPLICATIONSETTINGSDIALOG_H

#include <QDialog>
#include "appconfigtypes.h"

class QTabWidget;
class QComboBox;
class QLineEdit;
class QSpinBox;
class QDoubleSpinBox;

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

    // Default project settings widgets (simplified)
    QLineEdit* m_defaultApiToken;
    QLineEdit* m_defaultApiModel;
    QSpinBox* m_defaultMaxTokens;
    QDoubleSpinBox* m_defaultTemperature;

    // App settings widgets
    QComboBox* m_timezoneCombo;
};

#endif // APPLICATIONSETTINGSDIALOG_H
