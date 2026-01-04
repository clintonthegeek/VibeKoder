#include "mainwindow.h"
#include "descriptiongenerator.h"
#include "projectsettingsdialog.h"
#include "sessiontabwidget.h"
#include "detachedwindow.h"
#include "appconfig.h"
#include "applicationsettingsdialog.h"
#include "project.h"
#include "openaibackend.h"
#include "session.h"

#include <QMenuBar>
#include <QMenu>
#include <QToolBar>
#include <QStatusBar>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QTreeWidget>
#include <QPushButton>
#include <QFileDialog>
#include <QFileInfo>
#include <QDir>
#include <QMessageBox>
#include <QDebug>
#include <QStandardPaths>
#include <QRegularExpression>

static QString wrapText(const QString &text, int maxLength = 80)
{
    QString result;
    int start = 0;
    int len = text.length();

    while (start < len) {
        int end = start + maxLength;
        if (end >= len) {
            result += text.mid(start);
            break;
        }

        int breakPos = -1;
        for (int i = end; i > start; --i) {
            QChar c = text.at(i);
            if (c.isSpace() || c == QChar('-')) {
                breakPos = i;
                break;
            }
        }
        if (breakPos == -1)
            breakPos = end;

        result += text.mid(start, breakPos - start + 1).trimmed() + '\n';
        start = breakPos + 1;
    }

    return result;
}

