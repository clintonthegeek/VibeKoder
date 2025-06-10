#include "mainwindow.h"
#include "projectsettingsdialog.h"
#include "sessiontabwidget.h"
#include "detachedwindow.h"

#include <QMenuBar>
#include <QMenu>
#include <QToolBar>
#include <QStatusBar>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QFileDialog>
#include <QMessageBox>
#include <QDebug>
#include <QStandardPaths>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setupUi();
    tryAutoLoadProject();
    m_tabManager = new TabManager(m_tabWidget, m_projectTab, m_project, this);
}

MainWindow::~MainWindow()
{
    // // Delete all open sessions
    // for (auto tab : m_openSessions) {
    //     if (tab) {
    //         tab->deleteLater();
    //     }
    // }
    // m_openSessions.clear();
    delete m_project;
}

void MainWindow::setupUi()
{
    setWindowTitle("VibeKoder");

    // === Menu and Toolbar ===
    QMenu* projectMenu = menuBar()->addMenu("Project");
    QAction* openProjectAction = new QAction("Open Project", this);
    projectMenu->addAction(openProjectAction);
    connect(openProjectAction, &QAction::triggered, this, &MainWindow::onOpenProject);

    QAction* newTempSessionAction = new QAction("New Temp Session", this);
    newTempSessionAction->setShortcut(QKeySequence("Ctrl+T"));
    connect(newTempSessionAction, &QAction::triggered, this, &MainWindow::onNewTempSession);

    QToolBar* toolbar = addToolBar("Main Toolbar");
    toolbar->addAction(openProjectAction);
    toolbar->addAction(newTempSessionAction);


    statusBar();

    // === Central Tabs ===
    m_tabWidget = new DraggableTabWidget(this);
    setCentralWidget(m_tabWidget);
    m_tabWidget->setTabsClosable(true);
    m_tabWidget->setIsMainTabWidget(true);

    // === Project Tab ===
    m_projectTab = new QWidget();
    m_tabWidget->setProjectTabWidget(m_projectTab);

    QVBoxLayout* projLayout = new QVBoxLayout(m_projectTab);

    m_projectInfoLabel = new QLabel("No project loaded");
    m_projectInfoLabel->setWordWrap(true);
    projLayout->addWidget(m_projectInfoLabel);

    projLayout->addWidget(new QLabel("Templates:"));
    m_templateList = new QListWidget();
    projLayout->addWidget(m_templateList);

    m_createSessionBtn = new QPushButton("Create New Session from Template");
    projLayout->addWidget(m_createSessionBtn);
    connect(m_createSessionBtn, &QPushButton::clicked, this, &MainWindow::onCreateSessionFromTemplate);

    projLayout->addSpacing(10);
    projLayout->addWidget(new QLabel("Available Sessions:"));
    m_sessionList = new QListWidget();
    projLayout->addWidget(m_sessionList);
    connect(m_sessionList, &QListWidget::itemDoubleClicked, this, &MainWindow::onOpenSelectedSession);

    m_openSessionBtn = new QPushButton("Open Selected Session");
    projLayout->addWidget(m_openSessionBtn);
    connect(m_openSessionBtn, &QPushButton::clicked, this, &MainWindow::onOpenSelectedSession);

    m_projectSettingsBtn = new QPushButton("Project Settings", m_projectTab);
    m_projectSettingsBtn->setEnabled(false); // Disabled until a project is loaded
    projLayout->addWidget(m_projectSettingsBtn);
    connect(m_projectSettingsBtn, &QPushButton::clicked, this, &MainWindow::onProjectSettingsClicked);

    m_tabWidget->addTab(m_projectTab, "Project");

    // Connections for Tab Widget
    connect(m_tabWidget, &QTabWidget::tabCloseRequested, this, [this](int index){
        QWidget* widget = m_tabWidget->widget(index);
        if (!widget)
            return;

        if (widget == m_projectTab) {
            // Ignore close request on Project tab
            return;
        }

        // Remove tab by widget pointer for safety
        m_tabWidget->removeTab(widget);
        widget->deleteLater();

        // Remove from m_openSessions by widget pointer
        QString sessionPathToRemove;
        for (auto it = m_openSessions.begin(); it != m_openSessions.end(); ++it) {
            if (it.value() == widget) {
                sessionPathToRemove = it.key();
                break;
            }
        }
        if (!sessionPathToRemove.isEmpty()) {
            m_openSessions.remove(sessionPathToRemove);
            qDebug() << "[MainWindow] Removed session from m_openSessions:" << sessionPathToRemove;
        }
    });

    // Remove close button on Project tab, prevent closure by overriding tab close
    int projIndex = m_tabWidget->indexOf(m_projectTab);
    if (projIndex != -1) {
        m_tabWidget->tabBar()->setTabButton(projIndex, QTabBar::RightSide, nullptr);
    }
}

