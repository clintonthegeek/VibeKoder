#include "projectsettingsdialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGridLayout>
#include <QTabWidget>
#include <QLineEdit>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QListWidget>
#include <QTableWidget>
#include <QPushButton>
#include <QLabel>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QHeaderView>
#include <QInputDialog>
#include <QMessageBox>

ProjectSettingsDialog::ProjectSettingsDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Project Settings");
    resize(700, 500);
    setupUi();
}

void ProjectSettingsDialog::setupUi()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Info banner
    QLabel* banner = new QLabel(
        "<b>Note:</b> Configure your project settings. API credentials are required for LLM functionality.",
        this);
    banner->setWordWrap(true);
    mainLayout->addWidget(banner);

    // Tab widget
    m_tabWidget = new QTabWidget(this);
    m_tabWidget->addTab(createApiTab(), "API Settings");
    m_tabWidget->addTab(createFoldersTab(), "Folders");
    m_tabWidget->addTab(createFiletypesTab(), "File Types");
    m_tabWidget->addTab(createCommandPipesTab(), "Command Pipes");
    mainLayout->addWidget(m_tabWidget);

    // Dialog buttons
    QDialogButtonBox* buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    mainLayout->addWidget(buttonBox);

    connect(buttonBox, &QDialogButtonBox::accepted, this, &ProjectSettingsDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &ProjectSettingsDialog::reject);
}

QWidget* ProjectSettingsDialog::createApiTab()
{
    QWidget* tab = new QWidget(this);
    QFormLayout* layout = new QFormLayout(tab);

    m_apiAccessToken = new QLineEdit(tab);
    m_apiAccessToken->setEchoMode(QLineEdit::Password);
    m_apiAccessToken->setPlaceholderText("Enter your OpenAI API key");
    layout->addRow("Access Token:", m_apiAccessToken);

    m_apiModel = new QLineEdit(tab);
    m_apiModel->setPlaceholderText("e.g., gpt-4.1-mini");
    layout->addRow("Model:", m_apiModel);

    m_apiMaxTokens = new QSpinBox(tab);
    m_apiMaxTokens->setRange(1, 100000);
    m_apiMaxTokens->setValue(20000);
    layout->addRow("Max Tokens:", m_apiMaxTokens);

    m_apiTemperature = new QDoubleSpinBox(tab);
    m_apiTemperature->setRange(0.0, 2.0);
    m_apiTemperature->setSingleStep(0.1);
    m_apiTemperature->setDecimals(2);
    m_apiTemperature->setValue(0.3);
    layout->addRow("Temperature:", m_apiTemperature);

    m_apiTopP = new QDoubleSpinBox(tab);
    m_apiTopP->setRange(0.0, 1.0);
    m_apiTopP->setSingleStep(0.1);
    m_apiTopP->setDecimals(2);
    m_apiTopP->setValue(1.0);
    layout->addRow("Top P:", m_apiTopP);

    m_apiFrequencyPenalty = new QDoubleSpinBox(tab);
    m_apiFrequencyPenalty->setRange(-2.0, 2.0);
    m_apiFrequencyPenalty->setSingleStep(0.1);
    m_apiFrequencyPenalty->setDecimals(2);
    m_apiFrequencyPenalty->setValue(0.0);
    layout->addRow("Frequency Penalty:", m_apiFrequencyPenalty);

    m_apiPresencePenalty = new QDoubleSpinBox(tab);
    m_apiPresencePenalty->setRange(-2.0, 2.0);
    m_apiPresencePenalty->setSingleStep(0.1);
    m_apiPresencePenalty->setDecimals(2);
    m_apiPresencePenalty->setValue(0.0);
    layout->addRow("Presence Penalty:", m_apiPresencePenalty);

    return tab;
}

QWidget* ProjectSettingsDialog::createFoldersTab()
{
    QWidget* tab = new QWidget(this);
    QFormLayout* layout = new QFormLayout(tab);

    // Root folder with browse button
    QHBoxLayout* rootLayout = new QHBoxLayout();
    m_rootFolder = new QLineEdit(tab);
    m_rootFolder->setPlaceholderText("Project root directory");
    rootLayout->addWidget(m_rootFolder);
    QPushButton* browseBtn = new QPushButton("Browse...", tab);
    connect(browseBtn, &QPushButton::clicked, this, &ProjectSettingsDialog::onBrowseRootFolder);
    rootLayout->addWidget(browseBtn);
    layout->addRow("Root Folder:", rootLayout);

    m_docsFolder = new QLineEdit(tab);
    m_docsFolder->setPlaceholderText("Relative to root, e.g., 'docs'");
    layout->addRow("Docs Folder:", m_docsFolder);

    m_srcFolder = new QLineEdit(tab);
    m_srcFolder->setPlaceholderText("Relative to root, e.g., '.' or 'src'");
    layout->addRow("Source Folder:", m_srcFolder);

    m_sessionsFolder = new QLineEdit(tab);
    m_sessionsFolder->setPlaceholderText("Relative to root, e.g., 'sessions'");
    layout->addRow("Sessions Folder:", m_sessionsFolder);

    m_templatesFolder = new QLineEdit(tab);
    m_templatesFolder->setPlaceholderText("Relative to root, e.g., 'templates'");
    layout->addRow("Templates Folder:", m_templatesFolder);

    return tab;
}

