#ifndef TABMANAGER_H
#define TABMANAGER_H

#include <QObject>
#include <QMap>
#include <QSet>
#include <QPointer>
#include <QRect>
#include "draggabletabwidget.h"

class SessionTabWidget;
class Project;
class DetachedWindow;

class TabManager : public QObject
{
    Q_OBJECT
public:
    explicit TabManager(DraggableTabWidget* mainTabWidget, QWidget* projectTab, Project* project, QObject* parent = nullptr);
    ~TabManager() override;

    // Open a session tab by file path; returns pointer to tab widget
    SessionTabWidget* openSession(const QString& sessionFilePath);

    // Close a session tab by file path
    void closeSession(const QString& sessionFilePath);

    // Get all open sessions
    QMap<QString, SessionTabWidget*> openSessions() const { return m_openSessions; }

    // Ensure Project tab is present in main tab widget
    void ensureProjectTabPresent();

signals:
    void sessionOpened(const QString& sessionFilePath, SessionTabWidget* tab);
    void sessionClosed(const QString& sessionFilePath);

public slots:
    void onTabRemoved(QWidget* widget);

private slots:
    void onTabMoved(QWidget* widget, DraggableTabWidget* oldParent, DraggableTabWidget* newParent);
    // void onTabCloseRequested(int index);
    void onDetachedWindowDestroyed(QObject* obj);

private:
    void setupTabWidgetConnections(DraggableTabWidget* tabWidget);
    void createDetachedWindowWithTab(const QRect& winRect, const DraggableTabWidget::TabInfo& tabInfo);

    DraggableTabWidget* m_mainTabWidget = nullptr;
    QWidget* m_projectTab = nullptr;
    Project* m_project = nullptr;

    // Map session file path -> session tab widget
    QMap<QString, SessionTabWidget*> m_openSessions;

    // Track detached windows to manage lifetime
    QSet<QPointer<DetachedWindow>> m_detachedWindows;
};

#endif // TABMANAGER_H
