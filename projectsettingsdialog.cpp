#include "projectsettingsdialog.h"

#include <QTabWidget>
#include <QFormLayout>
#include <QLineEdit>
#include <QTableWidget>
#include <QHeaderView>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QIntValidator>
#include <QDoubleValidator>
#include <QDebug>

ProjectSettingsDialog::ProjectSettingsDialog(QWidget* parent)
    : QDialog(parent)
{
    setupUi();
}

void ProjectSettingsDialog::setupUi()
{
    setWindowTitle("Project Settings");
    resize(600, 400);

    m_tabWidget = new QTabWidget(this);

    // === API Tab ===
    QWidget* apiTab = new QWidget(this);
    QFormLayout* apiLayout = new QFormLayout(apiTab);

    m_accessTokenEdit = new QLineEdit(apiTab);
    m_modelEdit = new QLineEdit(apiTab);
    m_maxTokensEdit = new QLineEdit(apiTab);
    m_maxTokensEdit->setValidator(new QIntValidator(1, 100000, this));
    m_temperatureEdit = new QLineEdit(apiTab);
    m_temperatureEdit->setValidator(new QDoubleValidator(0.0, 2.0, 3, this));
    m_topPEdit = new QLineEdit(apiTab);
    m_topPEdit->setValidator(new QDoubleValidator(0.0, 1.0, 3, this));
    m_frequencyPenaltyEdit = new QLineEdit(apiTab);
    m_frequencyPenaltyEdit->setValidator(new QDoubleValidator(-2.0, 2.0, 3, this));
    m_presencePenaltyEdit = new QLineEdit(apiTab);
    m_presencePenaltyEdit->setValidator(new QDoubleValidator(-2.0, 2.0, 3, this));

    apiLayout->addRow("Access Token:", m_accessTokenEdit);
    apiLayout->addRow("Model:", m_modelEdit);
    apiLayout->addRow("Max Tokens:", m_maxTokensEdit);
    apiLayout->addRow("Temperature:", m_temperatureEdit);
    apiLayout->addRow("Top P:", m_topPEdit);
    apiLayout->addRow("Frequency Penalty:", m_frequencyPenaltyEdit);
    apiLayout->addRow("Presence Penalty:", m_presencePenaltyEdit);

    m_tabWidget->addTab(apiTab, "API");

    // === Folders Tab ===
    QWidget* foldersTab = new QWidget(this);
    QFormLayout* foldersLayout = new QFormLayout(foldersTab);

    m_rootFolderEdit = new QLineEdit(foldersTab);
    m_docsFolderEdit = new QLineEdit(foldersTab);
    m_srcFolderEdit = new QLineEdit(foldersTab);
    m_sessionsFolderEdit = new QLineEdit(foldersTab);
    m_templatesFolderEdit = new QLineEdit(foldersTab);
    m_includeDocsFolderEdit = new QLineEdit(foldersTab);

    foldersLayout->addRow("Root Folder:", m_rootFolderEdit);
    foldersLayout->addRow("Docs Folder:", m_docsFolderEdit);
    foldersLayout->addRow("Source Folder:", m_srcFolderEdit);
    foldersLayout->addRow("Sessions Folder:", m_sessionsFolderEdit);
    foldersLayout->addRow("Templates Folder:", m_templatesFolderEdit);
    foldersLayout->addRow("Include Docs Folder:", m_includeDocsFolderEdit);

    m_tabWidget->addTab(foldersTab, "Folders");

    // === Filetypes Tab ===
    QWidget* filetypesTab = new QWidget(this);
    QFormLayout* filetypesLayout = new QFormLayout(filetypesTab);

    m_sourceFileTypesEdit = new QLineEdit(filetypesTab);
    m_docFileTypesEdit = new QLineEdit(filetypesTab);

    filetypesLayout->addRow("Source File Types (comma separated):", m_sourceFileTypesEdit);
    filetypesLayout->addRow("Doc File Types (comma separated):", m_docFileTypesEdit);

    m_tabWidget->addTab(filetypesTab, "File Types");

    // === Command Pipes Tab ===
    QWidget* commandPipesTab = new QWidget(this);
    QVBoxLayout* cpLayout = new QVBoxLayout(commandPipesTab);

    m_commandPipesTable = new QTableWidget(commandPipesTab);
    m_commandPipesTable->setColumnCount(2);
    m_commandPipesTable->setHorizontalHeaderLabels(QStringList() << "Name" << "Command(s)");
    m_commandPipesTable->horizontalHeader()->setStretchLastSection(true);
    m_commandPipesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_commandPipesTable->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);

    cpLayout->addWidget(m_commandPipesTable);

    QHBoxLayout* cpButtonsLayout = new QHBoxLayout();
    QPushButton* addBtn = new QPushButton("Add", commandPipesTab);
    QPushButton* removeBtn = new QPushButton("Remove", commandPipesTab);
    cpButtonsLayout->addWidget(addBtn);
    cpButtonsLayout->addWidget(removeBtn);
    cpButtonsLayout->addStretch();

    cpLayout->addLayout(cpButtonsLayout);

    connect(addBtn, &QPushButton::clicked, this, &ProjectSettingsDialog::onAddCommandPipe);
    connect(removeBtn, &QPushButton::clicked, this, &ProjectSettingsDialog::onRemoveCommandPipe);

    m_tabWidget->addTab(commandPipesTab, "Command Pipes");

    // === Dialog buttons ===
    QHBoxLayout* buttonsLayout = new QHBoxLayout();
    QPushButton* okBtn = new QPushButton("OK", this);
    QPushButton* cancelBtn = new QPushButton("Cancel", this);
    buttonsLayout->addStretch();
    buttonsLayout->addWidget(okBtn);
    buttonsLayout->addWidget(cancelBtn);

    connect(okBtn, &QPushButton::clicked, this, [this]() {
        // Validate inputs here if needed
        accept();
    });
    connect(cancelBtn, &QPushButton::clicked, this, &ProjectSettingsDialog::reject);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(m_tabWidget);
    mainLayout->addLayout(buttonsLayout);
}