QWidget* ProjectSettingsDialog::createFiletypesTab()
{
    QWidget* tab = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(tab);

    // Source file types section
    layout->addWidget(new QLabel("<b>Source File Types</b> (patterns like *.cpp, *.h):", tab));

    QHBoxLayout* sourceLayout = new QHBoxLayout();
    m_sourceFileTypesList = new QListWidget(tab);
    sourceLayout->addWidget(m_sourceFileTypesList);

    QVBoxLayout* sourceButtonsLayout = new QVBoxLayout();
    m_addSourceBtn = new QPushButton("Add", tab);
    m_removeSourceBtn = new QPushButton("Remove", tab);
    connect(m_addSourceBtn, &QPushButton::clicked, this, &ProjectSettingsDialog::onAddSourceFileType);
    connect(m_removeSourceBtn, &QPushButton::clicked, this, &ProjectSettingsDialog::onRemoveSourceFileType);
    sourceButtonsLayout->addWidget(m_addSourceBtn);
    sourceButtonsLayout->addWidget(m_removeSourceBtn);
    sourceButtonsLayout->addStretch();
    sourceLayout->addLayout(sourceButtonsLayout);

    layout->addLayout(sourceLayout);

    // Doc file types section
    layout->addWidget(new QLabel("<b>Documentation File Types</b> (extensions like md, txt):", tab));

    QHBoxLayout* docLayout = new QHBoxLayout();
    m_docFileTypesList = new QListWidget(tab);
    docLayout->addWidget(m_docFileTypesList);

    QVBoxLayout* docButtonsLayout = new QVBoxLayout();
    m_addDocBtn = new QPushButton("Add", tab);
    m_removeDocBtn = new QPushButton("Remove", tab);
    connect(m_addDocBtn, &QPushButton::clicked, this, &ProjectSettingsDialog::onAddDocFileType);
    connect(m_removeDocBtn, &QPushButton::clicked, this, &ProjectSettingsDialog::onRemoveDocFileType);
    docButtonsLayout->addWidget(m_addDocBtn);
    docButtonsLayout->addWidget(m_removeDocBtn);
    docButtonsLayout->addStretch();
    docLayout->addLayout(docButtonsLayout);

    layout->addLayout(docLayout);

    return tab;
}

QWidget* ProjectSettingsDialog::createCommandPipesTab()
{
    QWidget* tab = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(tab);

    layout->addWidget(new QLabel(
        "<b>Command Pipes:</b> Define shell commands that can be inserted into prompts.", tab));

    m_commandPipesTable = new QTableWidget(tab);
    m_commandPipesTable->setColumnCount(2);
    m_commandPipesTable->setHorizontalHeaderLabels(QStringList() << "Name" << "Command");
    m_commandPipesTable->horizontalHeader()->setStretchLastSection(true);
    m_commandPipesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    layout->addWidget(m_commandPipesTable);

    QHBoxLayout* buttonsLayout = new QHBoxLayout();
    m_addPipeBtn = new QPushButton("Add Pipe", tab);
    m_removePipeBtn = new QPushButton("Remove Pipe", tab);
    connect(m_addPipeBtn, &QPushButton::clicked, this, &ProjectSettingsDialog::onAddCommandPipe);
    connect(m_removePipeBtn, &QPushButton::clicked, this, &ProjectSettingsDialog::onRemoveCommandPipe);
    buttonsLayout->addWidget(m_addPipeBtn);
    buttonsLayout->addWidget(m_removePipeBtn);
    buttonsLayout->addStretch();
    layout->addLayout(buttonsLayout);

    return tab;
}

void ProjectSettingsDialog::loadSettings(const ProjectConfig &config)
{
    // API tab
    m_apiAccessToken->setText(config.apiAccessToken);
    m_apiModel->setText(config.apiModel);
    m_apiMaxTokens->setValue(config.apiMaxTokens);
    m_apiTemperature->setValue(config.apiTemperature);
    m_apiTopP->setValue(config.apiTopP);
    m_apiFrequencyPenalty->setValue(config.apiFrequencyPenalty);
    m_apiPresencePenalty->setValue(config.apiPresencePenalty);

    // Folders tab
    m_rootFolder->setText(config.rootFolder);
    m_docsFolder->setText(config.docsFolder);
    m_srcFolder->setText(config.srcFolder);
    m_sessionsFolder->setText(config.sessionsFolder);
    m_templatesFolder->setText(config.templatesFolder);

    // Filetypes tab
    m_sourceFileTypesList->clear();
    m_sourceFileTypesList->addItems(config.sourceFileTypes);

    m_docFileTypesList->clear();
    m_docFileTypesList->addItems(config.docFileTypes);

    // Command Pipes tab
    m_commandPipesTable->setRowCount(0);
    for (auto it = config.commandPipes.constBegin(); it != config.commandPipes.constEnd(); ++it) {
        int row = m_commandPipesTable->rowCount();
        m_commandPipesTable->insertRow(row);
        m_commandPipesTable->setItem(row, 0, new QTableWidgetItem(it.key()));
        m_commandPipesTable->setItem(row, 1, new QTableWidgetItem(it.value().join(" ")));
    }
}

