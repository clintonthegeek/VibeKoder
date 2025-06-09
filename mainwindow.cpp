#include "mainwindow.h"
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

    QMenu* sessionMenu = menuBar()->addMenu("Sessions");
    QAction* openSessionAction = new QAction("Open Session", this);
    QAction* saveSessionAction = new QAction("Save Session", this);
    sessionMenu->addAction(openSessionAction);
    sessionMenu->addAction(saveSessionAction);
    connect(openSessionAction, &QAction::triggered, this, &MainWindow::onOpenSession);
    connect(saveSessionAction, &QAction::triggered, this, &MainWindow::onSaveSession);

    QAction* dumpSessionsAction = new QAction("Dump Sessions", this);
    dumpSessionsAction->setToolTip("Dump current open sessions to debug output");
    connect(dumpSessionsAction, &QAction::triggered, this, &MainWindow::dumpOpenSessions);

    QToolBar* toolbar = addToolBar("Main Toolbar");
    toolbar->addAction(openProjectAction);
    toolbar->addAction(openSessionAction);
    toolbar->addAction(saveSessionAction);
    toolbar->addAction(dumpSessionsAction);  // Add here

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

        m_tabWidget->removeTab(index);
        widget->deleteLater();

        // Remove from m_openSessions
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

    // Find all .toml files in VK folder (non-recursive)
    QStringList filters;
    filters << "*.toml";
    QFileInfoList tomlFiles = vkDir.entryInfoList(filters, QDir::Files | QDir::NoSymLinks);

    if (tomlFiles.size() == 1) {
        QString projectFilePath = tomlFiles.first().absoluteFilePath();
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
    } else if (tomlFiles.isEmpty()) {
        qDebug() << "No project files found in VK folder.";
    } else {
        qDebug() << "Multiple project files found in VK folder; skipping auto-load.";
    }
}

void MainWindow::dumpOpenSessions() const
{
    qDebug() << "=== Dumping Open Sessions ===";
    for (auto it = m_openSessions.begin(); it != m_openSessions.end(); ++it) {
        QString key = it.key();
        QWidget* w = it.value();
        qDebug() << "Session:" << key << ", Widget:" << w;
        if (w) {
            QWidget* parent = w->parentWidget();
            qDebug() << "  Parent chain:";
            while (parent) {
                qDebug() << "   " << parent;
                parent = parent->parentWidget();
            }
        }
    }
    int projIndex = m_tabWidget->indexOf(m_projectTab);
    qDebug() << "[MainWindow] Project tab index:" << projIndex
             << "Tab count:" << m_tabWidget->count();
    if (projIndex == -1) {
        qWarning() << "[MainWindow] Project tab is missing from main tab widget!";
    }
    qDebug() << "=== End Dump ===";
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
    QString fileName = QFileDialog::getOpenFileName(this, "Open Project File", QString(), "Project Files (*.toml)");
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

void MainWindow::onOpenSession()
{
    // Optional: implement dialog for arbitrary session file selection
}

void MainWindow::onSaveSession()
{
    if (m_openSessions.isEmpty())
        return;

    QWidget* currentWidget = m_tabWidget->currentWidget();
    if (!currentWidget)
        return;

    SessionTabWidget *tab = qobject_cast<SessionTabWidget*>(currentWidget);
    if (tab) {
        if (!tab->saveSession())
            QMessageBox::warning(this, "Save Failed", "Failed to save the current session.");
        else
            statusBar()->showMessage("Session saved.");
    }
}