void MainWindow::onNewTempSession()
{
    QString tempDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    QString tempFileName = QString("vibekoder_temp_%1.md").arg(QDateTime::currentMSecsSinceEpoch());
    QString tempFilePath = QDir(tempDir).filePath(tempFileName);

    QFile tempFile(tempFilePath);
    if (!tempFile.exists()) {
        if (!tempFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QMessageBox::warning(this, "Error", "Failed to create temporary session file.");
            return;
        }

        QTextStream out(&tempFile);

        // Write minimal valid session content with a system prompt slice
        QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
        out << "=={ System | " << timestamp << " }==\n";
        out << "You are a helpful assistant.\n\n";

        tempFile.close();
    }

    SessionTabWidget* tempTab = new SessionTabWidget(tempFilePath, nullptr, m_tabWidget, true);
    m_tabWidget->addTab(tempTab, "Temp");
    m_tabWidget->setCurrentWidget(tempTab);

    connect(tempTab, &SessionTabWidget::tempSessionSaved, this, [this, tempTab](const QString& newFilePath){
        int idx = m_tabWidget->indexOf(tempTab);
        if (idx != -1) {
            m_tabWidget->setTabText(idx, QFileInfo(newFilePath).fileName());
        }
    });
}

void MainWindow::tryAutoLoadProject()
{
    // Get current working directory (project root)
    QDir currentDir = QDir::current();
    if (!currentDir.cdUp()) {
        qWarning() << "Failed to go up from current directory";
    }

    // Check for "VK" folder inside current directory
    QString vkFolderName = "VK";
    if (!currentDir.exists(vkFolderName)) {
        qDebug() << "VK folder not found in project root.";
        return;
    }

    QDir vkDir(currentDir.filePath(vkFolderName));

    // Find all .json files in VK folder (non-recursive)
    QStringList filters;
    filters << "*.json";
    QFileInfoList jsonFiles = vkDir.entryInfoList(filters, QDir::Files | QDir::NoSymLinks);

    if (jsonFiles.size() == 1) {
        QString projectFilePath = jsonFiles.first().absoluteFilePath();
        qDebug() << "Auto-loading project file:" << projectFilePath;

        if (m_project)
            delete m_project;

        m_project = new Project();
        if (!m_project->load(projectFilePath)) {
            QMessageBox::warning(this, "Error", "Failed to auto-load project file.");
            delete m_project;
            m_project = nullptr;
            return;
        }

        loadProjectDataToUi();
        refreshSessionList();
        statusBar()->showMessage("Project auto-loaded.");
    } else if (jsonFiles.isEmpty()) {
        qDebug() << "No project files found in VK folder.";
    } else {
        qDebug() << "Multiple project files found in VK folder; skipping auto-load.";
    }
}

void MainWindow::updateBackendConfigForAllSessions()
{
    if (!m_project)
        return;

    QVariantMap config;
    config["access_token"] = m_project->accessToken();
    config["model"] = m_project->model();
    config["max_tokens"] = m_project->maxTokens();
    config["temperature"] = m_project->temperature();
    config["top_p"] = m_project->topP();
    config["frequency_penalty"] = m_project->frequencyPenalty();
    config["presence_penalty"] = m_project->presencePenalty();

    for (SessionTabWidget* tab : m_openSessions) {
        if (tab) {
            // Assuming you add a method to SessionTabWidget to update backend config
            tab->updateBackendConfig(config);
        }
    }
}