void ProjectSettingsDialog::loadSettings(const QVariantMap& apiSettings,
                                         const QVariantMap& folders,
                                         const QStringList& sourceFileTypes,
                                         const QStringList& docFileTypes,
                                         const QMap<QString, QStringList>& commandPipes)
{
    m_apiSettings = apiSettings;
    m_folders = folders;
    m_sourceFileTypes = sourceFileTypes;
    m_docFileTypes = docFileTypes;
    m_commandPipes = commandPipes;

    // API
    m_accessTokenEdit->setText(apiSettings.value("access_token").toString());
    m_modelEdit->setText(apiSettings.value("model").toString());
    m_maxTokensEdit->setText(QString::number(apiSettings.value("max_tokens", 800).toInt()));
    m_temperatureEdit->setText(QString::number(apiSettings.value("temperature", 0.3).toDouble()));
    m_topPEdit->setText(QString::number(apiSettings.value("top_p", 1.0).toDouble()));
    m_frequencyPenaltyEdit->setText(QString::number(apiSettings.value("frequency_penalty", 0.0).toDouble()));
    m_presencePenaltyEdit->setText(QString::number(apiSettings.value("presence_penalty", 0.0).toDouble()));

    // Folders
    m_rootFolderEdit->setText(folders.value("root").toString());
    m_docsFolderEdit->setText(folders.value("docs").toString());
    m_srcFolderEdit->setText(folders.value("src").toString());
    m_sessionsFolderEdit->setText(folders.value("sessions").toString());
    m_templatesFolderEdit->setText(folders.value("templates").toString());
    m_includeDocsFolderEdit->setText(folders.value("include_docs").toString());

    // Filetypes
    m_sourceFileTypesEdit->setText(sourceFileTypes.join(", "));
    m_docFileTypesEdit->setText(docFileTypes.join(", "));

    // Command pipes
    populateCommandPipesTable();
}

