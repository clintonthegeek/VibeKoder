#include "applicationsettingsdialog.h"
#include "projectsettingsdialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QTabWidget>
#include <QComboBox>
#include <QLabel>
#include <QDialogButtonBox>
#include <QTimeZone>
#include <QDebug>

ApplicationSettingsDialog::ApplicationSettingsDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Application Settings");
    resize(750, 550);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    m_tabWidget = new QTabWidget(this);
    mainLayout->addWidget(m_tabWidget);

    // Default Project Settings tab - embed the ProjectSettingsDialog as a widget
    m_defaultProjectSettingsDialog = new ProjectSettingsDialog(this);
    m_defaultProjectSettingsDialog->setWindowFlags(Qt::Widget); // Embed as widget, not dialog
    m_tabWidget->addTab(m_defaultProjectSettingsDialog, "Default Project Settings");

    // App Settings tab
    QWidget* appSettingsTab = new QWidget(this);
    QFormLayout* appLayout = new QFormLayout(appSettingsTab);

    m_timezoneCombo = new QComboBox(appSettingsTab);
    QList<QByteArray> tzIdBytes = QTimeZone::availableTimeZoneIds();
    for (const QByteArray& ba : tzIdBytes) {
        m_timezoneCombo->addItem(QString::fromUtf8(ba));
    }
    appLayout->addRow("Timezone:", m_timezoneCombo);

    m_tabWidget->addTab(appSettingsTab, "App Settings");

    // Dialog buttons
    QDialogButtonBox* buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    mainLayout->addWidget(buttonBox);

    connect(buttonBox, &QDialogButtonBox::accepted, this, &ApplicationSettingsDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &ApplicationSettingsDialog::reject);
}

void ApplicationSettingsDialog::loadSettings(const AppConfigData &data)
{
    // Load default project settings into embedded dialog
    m_defaultProjectSettingsDialog->loadSettings(data.defaultProjectSettings);

    // Load timezone
    int idx = m_timezoneCombo->findText(data.timezone);
    if (idx >= 0) {
        m_timezoneCombo->setCurrentIndex(idx);
    } else {
        qWarning() << "Timezone" << data.timezone << "not found in combo box";
    }
}

AppConfigData ApplicationSettingsDialog::getSettings() const
{
    AppConfigData data;

    // Get default project settings from embedded dialog
    data.defaultProjectSettings = m_defaultProjectSettingsDialog->getSettings();

    // Get timezone
    data.timezone = m_timezoneCombo->currentText();

    return data;
}