ProjectConfig ProjectSettingsDialog::getSettings() const
{
    ProjectConfig config;

    // API tab
    config.apiAccessToken = m_apiAccessToken->text();
    config.apiModel = m_apiModel->text();
    config.apiMaxTokens = m_apiMaxTokens->value();
    config.apiTemperature = m_apiTemperature->value();
    config.apiTopP = m_apiTopP->value();
    config.apiFrequencyPenalty = m_apiFrequencyPenalty->value();
    config.apiPresencePenalty = m_apiPresencePenalty->value();

    // Folders tab
    config.rootFolder = m_rootFolder->text();
    config.docsFolder = m_docsFolder->text();
    config.srcFolder = m_srcFolder->text();
    config.sessionsFolder = m_sessionsFolder->text();
    config.templatesFolder = m_templatesFolder->text();

    // Filetypes tab
    config.sourceFileTypes.clear();
    for (int i = 0; i < m_sourceFileTypesList->count(); ++i) {
        config.sourceFileTypes.append(m_sourceFileTypesList->item(i)->text());
    }

    config.docFileTypes.clear();
    for (int i = 0; i < m_docFileTypesList->count(); ++i) {
        config.docFileTypes.append(m_docFileTypesList->item(i)->text());
    }

    // Command Pipes tab
    config.commandPipes.clear();
    for (int row = 0; row < m_commandPipesTable->rowCount(); ++row) {
        QString name = m_commandPipesTable->item(row, 0)->text();
        QString cmdStr = m_commandPipesTable->item(row, 1)->text();
        QStringList cmdParts = cmdStr.split(' ', Qt::SkipEmptyParts);
        config.commandPipes.insert(name, cmdParts);
    }

    return config;
}

void ProjectSettingsDialog::onBrowseRootFolder()
{
    QString dir = QFileDialog::getExistingDirectory(this, "Select Project Root Folder",
                                                     m_rootFolder->text());
    if (!dir.isEmpty()) {
        m_rootFolder->setText(dir);
    }
}

void ProjectSettingsDialog::onAddSourceFileType()
{
    bool ok;
    QString pattern = QInputDialog::getText(this, "Add Source File Type",
                                           "File pattern (e.g., *.cpp, *.h):",
                                           QLineEdit::Normal, "", &ok);
    if (ok && !pattern.isEmpty()) {
        m_sourceFileTypesList->addItem(pattern);
    }
}

void ProjectSettingsDialog::onRemoveSourceFileType()
{
    QListWidgetItem* item = m_sourceFileTypesList->currentItem();
    if (item) {
        delete item;
    }
}

void ProjectSettingsDialog::onAddDocFileType()
{
    bool ok;
    QString ext = QInputDialog::getText(this, "Add Doc File Type",
                                       "File extension (e.g., md, txt):",
                                       QLineEdit::Normal, "", &ok);
    if (ok && !ext.isEmpty()) {
        m_docFileTypesList->addItem(ext);
    }
}

void ProjectSettingsDialog::onRemoveDocFileType()
{
    QListWidgetItem* item = m_docFileTypesList->currentItem();
    if (item) {
        delete item;
    }
}

void ProjectSettingsDialog::onAddCommandPipe()
{
    bool ok;
    QString name = QInputDialog::getText(this, "Add Command Pipe",
                                        "Pipe name (e.g., git_diff):",
                                        QLineEdit::Normal, "", &ok);
    if (!ok || name.isEmpty()) return;

    QString cmd = QInputDialog::getText(this, "Add Command Pipe",
                                       "Command (e.g., git diff .):",
                                       QLineEdit::Normal, "", &ok);
    if (!ok || cmd.isEmpty()) return;

    int row = m_commandPipesTable->rowCount();
    m_commandPipesTable->insertRow(row);
    m_commandPipesTable->setItem(row, 0, new QTableWidgetItem(name));
    m_commandPipesTable->setItem(row, 1, new QTableWidgetItem(cmd));
}

void ProjectSettingsDialog::onRemoveCommandPipe()
{
    int row = m_commandPipesTable->currentRow();
    if (row >= 0) {
        m_commandPipesTable->removeRow(row);
    }
}
