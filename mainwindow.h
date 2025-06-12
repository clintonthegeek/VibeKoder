#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMap>
#include "draggabletabwidget.h"

class QLabel;
class QListWidget;
class QPushButton;
class QTreeWidget;

#include "project.h"
#include "sessiontabwidget.h"
#include "tabmanager.h"


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onOpenProject();
    void onNewTempSession();
    void onNewProject();

    void onApplicationSettings();

    void onCreateSessionFromTemplate();
    void onOpenSelectedSession();

private:
    void setupUi();
    void loadProjectDataToUi();
    void refreshSessionList();
    void tryAutoLoadProject();
    void updateBackendConfigForAllSessions();
    void onProjectSettingsClicked();

    void onDescribeSelectedSession();
    void onDeleteSelectedSession();

    bool m_verticalTabs = false;
    void toggleVerticalTabs();

    Project* m_project = nullptr;

    // UI widgets
    DraggableTabWidget* m_tabWidget = nullptr;


    // Project tab widgets
    QWidget* m_projectTab = nullptr;
    QLabel* m_projectInfoLabel = nullptr;
    QListWidget* m_templateList = nullptr;
    QTreeWidget* m_sessionList = nullptr;
    QPushButton* m_createSessionBtn = nullptr;
    QPushButton* m_openSessionBtn = nullptr;
    QPushButton* m_projectSettingsBtn = nullptr;
    QPushButton* m_deleteSessionBtn = nullptr;
    QPushButton* m_describeSessionBtn = nullptr;

    TabManager* m_tabManager = nullptr;

    // Map session file paths to session tab widgets (avoid duplicates)
    QMap<QString, SessionTabWidget*> m_openSessions;

    QMap<QString, SessionTabWidget*> m_pendingSessions;
};

#endif // MAINWINDOW_H