void ProjectSettingsDialog::populateCommandPipesTable()
{
    m_commandPipesTable->clearContents();
    m_commandPipesTable->setRowCount(m_commandPipes.size());

    int row = 0;
    for (auto it = m_commandPipes.begin(); it != m_commandPipes.end(); ++it, ++row) {
        QTableWidgetItem* nameItem = new QTableWidgetItem(it.key());
        nameItem->setFlags(nameItem->flags() & ~Qt::ItemIsEditable); // Name not editable for now
        m_commandPipesTable->setItem(row, 0, nameItem);

        // Join commands with " | " separator for display/edit
        QString cmdStr = it.value().join(" | ");
        QTableWidgetItem* cmdItem = new QTableWidgetItem(cmdStr);
        m_commandPipesTable->setItem(row, 1, cmdItem);
    }
}

void ProjectSettingsDialog::getSettings(QVariantMap& apiSettings,
                                        QVariantMap& folders,
                                        QStringList& sourceFileTypes,
                                        QStringList& docFileTypes,
                                        QMap<QString, QStringList>& commandPipes) const
{
    // API
    apiSettings.clear();
    apiSettings["access_token"] = m_accessTokenEdit->text().trimmed();
    apiSettings["model"] = m_modelEdit->text().trimmed();
    apiSettings["max_tokens"] = m_maxTokensEdit->text().toInt();
    apiSettings["temperature"] = m_temperatureEdit->text().toDouble();
    apiSettings["top_p"] = m_topPEdit->text().toDouble();
    apiSettings["frequency_penalty"] = m_frequencyPenaltyEdit->text().toDouble();
    apiSettings["presence_penalty"] = m_presencePenaltyEdit->text().toDouble();

    // Folders
    folders.clear();
    folders["root"] = m_rootFolderEdit->text().trimmed();
    folders["docs"] = m_docsFolderEdit->text().trimmed();
    folders["src"] = m_srcFolderEdit->text().trimmed();
    folders["sessions"] = m_sessionsFolderEdit->text().trimmed();
    folders["templates"] = m_templatesFolderEdit->text().trimmed();
    folders["include_docs"] = m_includeDocsFolderEdit->text().trimmed();

    // Filetypes
    sourceFileTypes = m_sourceFileTypesEdit->text().split(',', Qt::SkipEmptyParts);
    for (QString& s : sourceFileTypes) s = s.trimmed();

    docFileTypes = m_docFileTypesEdit->text().split(',', Qt::SkipEmptyParts);
    for (QString& s : docFileTypes) s = s.trimmed();

    // Command pipes
    commandPipes.clear();
    int rowCount = m_commandPipesTable->rowCount();
    for (int row = 0; row < rowCount; ++row) {
        QTableWidgetItem* nameItem = m_commandPipesTable->item(row, 0);
        QTableWidgetItem* cmdItem = m_commandPipesTable->item(row, 1);
        if (!nameItem || !cmdItem)
            continue;

        QString name = nameItem->text().trimmed();
        QString cmdStr = cmdItem->text().trimmed();
        QStringList cmds = cmdStr.split('|', Qt::SkipEmptyParts);
        for (QString& s : cmds) s = s.trimmed();

        if (!name.isEmpty() && !cmds.isEmpty()) {
            commandPipes[name] = cmds;
        }
    }
}

void ProjectSettingsDialog::onAddCommandPipe()
{
    int newRow = m_commandPipesTable->rowCount();
    m_commandPipesTable->insertRow(newRow);

    QTableWidgetItem* nameItem = new QTableWidgetItem("new_command");
    QTableWidgetItem* cmdItem = new QTableWidgetItem("command args");

    m_commandPipesTable->setItem(newRow, 0, nameItem);
    m_commandPipesTable->setItem(newRow, 1, cmdItem);

    m_commandPipesTable->editItem(nameItem);
}

void ProjectSettingsDialog::onRemoveCommandPipe()
{
    auto selectedRanges = m_commandPipesTable->selectedRanges();
    if (selectedRanges.isEmpty())
        return;

    // Remove rows from bottom to top to avoid index shifting
    for (int i = selectedRanges.size() - 1; i >= 0; --i) {
        int topRow = selectedRanges.at(i).topRow();
        int bottomRow = selectedRanges.at(i).bottomRow();
        for (int row = bottomRow; row >= topRow; --row) {
            m_commandPipesTable->removeRow(row);
        }
    }
}