void MainWindow::onOpenProject()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Open Project File", QString(), "Project Files (*.json)");
    if (fileName.isEmpty())
        return;

    if (m_project)
        m_project->deleteLater();

    m_project = new Project();
    if (!m_project->load(fileName)) {
        QMessageBox::warning(this, "Error", "Failed to load project file.");
        delete m_project;
        m_project = nullptr;
        return;
    }

    loadProjectDataToUi();
    refreshSessionList();
    updateBackendConfigForAllSessions();

    statusBar()->showMessage("Project loaded.");
}

void MainWindow::loadProjectDataToUi()
{
    if (!m_project) {
        m_projectInfoLabel->setText("No project loaded");
        m_templateList->clear();
        m_sessionList->clear();
        return;
    }

    // Display project folder info (simple HTML)
    QString html;
    html += "<b>Project Root:</b> " + m_project->rootFolder() + "<br>";
    html += "<b>Docs Folder:</b> " + m_project->docsFolder() + "<br>";
    html += "<b>Source Folder:</b> " + m_project->srcFolder() + "<br>";
    html += "<b>Sessions Folder:</b> " + m_project->sessionsFolder() + "<br>";
    html += "<b>Templates Folder:</b> " + m_project->templatesFolder() + "<br>";
    html += "<b>Include Doc Folders:</b><ul>";
    for (const QString &incFolder : m_project->includeDocFolders()) {
        html += "<li>" + incFolder + "</li>";
    }
    html += "</ul>";

    m_projectInfoLabel->setText(html);

    // Populate template list
    m_templateList->clear();
    QDir templatesDir(m_project->templatesFolder());
    if (templatesDir.exists()) {
        QStringList filters{"*.md", "*.markdown"};
        QFileInfoList files = templatesDir.entryInfoList(filters, QDir::Files);
        for (const QFileInfo &fi : files)
            m_templateList->addItem(fi.fileName());
    }

    if (m_projectSettingsBtn)
        m_projectSettingsBtn->setEnabled(true);
}

void MainWindow::onProjectSettingsClicked()
{
    if (!m_project)
        return;

    ProjectSettingsDialog dlg(this);

    // Load current project settings into dialog
    dlg.loadSettings(
        QVariantMap{
            {"access_token", m_project->accessToken()},
            {"model", m_project->model()},
            {"max_tokens", m_project->maxTokens()},
            {"temperature", m_project->temperature()},
            {"top_p", m_project->topP()},
            {"frequency_penalty", m_project->frequencyPenalty()},
            {"presence_penalty", m_project->presencePenalty()}
        },
        QVariantMap{
            {"root", m_project->rootFolder()},
            {"docs", m_project->docsFolder()},
            {"src", m_project->srcFolder()},
            {"sessions", m_project->sessionsFolder()},
            {"templates", m_project->templatesFolder()},
            {"include_docs", QVariant::fromValue(m_project->includeDocFolders())}
        },
        m_project->sourceFileTypes(),
        m_project->docFileTypes(),
        m_project->commandPipes()
        );

    if (dlg.exec() == QDialog::Accepted) {
        QVariantMap apiSettings;
        QVariantMap folders;
        QStringList sourceFileTypes;
        QStringList docFileTypes;
        QMap<QString, QStringList> commandPipes;

        dlg.getSettings(apiSettings, folders, sourceFileTypes, docFileTypes, commandPipes);

        // Apply settings back to project
        m_project->setAccessToken(apiSettings.value("access_token").toString());
        m_project->setModel(apiSettings.value("model").toString());
        m_project->setMaxTokens(apiSettings.value("max_tokens").toInt());
        m_project->setTemperature(apiSettings.value("temperature").toDouble());
        m_project->setTopP(apiSettings.value("top_p").toDouble());
        m_project->setFrequencyPenalty(apiSettings.value("frequency_penalty").toDouble());
        m_project->setPresencePenalty(apiSettings.value("presence_penalty").toDouble());

        m_project->setRootFolder(folders.value("root").toString());
        m_project->setDocsFolder(folders.value("docs").toString());
        m_project->setSrcFolder(folders.value("src").toString());
        m_project->setSessionsFolder(folders.value("sessions").toString());
        m_project->setTemplatesFolder(folders.value("templates").toString());

        // include_docs is QStringList stored as QVariant
        QVariant includeDocsVar = folders.value("include_docs");
        if (includeDocsVar.canConvert<QStringList>())
            m_project->setIncludeDocFolders(includeDocsVar.toStringList());
        else if (includeDocsVar.canConvert<QVariantList>()) {
            QStringList list;
            for (const QVariant &v : includeDocsVar.toList())
                list.append(v.toString());
            m_project->setIncludeDocFolders(list);
        }

        m_project->setSourceFileTypes(sourceFileTypes);
        m_project->setDocFileTypes(docFileTypes);
        m_project->setCommandPipes(commandPipes);

        // Update UI to reflect changes
        loadProjectDataToUi();

        // Optionally save project file if you have save implemented
        // m_project->save(...);

        // Update backend config for open sessions if needed
        updateBackendConfigForAllSessions();
    }
}