void setProjectSettingsFromMap(Project* project, const QVariantMap& map, const QString& prefix = QString())
{
    for (auto it = map.constBegin(); it != map.constEnd(); ++it) {
        QString key = prefix.isEmpty() ? it.key() : prefix + "." + it.key();
        if (it.value().typeId() == QMetaType::QVariantMap) {
            setProjectSettingsFromMap(project, it.value().toMap(), key);
        } else {
            project->setValue(key, it.value());
        }
    }
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    this->resize(700, 1200);
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

AIBackend* createBackendForProject(Project* project, QObject* parent = nullptr)
{
    OpenAIBackend* backend = new OpenAIBackend(parent);
    if (project) {
        QVariantMap apiConfig;
        apiConfig["access_token"] = project->accessToken();
        apiConfig["model"] = project->model();
        apiConfig["max_tokens"] = project->maxTokens();
        apiConfig["temperature"] = project->temperature();
        apiConfig["top_p"] = project->topP();
        apiConfig["frequency_penalty"] = project->frequencyPenalty();
        apiConfig["presence_penalty"] = project->presencePenalty();
        backend->setConfig(apiConfig);
    }
    return backend;
}

void MainWindow::setupUi()
{
    setWindowTitle("VibeKoder");

    // === Menu and Toolbar ===
    QMenu* appMenu = menuBar()->addMenu("Application");
    QAction* appPreferencesAction = new QAction("Preferences...", this);
    appPreferencesAction->setToolTip("Configure default settings for new projects");
    appMenu->addAction(appPreferencesAction);
    connect(appPreferencesAction, &QAction::triggered, this, &MainWindow::onApplicationSettings);

    QMenu* projectMenu = menuBar()->addMenu("Project");

    QAction* newProjectAction = new QAction("New Project...", this);
    projectMenu->addAction(newProjectAction);
    connect(newProjectAction, &QAction::triggered, this, &MainWindow::onNewProject);

    QAction* openProjectAction = new QAction("Open Project...", this);
    projectMenu->addAction(openProjectAction);
    connect(openProjectAction, &QAction::triggered, this, &MainWindow::onOpenProject);

    projectMenu->addSeparator();

    m_projectSettingsAction = new QAction("Project Settings...", this);
    m_projectSettingsAction->setToolTip("Edit settings for the currently loaded project (API key, folders, etc.)");
    m_projectSettingsAction->setEnabled(false); // Disabled until project loads
    m_projectSettingsAction->setShortcut(QKeySequence("Ctrl+,"));
    projectMenu->addAction(m_projectSettingsAction);
    connect(m_projectSettingsAction, &QAction::triggered, this, &MainWindow::onProjectSettingsClicked);

    QAction* newTempSessionAction = new QAction("New Temp Session", this);
    newTempSessionAction->setShortcut(QKeySequence("Ctrl+T"));
    connect(newTempSessionAction, &QAction::triggered, this, &MainWindow::onNewTempSession);

    QToolBar* toolbar = addToolBar("Main Toolbar");
    toolbar->addAction(openProjectAction);
    toolbar->addAction(newTempSessionAction);
    toolbar->hide();

    QMenu* viewMenu = menuBar()->addMenu("View");

    QAction* toggleVerticalTabsAction = new QAction("Vertical Tabs", this);
    toggleVerticalTabsAction->setCheckable(true);
    toggleVerticalTabsAction->setChecked(m_verticalTabs);
    connect(toggleVerticalTabsAction, &QAction::triggered, this, [this, toggleVerticalTabsAction]() {
        m_verticalTabs = toggleVerticalTabsAction->isChecked();
        if (m_tabWidget)
            m_tabWidget->setTabOrientation(m_verticalTabs ? Qt::Vertical : Qt::Horizontal);
        // Update detached windows similarly if needed
    });
    viewMenu->addAction(toggleVerticalTabsAction);

    QAction* toggleToolbarAction = new QAction("Main Toolbar", this);
    toggleToolbarAction->setCheckable(true);
    toggleToolbarAction->setChecked(toolbar->isVisible());  // Initialize checked state properly
    connect(toggleToolbarAction, &QAction::toggled, toolbar, &QToolBar::setVisible);
    // Keep toolbar visibility changes in sync with the menu action
    connect(toolbar, &QToolBar::visibilityChanged, toggleToolbarAction, &QAction::setChecked);
    viewMenu->addAction(toggleToolbarAction);


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

    m_projectSettingsBtn = new QPushButton("Project Settings", m_projectTab);
    m_projectSettingsBtn->setEnabled(false); // Disabled until a project is loaded
    m_projectSettingsBtn->setToolTip("Edit settings for the currently loaded project\n(API key, folders, file types, etc.)");
    projLayout->addWidget(m_projectSettingsBtn);
    connect(m_projectSettingsBtn, &QPushButton::clicked, this, &MainWindow::onProjectSettingsClicked);

    projLayout->addWidget(new QLabel("Templates:"));
    m_templateList = new QListWidget();
    projLayout->addWidget(m_templateList);

    m_createSessionBtn = new QPushButton("Create New Session from Template");
    projLayout->addWidget(m_createSessionBtn);
    connect(m_createSessionBtn, &QPushButton::clicked, this, &MainWindow::onCreateSessionFromTemplate);

    projLayout->addSpacing(10);
    projLayout->addWidget(new QLabel("Available Sessions:"));
    m_sessionList = new QTreeWidget();
    m_sessionList->setColumnCount(4);
    m_sessionList->setHeaderLabels(QStringList() << "#" << "Name" << "Slices" << "Size (KB)");
    m_sessionList->setRootIsDecorated(false);
    m_sessionList->setSelectionMode(QAbstractItemView::SingleSelection);
    projLayout->addWidget(m_sessionList);
    connect(m_sessionList, &QTreeWidget::itemDoubleClicked, this, &MainWindow::onOpenSelectedSession);
    m_openSessionBtn = new QPushButton("Open Session");
    connect(m_openSessionBtn, &QPushButton::clicked, this, &MainWindow::onOpenSelectedSession);

    QHBoxLayout* sessionButtonsLayout = new QHBoxLayout();
    sessionButtonsLayout->addWidget(m_openSessionBtn);
    m_openSessionBtn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    m_describeSessionBtn = new QPushButton("Describe", m_projectTab);
    sessionButtonsLayout->addWidget(m_describeSessionBtn);

    sessionButtonsLayout->addStretch();

    m_deleteSessionBtn = new QPushButton("Delete", m_projectTab);
    sessionButtonsLayout->addWidget(m_deleteSessionBtn);

    projLayout->addLayout(sessionButtonsLayout);

    // Connect delete button
    connect(m_deleteSessionBtn, &QPushButton::clicked, this, &MainWindow::onDeleteSelectedSession);

    // Connect describe button
    connect(m_describeSessionBtn, &QPushButton::clicked, this, &MainWindow::onDescribeSelectedSession);





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

        auto sessionTab = qobject_cast<SessionTabWidget*>(widget);
        if (sessionTab) {
            if (!sessionTab->confirmDiscardUnsavedChanges()) {
                // Cancel close
                return;
            }
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

void MainWindow::toggleVerticalTabs()
{
    m_verticalTabs = !m_verticalTabs;
    if (m_tabWidget) {
        m_tabWidget->setTabOrientation(m_verticalTabs ? Qt::Vertical : Qt::Horizontal);
    }
    // Update detached windows
    for (auto dw : m_tabManager->detachedWindows()) {
        if (!dw)
            continue;
        auto tabWidget = dw->findChild<DraggableTabWidget*>();
        if (tabWidget) {
            tabWidget->setTabOrientation(m_verticalTabs ? Qt::Vertical : Qt::Horizontal);
        }
    }
}

// helper for merging changed settings with unchanged settings.
void mergeVariantMaps(QVariantMap &base, const QVariantMap &updates)
{
    for (auto it = updates.constBegin(); it != updates.constEnd(); ++it) {
        const QString &key = it.key();
        const QVariant &val = it.value();

        if (val.typeId() == QMetaType::QVariantMap && base.contains(key) && base.value(key).typeId() == QMetaType::QVariantMap) {
            QVariantMap baseSubMap = base.value(key).toMap();
            mergeVariantMaps(baseSubMap, val.toMap());
            base[key] = baseSubMap;
        } else {
            base[key] = val;
        }
    }
}

void MainWindow::onApplicationSettings()
{
    ApplicationSettingsDialog dlg(this);
    dlg.loadSettings(AppConfig::instance().data());

    if (dlg.exec() == QDialog::Accepted) {
        AppConfigData newData = dlg.getSettings();
        AppConfig::instance().data() = newData;

        if (!AppConfig::instance().save()) {
            QMessageBox::warning(this, "Save Failed", "Failed to save application configuration.");
        } else {
            statusBar()->showMessage("Application settings saved.", 3000);
        }
    }
}

void MainWindow::onDeleteSelectedSession()
{
    if (!m_project) {
        QMessageBox::warning(this, "No Project", "Load a project before deleting sessions.");
        return;
    }

    QTreeWidgetItem* selItem = m_sessionList->currentItem();
    if (!selItem) {
        QMessageBox::warning(this, "No Selection", "Select a session to delete.");
        return;
    }

    QString sessionPath = selItem->data(0, Qt::UserRole).toString();
    if (sessionPath.isEmpty()) {
        QMessageBox::warning(this, "Invalid Selection", "Selected session path is invalid.");
        return;
    }

    // Confirm deletion
    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "Delete Session",
        QString("Are you sure you want to delete session '%1'? This cannot be undone.").arg(QFileInfo(sessionPath).fileName()),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);

    if (reply != QMessageBox::Yes)
        return;

    // Close session tab if open
    if (m_tabManager) {
        m_tabManager->closeSession(sessionPath);
    }

    // Delete session file
    if (!QFile::remove(sessionPath)) {
        QMessageBox::warning(this, "Delete Failed", "Failed to delete session file.");
        return;
    }

    // Delete session cache folder if exists
    QFileInfo fi(sessionPath);
    QString cacheFolder = QDir(fi.absolutePath()).filePath(fi.completeBaseName());
    QDir cacheDir(cacheFolder);
    if (cacheDir.exists()) {
        cacheDir.removeRecursively();
    }

    refreshSessionList();
    statusBar()->showMessage("Session deleted.", 3000);
}

void MainWindow::onDescribeSelectedSession()
{
    if (!m_project) {
        QMessageBox::warning(this, "No Project", "Load a project before describing sessions.");
        return;
    }
    QTreeWidgetItem* selItem = m_sessionList->currentItem();
    if (!selItem) {
        QMessageBox::warning(this, "No Selection", "Select a session to describe.");
        return;
    }
    QString sessionPath = selItem->data(0, Qt::UserRole).toString();
    if (sessionPath.isEmpty()) {
        QMessageBox::warning(this, "Invalid Selection", "Selected session path is invalid.");
        return;
    }

    // Load session from file (no UI)
    Session* session = new Session(m_project, this);
    if (!session->load(sessionPath)) {
        QMessageBox::warning(this, "Error", "Failed to load session file.");
        delete session;
        return;
    }

    // Create AI backend configured for current project
    AIBackend* backend = createBackendForProject(m_project, this);

    // Create DescriptionGenerator with session and backend
    DescriptionGenerator* gen = new DescriptionGenerator(session, backend, this);

    // Connect signals to handle completion and errors
connect(gen, &DescriptionGenerator::generationFinished, this, [this, session, sessionPath, gen](const QString& title, const QString&){        // Save updated session metadata
        if (!session->save(sessionPath)) {
            QMessageBox::warning(this, "Save Failed", "Failed to save session after updating description.");
        }
        refreshSessionList();
        // Select updated session in list
        for (int i = 0; i < m_sessionList->topLevelItemCount(); ++i) {
            QTreeWidgetItem* item = m_sessionList->topLevelItem(i);
            if (item && item->data(0, Qt::UserRole).toString() == sessionPath) {
                m_sessionList->setCurrentItem(item);
                break;
            }
        }
        statusBar()->showMessage("Session description updated.", 3000);
        // Clean up
        gen->deleteLater();
        session->deleteLater();
    });
    connect(gen, &DescriptionGenerator::generationError, this, [this, gen, session](const QString& error){
        QMessageBox::warning(this, "Description Generation Error", error);
        gen->deleteLater();
        session->deleteLater();
    });

    // Start generation invisibly
    gen->generateTitleAndDescription();
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

    // Create a temporary Project with app-wide defaults
    Project* tempProject = new Project(this);

    // Load app-wide config map from AppConfig singleton
    QVariantMap appConfigMap = AppConfig::instance().getConfigMap();

    // Extract default_project_settings map
    QVariantMap defaultProjectSettings = appConfigMap.value("default_project_settings").toMap();

    // Apply default_project_settings into tempProject
    std::function<void(const QVariantMap&, const QString&)> setRec;
    setRec = [&](const QVariantMap& map, const QString& prefix) {
        for (auto it = map.constBegin(); it != map.constEnd(); ++it) {
            QString key = prefix.isEmpty() ? it.key() : prefix + "." + it.key();
            if (it.value().typeId() == QMetaType::QVariantMap) {
                setRec(it.value().toMap(), key);
            } else {
                tempProject->setValue(key, it.value());
            }
        }
    };
    setRec(defaultProjectSettings, QString());

    // Create the SessionTabWidget with the tempProject and mark it as a temp session
    SessionTabWidget* tempTab = new SessionTabWidget(tempFilePath, tempProject, m_tabWidget, true);

    // Add tab and select it
    m_tabWidget->addTab(tempTab, "Temp");
    m_tabWidget->setCurrentWidget(tempTab);

    // Connect to update tab text if temp session gets saved to a real file
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

    // 4. Get default settings and override root folder
    ProjectConfig initialConfig = appConfig.data().defaultProjectSettings;
    initialConfig.rootFolder = projectRootDir.absolutePath();

    // 5. Show ProjectSettingsDialog initialized with defaults
    ProjectSettingsDialog dlg(this);
    dlg.loadSettings(initialConfig);

    if (dlg.exec() != QDialog::Accepted)
        return;

    // 6. Get user-configured settings
    ProjectConfig newConfig = dlg.getSettings();

    // 7. Ensure root folder exists
    if (!QDir(newConfig.rootFolder).exists()) {
        if (!QDir().mkpath(newConfig.rootFolder)) {
            QMessageBox::warning(this, "Error", "Failed to create root folder.");
            return;
        }
    }

    // 8. Create subfolders relative to root
    QString rootFolder = newConfig.rootFolder;
    QString docsFolder = QDir(rootFolder).filePath(newConfig.docsFolder);
    QString templatesFolder = QDir(rootFolder).filePath(newConfig.templatesFolder);
    QString srcFolder = QDir(rootFolder).filePath(newConfig.srcFolder);
    QString sessionsFolder = QDir(rootFolder).filePath(newConfig.sessionsFolder);
    QDir().mkpath(docsFolder);
    QDir().mkpath(templatesFolder);
    QDir().mkpath(srcFolder);
    QDir().mkpath(sessionsFolder);

    // 9. Copy default docs and templates from system-wide config folder
    QString configFolder = appConfig.configFolder();
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
    copyFolderContents(defaultTemplatesSrc, templatesFolder);

    // 10. Create Project and set config
    Project* newProject = new Project(this);
    newProject->config() = newConfig;

    // 11. Save the new project file
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

    // Use TabManager's list of open sessions, not MainWindow's stale m_openSessions
    if (m_tabManager) {
        QMap<QString, SessionTabWidget*> openSessions = m_tabManager->openSessions();
        for (SessionTabWidget* tab : openSessions) {
            if (tab) {
                qDebug() << "[MainWindow] Updating backend config for session:" << tab;
                tab->updateBackendConfig(config);
            }
        }
        qDebug() << "[MainWindow] Updated backend config for" << openSessions.size() << "open sessions";
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

    if (m_projectSettingsAction)
        m_projectSettingsAction->setEnabled(true);
}

void MainWindow::onProjectSettingsClicked()
{
    if (!m_project)
        return;

    ProjectSettingsDialog dlg(this);
    dlg.loadSettings(m_project->config());

    if (dlg.exec() == QDialog::Accepted) {
        ProjectConfig newConfig = dlg.getSettings();
        m_project->config() = newConfig;

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
    if (!m_sessionList)
        return;

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
    QFileInfoList files = sessionsDir.entryInfoList(filters, QDir::Files, QDir::Name);

    struct SessionInfo {
        QTreeWidgetItem* item;
        QDateTime lastSliceTimestamp;
    };

    QList<SessionInfo> sessionItems;

    QRegularExpression delimiterRe(
        R"(^==\{\s*(System|User|Assistant)\s*(?:\|\s*([0-9]{4}-[0-9]{2}-[0-9]{2} [0-9]{2}:[0-9]{2}:[0-9]{2}))?\s*\}==\s*$)",
        QRegularExpression::MultilineOption | QRegularExpression::CaseInsensitiveOption);

    for (const QFileInfo &fi : files) {
        QString fileName = fi.fileName();
        QString baseName = fi.completeBaseName(); // e.g. "001"

        // Trim leading zeros for session number display
        QString sessionNumber = baseName;
        sessionNumber.remove(QRegularExpression("^0+"));
        if (sessionNumber.isEmpty())
            sessionNumber = "0";

        // Open file and parse slices and last timestamp
        int sliceCount = 0;
        QDateTime lastTimestamp;
        QString title;
        QString description;

        QFile file(fi.absoluteFilePath());
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            QString content = in.readAll();
            file.close();

            QRegularExpressionMatchIterator it = delimiterRe.globalMatch(content);
            while (it.hasNext()) {
                QRegularExpressionMatch match = it.next();
                ++sliceCount;

                QString tsStr = match.captured(2);
                if (!tsStr.isEmpty()) {
                    QDateTime ts = QDateTime::fromString(tsStr, "yyyy-MM-dd HH:mm:ss");
                    if (ts.isValid() && (lastTimestamp.isNull() || ts > lastTimestamp)) {
                        lastTimestamp = ts;
                    }
                }
            }

            QRegularExpression yamlRe(R"(^---\s*\n(.*?)\n---\s*\n)", QRegularExpression::DotMatchesEverythingOption);
            QRegularExpressionMatch match = yamlRe.match(content);
            if (match.hasMatch()) {
                QString yamlBlock = match.captured(1);

                // Parse title
                QRegularExpression titleRe(R"(^title:\s*(.*)$)", QRegularExpression::MultilineOption);
                QRegularExpressionMatch titleMatch = titleRe.match(yamlBlock);
                if (titleMatch.hasMatch()) {
                    title = titleMatch.captured(1).trimmed();
                    // Remove quotes if any
                    if ((title.startsWith('"') && title.endsWith('"')) || (title.startsWith('\'') && title.endsWith('\''))) {
                        title = title.mid(1, title.length() - 2);
                    }
                }

                // Parse description
                QRegularExpression descRe(R"(^description:\s*(.*)$)", QRegularExpression::MultilineOption);
                QRegularExpressionMatch descMatch = descRe.match(yamlBlock);
                if (descMatch.hasMatch()) {
                    description = descMatch.captured(1).trimmed();
                    // Remove quotes if any
                    if ((description.startsWith('"') && description.endsWith('"')) || (description.startsWith('\'') && description.endsWith('\''))) {
                        description = description.mid(1, description.length() - 2);
                    }
                }
            }
        }

        // File size in KB
        qint64 sizeBytes = fi.size();
        double sizeKB = sizeBytes / 1024.0;

        // Create tree widget item
        QTreeWidgetItem *item = new QTreeWidgetItem();
        item->setText(0, sessionNumber);
        item->setText(1, title); // Name column empty for now
        item->setText(2, QString::number(sliceCount));
        item->setText(3, QString::number(sizeKB, 'f', 2));
        item->setText(4, lastTimestamp.isNull() ? "" : lastTimestamp.toString("yyyy-MM-dd HH:mm:ss"));

        // Store full file path for retrieval on selection
        item->setData(0, Qt::UserRole, fi.absoluteFilePath());

        // Set tooltip on "Name" column (index 1) with wrapped description if not empty
        if (!description.isEmpty()) {
            item->setToolTip(1, wrapText(description));
        }

        sessionItems.append({item, lastTimestamp});
    }

    // Sort by last slice timestamp descending (most recent first)
    std::sort(sessionItems.begin(), sessionItems.end(), [](const SessionInfo &a, const SessionInfo &b) {
        return a.lastSliceTimestamp > b.lastSliceTimestamp;
    });

    // Set column count to 5 for new column
    m_sessionList->setColumnCount(5);
    m_sessionList->setHeaderLabels(QStringList() << "#" << "Name" << "Slices" << "Size (KB)" << "Last Modified");
    m_sessionList->setRootIsDecorated(false);

    // Add items to tree widget
    for (const SessionInfo &info : sessionItems) {
        m_sessionList->addTopLevelItem(info.item);
    }

    // Resize columns to contents
    for (int col = 0; col < m_sessionList->columnCount(); ++col) {
        m_sessionList->resizeColumnToContents(col);
    }
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
    QTreeWidgetItem* selItem = m_sessionList->currentItem();
    if (!selItem) {
        QMessageBox::warning(this, "No Session", "Select a session to open.");
        return;
    }

    QString sessionPath = selItem->data(0, Qt::UserRole).toString();
    if (sessionPath.isEmpty()) {
        QMessageBox::warning(this, "No Session", "Invalid session file path.");
        return;
    }

    SessionTabWidget* tab = m_tabManager->openSession(sessionPath);
    m_tabWidget->setCurrentWidget(tab);

    statusBar()->showMessage(QString("Opened session %1").arg(QFileInfo(sessionPath).fileName()));
}


