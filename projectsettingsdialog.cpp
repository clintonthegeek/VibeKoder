#include "projectsettingsdialog.h"
#include "projectsettingswidget.h"
#include <QVBoxLayout>
#include <QDialogButtonBox>

ProjectSettingsDialog::ProjectSettingsDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Project Settings");
    resize(600, 400);

    auto layout = new QVBoxLayout(this);

    m_settingsWidget = new ProjectSettingsWidget(this);
    layout->addWidget(m_settingsWidget);

    // Add standard OK/Cancel buttons
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    layout->addWidget(buttonBox);

    connect(buttonBox, &QDialogButtonBox::accepted, this, &ProjectSettingsDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &ProjectSettingsDialog::reject);
}

void ProjectSettingsDialog::loadSettings(const QVariantMap &settings)
{
    if (m_settingsWidget)
        m_settingsWidget->loadSettings(settings);
}

QVariantMap ProjectSettingsDialog::getSettings() const
{
    if (m_settingsWidget)
        return m_settingsWidget->getSettings();
    return QVariantMap();
}