void MainWindow::refreshSessionList()
{
    m_sessionList->clear();
    if (!m_project)
        return;

    QDir sessionsDir(m_project->sessionsFolder());
    if (!sessionsDir.exists())
        return;

    QStringList filters{"*.md", "*.markdown"};
    QFileInfoList files = sessionsDir.entryInfoList(filters, QDir::Files, QDir::Time);
    for (const QFileInfo &fi : files)
        m_sessionList->addItem(fi.fileName());
}

void MainWindow::onCreateSessionFromTemplate()
{
    if (!m_project) {
        QMessageBox::warning(this, "No Project", "Load a project before creating a session.");
        return;
    }

    QListWidgetItem *selItem = m_templateList->currentItem();
    if (!selItem) {
        QMessageBox::warning(this, "No Template", "Select a template first.");
        return;
    }

    QString templateName = selItem->text();
    QString templatePath = QDir(m_project->templatesFolder()).filePath(templateName);

    QFile templateFile(templatePath);
    if (!templateFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Error", "Failed to open template.");
        return;
    }

    QString templateContent = QString::fromUtf8(templateFile.readAll());
    templateFile.close();

    QDir sessionsDir(m_project->sessionsFolder());
    if (!sessionsDir.exists())
        sessionsDir.mkpath(".");

    // Find next unused session number (001, 002, ...)
    int sessionNumber = 1;
    QString filename;
    do {
        filename = QString("%1.md").arg(sessionNumber, 3, 10, QChar('0'));
        ++sessionNumber;
    } while (sessionsDir.exists(filename));

    QString newSessionPath = sessionsDir.filePath(filename);

    QFile newSessionFile(newSessionPath);
    if (!newSessionFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Error", "Failed to create new session file.");
        return;
    }

    QTextStream out(&newSessionFile);
    out << templateContent;
    newSessionFile.close();

    // Open newly created session tab
    SessionTabWidget* tab = m_tabManager->openSession(newSessionPath);
    m_tabWidget->setCurrentWidget(tab);

    refreshSessionList();
    statusBar()->showMessage(QString("Created session %1").arg(filename));
}

void MainWindow::onOpenSelectedSession()
{
    if (!m_project) {
        QMessageBox::warning(this, "No Project", "Load a project before opening sessions.");
        return;
    }
    QListWidgetItem* selItem = m_sessionList->currentItem();
    if (!selItem) {
        QMessageBox::warning(this, "No Session", "Select a session to open.");
        return;
    }

    QString sessionName = selItem->text();
    QString sessionPath = QDir(m_project->sessionsFolder()).filePath(sessionName);

    SessionTabWidget* tab = m_tabManager->openSession(sessionPath);
    m_tabWidget->setCurrentWidget(tab);

    statusBar()->showMessage(QString("Opened session %1").arg(sessionName));
}


