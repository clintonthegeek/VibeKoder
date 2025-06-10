#include "mainwindow.h"
#include "projectsettingsdialog.h"
#include "sessiontabwidget.h"
#include "detachedwindow.h"
#include "appconfig.h"
#include "project.h"

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
#include <QFileInfo>
#include <QDir>
#include <QMessageBox>
#include <QDebug>
#include <QStandardPaths>

void setProjectSettingsFromMap(Project* project, const QVariantMap& map, const QString& prefix = QString())
{
    for (auto it = map.constBegin(); it != map.constEnd(); ++it) {
        QString key = prefix.isEmpty() ? it.key() : prefix + "." + it.key();
        if (it.value().type() == QVariant::Map) {
            setProjectSettingsFromMap(project, it.value().toMap(), key);
        } else {
            project->setValue(key, it.value());
        }
    }
}

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

    QAction* newProjectAction = new QAction("New Project", this);
    projectMenu->addAction(newProjectAction);
    connect(newProjectAction, &QAction::triggered, this, &MainWindow::onNewProject);

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

void MainWindow::onNewProject()
{
    // 1. Ask user for new project file path
    QString filePath = QFileDialog::getSaveFileName(this,
                                                    "Create New Project File",
                                                    QDir::homePath(),
                                                    "Project Files (*.json);;All Files (*)");
    if (filePath.isEmpty())
        return;

    QFileInfo fi(filePath);
    QDir projectRootDir = fi.dir();

    // 2. Ensure project root folder exists (the folder containing the project file)
    if (!projectRootDir.exists()) {
        if (!projectRootDir.mkpath(".")) {
            QMessageBox::warning(this, "Error", "Failed to create project directory.");
            return;
        }
    }

    // 3. Load app defaults from AppConfig
    auto& appConfig = AppConfig::instance();
    if (!appConfig.load()) {
        QMessageBox::warning(this, "Error", "Failed to load application configuration.");
        return;
    }

    // Replace with dynamic access to AppConfig via ConfigManager
    QVariantMap apiSettings = appConfig.getValue("default_project_settings.api").toMap();
    QVariantMap folders = appConfig.getValue("default_project_settings.folders").toMap();
    QStringList sourceFileTypes = appConfig.getValue("default_project_settings.filetypes.source").toStringList();
    QStringList docFileTypes = appConfig.getValue("default_project_settings.filetypes.docs").toStringList();

    // Convert command_pipes QVariantMap to QMap<QString, QStringList>
    QVariantMap cmdPipeMap = appConfig.getValue("default_project_settings.command_pipes").toMap();
    QMap<QString, QStringList> commandPipes;
    for (auto it = cmdPipeMap.constBegin(); it != cmdPipeMap.constEnd(); ++it) {
        commandPipes.insert(it.key(), it.value().toStringList());
    }

    // 4. Override root folder default with the directory of the chosen project file
    folders["root"] = projectRootDir.absolutePath();

    // 5. Show ProjectSettingsDialog initialized with combined settings map
    ProjectSettingsDialog dlg(this);

    // Combine all initial settings into a nested QVariantMap
    QVariantMap initialSettings;

    // Insert api settings
    initialSettings.insert("api", apiSettings);

    // Insert folders settings
    initialSettings.insert("folders", folders);

    // Insert filetypes settings
    initialSettings.insert("filetypes", QVariantMap{
                                            {"source", QVariant::fromValue(sourceFileTypes)},
                                            {"docs", QVariant::fromValue(docFileTypes)}
                                        });

    // Insert command_pipes settings
    initialSettings.insert("command_pipes", QVariantMap());
    for (auto it = commandPipes.constBegin(); it != commandPipes.constEnd(); ++it) {
        initialSettings["command_pipes"].toMap().insert(it.key(), QVariant::fromValue(it.value()));
    }

    // Load initial settings into dialog
    dlg.loadSettings(initialSettings);

    if (dlg.exec() != QDialog::Accepted)
        return;

    // 6. Extract updated nested settings from dialog
    QVariantMap updatedSettings = dlg.getSettings();

    // Helper function to flatten and apply nested settings to Project
    auto applySettingsToProject = [&](const QVariantMap &map) {
        std::function<void(const QVariantMap&, const QString&)> setRec;
        setRec = [&](const QVariantMap &m, const QString &prefix) {
            for (auto it = m.constBegin(); it != m.constEnd(); ++it) {
                QString key = prefix.isEmpty() ? it.key() : prefix + "." + it.key();
                if (it.value().type() == QVariant::Map) {
                    setRec(it.value().toMap(), key);
                } else {
                    m_project->setValue(key, it.value());
                }
            }
        };
        setRec(map, QString());
    };
    applySettingsToProject(updatedSettings);

    // 7. Resolve absolute root folder path from updated folders map
    QString rootFolder = m_project->getValue("folders.root").toString();
    if (!QDir(rootFolder).exists()) {
        if (!QDir().mkpath(rootFolder)) {
            QMessageBox::warning(this, "Error", "Failed to create root folder.");
            return;
        }
    }

    // 8. Create subfolders relative to root
    QString docsFolder = QDir(rootFolder).filePath(folders.value("docs").toString());
    QString templatesFolder = QDir(rootFolder).filePath(folders.value("templates").toString());
    QString srcFolder = QDir(rootFolder).filePath(folders.value("src").toString());
    QString sessionsFolder = QDir(rootFolder).filePath(folders.value("sessions").toString());
    QDir().mkpath(docsFolder);
    QDir().mkpath(templatesFolder);
    QDir().mkpath(srcFolder);
    QDir().mkpath(sessionsFolder);

    // 9. Copy default docs and templates from system-wide config folder
    QString configFolder = appConfig.configFolder(); // e.g. ~/.config/VibeKoder
    QString defaultDocsSrc = QDir(configFolder).filePath("docs");
    QString defaultTemplatesSrc = QDir(configFolder).filePath("templates");
    auto copyFolderContents = [](const QString& srcDirPath, const QString& destDirPath) {
        QDir srcDir(srcDirPath);
        QDir destDir(destDirPath);
        if (!srcDir.exists())
            return;
        if (!destDir.exists())
            destDir.mkpath(".");
        for (const QString& fileName : srcDir.entryList(QDir::Files)) {
            QString srcFile = srcDir.filePath(fileName);
            QString destFile = destDir.filePath(fileName);
            if (!QFile::exists(destFile)) {
                QFile::copy(srcFile, destFile);
            }
        }
    };
    copyFolderContents(defaultDocsSrc, docsFolder);
    copyFolderContents(defaultTemplatesSrc, templatesFolder);

    // 10. Create Project instance on heap and load it to initialize ConfigManager
    Project* newProject = new Project(this);
    if (!newProject->load(filePath)) {
        QMessageBox::warning(this, "Error", "Failed to load new project file.");
        delete newProject;
        return;
    }

    // 11. Set values dynamically
    newProject->setValue("api.access_token", apiSettings.value("access_token"));
    newProject->setValue("api.model", apiSettings.value("model"));
    newProject->setValue("api.max_tokens", apiSettings.value("max_tokens"));
    newProject->setValue("api.temperature", apiSettings.value("temperature"));
    newProject->setValue("api.top_p", apiSettings.value("top_p"));
    newProject->setValue("api.frequency_penalty", apiSettings.value("frequency_penalty"));
    newProject->setValue("api.presence_penalty", apiSettings.value("presence_penalty"));

    newProject->setValue("folders.root", rootFolder);
    newProject->setValue("folders.docs", docsFolder);
    newProject->setValue("folders.templates", templatesFolder);
    newProject->setValue("folders.src", srcFolder);
    newProject->setValue("folders.sessions", sessionsFolder);

    // Handle include_docs as QStringList
    QStringList includeDocs;
    QVariant incDocsVar = folders.value("include_docs");
    if (incDocsVar.canConvert<QStringList>())
        includeDocs = incDocsVar.toStringList();
    else if (incDocsVar.canConvert<QVariantList>()) {
        for (const QVariant& v : incDocsVar.toList())
            includeDocs.append(v.toString());
    }
    newProject->setValue("folders.include_docs", QVariant::fromValue(includeDocs));

    newProject->setValue("filetypes.source", QVariant::fromValue(sourceFileTypes));
    newProject->setValue("filetypes.docs", QVariant::fromValue(docFileTypes));
    newProject->setValue("command_pipes", QVariant::fromValue(commandPipes));

    // 12. Save the new project file
    if (!newProject->save(filePath)) {
        QMessageBox::warning(this, "Error", "Failed to save new project file.");
        delete newProject;
        return;
    }

    // 13. Close all open session tabs before switching projects
    if (m_tabManager) {
        auto openSessions = m_tabManager->openSessions();
        for (const QString& sessionPath : openSessions.keys()) {
            m_tabManager->closeSession(sessionPath);
        }
    }

    // 14. Replace current project pointer with new project
    if (m_project)
        delete m_project;
    m_project = newProject;

    if (m_tabManager) {
        m_tabManager->setProject(m_project);
    }

    loadProjectDataToUi();
    refreshSessionList();
    updateBackendConfigForAllSessions();
    statusBar()->showMessage("New project created and loaded.");
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

    if (m_tabManager) {
        auto openSessions = m_tabManager->openSessions();
        for (const QString& sessionPath : openSessions.keys()) {
            m_tabManager->closeSession(sessionPath);
        }
    }

    if (m_project)
        m_project->deleteLater();

    m_project = new Project();
    if (!m_project->load(fileName)) {
        QMessageBox::warning(this, "Error", "Failed to load project file.");
        delete m_project;
        m_project = nullptr;
        return;
    }
    if (m_tabManager) {
        m_tabManager->setProject(m_project);
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

    // Helper lambda to resolve relative folder paths to absolute
    auto resolvePath = [&](const QString& folder) -> QString {
        if (QDir(folder).isAbsolute())
            return QDir::cleanPath(folder);
        return QDir(m_project->rootFolder()).filePath(folder);
    };

    QString rootFolder = m_project->rootFolder();
    QString docsFolder = resolvePath(m_project->getValue("folders.docs").toString());
    QString srcFolder = resolvePath(m_project->srcFolder());
    QString sessionsFolder = resolvePath(m_project->sessionsFolder());
    QString templatesFolder = resolvePath(m_project->templatesFolder());
    QStringList includeDocFolders = m_project->getValue("folders.include_docs").toStringList();

    // Display project folder info (simple HTML)
    QString html;
    html += "<b>Project Root:</b> " + rootFolder + "<br>";
    html += "<b>Docs Folder:</b> " + docsFolder + "<br>";
    html += "<b>Source Folder:</b> " + srcFolder + "<br>";
    html += "<b>Sessions Folder:</b> " + sessionsFolder + "<br>";
    html += "<b>Templates Folder:</b> " + templatesFolder + "<br>";
    html += "<b>Include Doc Folders:</b><ul>";
    for (const QString &incFolder : includeDocFolders) {
        QString absIncFolder = resolvePath(incFolder);
        html += "<li>" + absIncFolder + "</li>";
    }
    html += "</ul>";

    m_projectInfoLabel->setText(html);

    // Populate template list
    m_templateList->clear();
    QDir templatesDir(templatesFolder);
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
    dlg.loadSettings(m_project->configManager()->configObject().toVariantMap());

    if (dlg.exec() == QDialog::Accepted) {
        QVariantMap newSettings = dlg.getSettings();

        // Apply nested settings into Project using helper
        setProjectSettingsFromMap(m_project, newSettings);

        if (!m_project->save(m_project->projectFilePath())) {
            QMessageBox::warning(this, "Save Failed", "Failed to save project file.");
        } else {
            statusBar()->showMessage("Project saved successfully.", 3000);
        }

        loadProjectDataToUi();
        updateBackendConfigForAllSessions();
    }
}

void MainWindow::refreshSessionList()
{
    m_sessionList->clear();
    if (!m_project)
        return;

    // Helper lambda to resolve relative folder paths to absolute
    auto resolvePath = [&](const QString& folder) -> QString {
        if (QDir(folder).isAbsolute())
            return QDir::cleanPath(folder);
        return QDir(m_project->rootFolder()).filePath(folder);
    };

    QString sessionsFolder = resolvePath(m_project->getValue("folders.sessions").toString());

    QDir sessionsDir(sessionsFolder);
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

    // Resolve templates folder path relative to project root
    auto resolvePath = [&](const QString& folder) -> QString {
        if (QDir(folder).isAbsolute())
            return QDir::cleanPath(folder);
        return QDir(m_project->rootFolder()).filePath(folder);
    };
    QString templatesFolder = resolvePath(m_project->getValue("folders.templates").toString());
    QString templatePath = QDir(templatesFolder).filePath(templateName);

    QFile templateFile(templatePath);
    if (!templateFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Error", "Failed to open template.");
        return;
    }

    QString templateContent = QString::fromUtf8(templateFile.readAll());
    templateFile.close();

    QString sessionsFolder = resolvePath(m_project->getValue("folders.sessions").toString());
    QDir sessionsDir(sessionsFolder);
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

    // Resolve sessions folder path relative to project root
    auto resolvePath = [&](const QString& folder) -> QString {
        if (QDir(folder).isAbsolute())
            return QDir::cleanPath(folder);
        return QDir(m_project->rootFolder()).filePath(folder);
    };
    QString sessionsFolder = resolvePath(m_project->getValue("folders.sessions").toString());

    QString sessionPath = QDir(sessionsFolder).filePath(sessionName);

    SessionTabWidget* tab = m_tabManager->openSession(sessionPath);
    m_tabWidget->setCurrentWidget(tab);

    statusBar()->showMessage(QString("Opened session %1").arg(sessionName));
}


