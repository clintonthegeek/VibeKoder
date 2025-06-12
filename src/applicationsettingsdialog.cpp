#include "applicationsettingsdialog.h"
#include "projectsettingswidget.h"

#include <QTabWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDialogButtonBox>
#include <QLabel>
#include <QComboBox>
#include <QTimeZone>
#include <QDebug>
#include <qformlayout.h>

ApplicationSettingsDialog::ApplicationSettingsDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Application Settings");
    resize(700, 500);

    auto mainLayout = new QVBoxLayout(this);

    auto tabWidget = new QTabWidget(this);
    mainLayout->addWidget(tabWidget);

    // DefaultProjectSettings tab with ProjectSettingsWidget embedded
    m_defaultProjectSettingsWidget = new ProjectSettingsWidget(this);
    tabWidget->addTab(m_defaultProjectSettingsWidget, "Default Project Settings");

    // App Settings tab with timezone selector
    QWidget *appSettingsTab = new QWidget(this);
    auto appLayout = new QFormLayout(appSettingsTab);

    m_timezoneCombo = new QComboBox(appSettingsTab);
    QList<QByteArray> tzIdBytes = QTimeZone::availableTimeZoneIds();
    QStringList tzIds;
    for (const QByteArray &ba : tzIdBytes) {
        tzIds.append(QString::fromUtf8(ba));
    }
    for (const QString &tzId : tzIds) {
        m_timezoneCombo->addItem(tzId);
    }
    appLayout->addRow(new QLabel("Timezone:"), m_timezoneCombo);

    tabWidget->addTab(appSettingsTab, "App Settings");

    // Dialog buttons
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    mainLayout->addWidget(buttonBox);

    connect(buttonBox, &QDialogButtonBox::accepted, this, &ApplicationSettingsDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &ApplicationSettingsDialog::reject);
}

void ApplicationSettingsDialog::loadSettings(const QVariantMap &settings)
{
    if (settings.isEmpty()) {
        qWarning() << "ApplicationSettingsDialog::loadSettings called with empty settings";
        return;
    }
    if (settings.contains("default_project_settings")) {
        QVariantMap defaultProjSettings = settings.value("default_project_settings").toMap();
        m_defaultProjectSettingsWidget->loadSettings(defaultProjSettings);
    } else {
        qWarning() << "No default_project_settings found in app settings";
    }
    if (settings.contains("app_settings")) {
        QVariantMap appSettings = settings.value("app_settings").toMap();
        m_appSettings = appSettings;
        QString tz = appSettings.value("timezone").toString();
        int idx = m_timezoneCombo->findText(tz);
        if (idx >= 0)
            m_timezoneCombo->setCurrentIndex(idx);
        else
            qWarning() << "Timezone" << tz << "not found in combo box";
    } else {
        qWarning() << "No app_settings found in app settings";
    }
}

QVariantMap ApplicationSettingsDialog::getSettings() const
{
    QVariantMap result;

    // Get default project settings from embedded widget
    QVariantMap defaultProjSettings = m_defaultProjectSettingsWidget->getSettings();
    result.insert("default_project_settings", defaultProjSettings);

    // Get app_settings from UI
    QVariantMap appSettings = m_appSettings;
    appSettings["timezone"] = m_timezoneCombo->currentText();
    result.insert("app_settings", appSettings);

    return result;
}
