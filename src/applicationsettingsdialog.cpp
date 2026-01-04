#include "applicationsettingsdialog.h"
#include "appconfig.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QTabWidget>
#include <QComboBox>
#include <QLineEdit>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QDialogButtonBox>
#include <QTimeZone>
#include <QGroupBox>
#include <QDebug>

ApplicationSettingsDialog::ApplicationSettingsDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Application Settings");
    resize(650, 450);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    QLabel* banner = new QLabel(
        "<b>Application Settings:</b> These are default settings for new projects.",
        this);
    banner->setWordWrap(true);
    mainLayout->addWidget(banner);

    m_tabWidget = new QTabWidget(this);
    mainLayout->addWidget(m_tabWidget);

    // Default Project Settings tab
    QWidget* defaultsTab = new QWidget(this);
    QVBoxLayout* defaultsLayout = new QVBoxLayout(defaultsTab);

    QLabel* defaultsInfo = new QLabel(
        "These settings will be used when creating <b>new projects</b>. "
        "To change settings for an existing project, use <b>Project Settings</b> instead.",
        this);
    defaultsInfo->setWordWrap(true);
    defaultsInfo->setStyleSheet("QLabel { background-color: #ffffcc; padding: 8px; border: 1px solid #cccc99; }");
    defaultsLayout->addWidget(defaultsInfo);

    QFormLayout* defaultsForm = new QFormLayout();

    m_defaultApiToken = new QLineEdit(this);
    m_defaultApiToken->setEchoMode(QLineEdit::Password);
    m_defaultApiToken->setPlaceholderText("Default API key for new projects");
    defaultsForm->addRow("Default API Key:", m_defaultApiToken);

    m_defaultApiModel = new QLineEdit(this);
    m_defaultApiModel->setPlaceholderText("e.g., gpt-4.1-mini");
    defaultsForm->addRow("Default Model:", m_defaultApiModel);

    m_defaultMaxTokens = new QSpinBox(this);
    m_defaultMaxTokens->setRange(1, 100000);
    m_defaultMaxTokens->setValue(20000);
    defaultsForm->addRow("Default Max Tokens:", m_defaultMaxTokens);

    m_defaultTemperature = new QDoubleSpinBox(this);
    m_defaultTemperature->setRange(0.0, 2.0);
    m_defaultTemperature->setSingleStep(0.1);
    m_defaultTemperature->setDecimals(2);
    m_defaultTemperature->setValue(0.3);
    defaultsForm->addRow("Default Temperature:", m_defaultTemperature);

    defaultsLayout->addLayout(defaultsForm);
    defaultsLayout->addStretch();

    m_tabWidget->addTab(defaultsTab, "Default Project Settings");

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
    // Load default project settings (simplified - just API settings)
    m_defaultApiToken->setText(data.defaultProjectSettings.apiAccessToken);
    m_defaultApiModel->setText(data.defaultProjectSettings.apiModel);
    m_defaultMaxTokens->setValue(data.defaultProjectSettings.apiMaxTokens);
    m_defaultTemperature->setValue(data.defaultProjectSettings.apiTemperature);

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

    // Get default project settings (partial - user can edit full settings per project)
    data.defaultProjectSettings.apiAccessToken = m_defaultApiToken->text();
    data.defaultProjectSettings.apiModel = m_defaultApiModel->text();
    data.defaultProjectSettings.apiMaxTokens = m_defaultMaxTokens->value();
    data.defaultProjectSettings.apiTemperature = m_defaultTemperature->value();
    // Keep other defaults from current config
    data.defaultProjectSettings = AppConfig::instance().data().defaultProjectSettings;
    data.defaultProjectSettings.apiAccessToken = m_defaultApiToken->text();
    data.defaultProjectSettings.apiModel = m_defaultApiModel->text();
    data.defaultProjectSettings.apiMaxTokens = m_defaultMaxTokens->value();
    data.defaultProjectSettings.apiTemperature = m_defaultTemperature->value();

    // Get timezone
    data.timezone = m_timezoneCombo->currentText();

    return data;
}
