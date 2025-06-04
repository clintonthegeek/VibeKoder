#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMap>
#include "draggabletabwidget.h"

class QLabel;
class QListWidget;
class QPushButton;

#include "project.h"
#include "sessiontabwidget.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onOpenProject();
    void onOpenSession();
    void onSaveSession();

    void onCreateSessionFromTemplate();
    void onOpenSelectedSession();

    void onTabRemovedFromWidget(DraggableTabWidget* tabWidget, int index, QWidget* widget);

    void onTabRemoved(QWidget* widget);
    void onTabMoved(QWidget* widget, DraggableTabWidget* oldParent, DraggableTabWidget* newParent);


private:
    void setupUi();
    void loadProjectDataToUi();
    void refreshSessionList();
    void tryAutoLoadProject();
    void updateBackendConfigForAllSessions();


    Project* m_project = nullptr;

    // UI widgets
    DraggableTabWidget* m_tabWidget = nullptr;
    void onCreateNewWindowWithTab(const QRect &winRect, const DraggableTabWidget::TabInfo &tabInfo);


    // Project tab widgets
    QWidget* m_projectTab = nullptr;
    QLabel* m_projectInfoLabel = nullptr;
    QListWidget* m_templateList = nullptr;
    QListWidget* m_sessionList = nullptr;
    QPushButton* m_createSessionBtn = nullptr;
    QPushButton* m_openSessionBtn = nullptr;

    // Map session file paths to session tab widgets (avoid duplicates)
    QMap<QString, SessionTabWidget*> m_openSessions;

    QMap<QString, SessionTabWidget*> m_pendingSessions;

    void setupTabWidgetConnections(DraggableTabWidget* tabWidget);
    void dumpOpenSessions() const;




};

#endif // MAINWINDOW_H
