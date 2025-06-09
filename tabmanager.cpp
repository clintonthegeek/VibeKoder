#include "tabmanager.h"
#include "draggabletabwidget.h"
#include "sessiontabwidget.h"
#include "project.h"
#include "detachedwindow.h"

#include <QDebug>
#include <QFileInfo>

TabManager::TabManager(DraggableTabWidget* mainTabWidget, QWidget* projectTab, Project* project, QObject* parent)
    : QObject(parent)
    , m_mainTabWidget(mainTabWidget)
    , m_projectTab(projectTab)
    , m_project(project)
{
    Q_ASSERT(mainTabWidget);
    Q_ASSERT(projectTab);

    // Connect signals on main tab widget
    setupTabWidgetConnections(m_mainTabWidget);

    // Enforce project tab presence
    ensureProjectTabPresent();
}

TabManager::~TabManager()
{
    // Clean up detached windows
    for (DetachedWindow* dw : m_detachedWindows) {
        if (dw)
            dw->deleteLater();
    }
    m_detachedWindows.clear();
}

SessionTabWidget* TabManager::openSession(const QString& sessionFilePath)
{
    QString absPath = QFileInfo(sessionFilePath).absoluteFilePath();

    if (m_openSessions.contains(absPath)) {
        // Already open, activate tab
        SessionTabWidget* tab = m_openSessions.value(absPath);
        if (tab && tab->parentWidget()) {
            auto tabWidget = qobject_cast<DraggableTabWidget*>(tab->parentWidget());
            if (tabWidget) {
                int idx = tabWidget->indexOf(tab);
                if (idx != -1)
                    tabWidget->setCurrentIndex(idx);
            }
        }
        return tab;
    }

    // Create new session tab widget
    SessionTabWidget* tab = new SessionTabWidget(absPath, m_project, m_mainTabWidget);
    m_openSessions.insert(absPath, tab);

    // Add to main tab widget
    m_mainTabWidget->addTab(tab, QFileInfo(absPath).fileName());
    m_mainTabWidget->setCurrentWidget(tab);

    // Connect destroyed to cleanup map
    connect(tab, &QObject::destroyed, this, [this, absPath]() {
        if (m_openSessions.contains(absPath)) {
            m_openSessions.remove(absPath);
            qDebug() << "[TabManager] Session tab destroyed, removed from openSessions:" << absPath;
            emit sessionClosed(absPath);
        }
    });

    qDebug() << "[TabManager] Opened session tab:" << absPath;

    emit sessionOpened(absPath, tab);

    return tab;
}

void TabManager::closeSession(const QString& sessionFilePath)
{
    QString absPath = QFileInfo(sessionFilePath).absoluteFilePath();

    if (!m_openSessions.contains(absPath))
        return;

    SessionTabWidget* tab = m_openSessions.value(absPath);
    if (!tab)
        return;

    auto tabWidget = qobject_cast<DraggableTabWidget*>(tab->parentWidget());
    if (!tabWidget)
        return;

    // Remove tab by widget pointer for safety
    tabWidget->removeTab(tab);
    tab->deleteLater();

    m_openSessions.remove(absPath);

    qDebug() << "[TabManager] Closed session tab:" << absPath;

    emit sessionClosed(absPath);

    ensureProjectTabPresent();
}

void TabManager::ensureProjectTabPresent()
{
    if (!m_projectTab || !m_mainTabWidget)
        return;

    int idx = m_mainTabWidget->indexOf(m_projectTab);
    if (idx == -1) {
        m_mainTabWidget->insertTab(0, m_projectTab, "Project");
        m_mainTabWidget->setProjectTabWidget(m_projectTab);
        m_mainTabWidget->tabBar()->setTabButton(0, QTabBar::RightSide, nullptr);
        qDebug() << "[TabManager] Restored missing Project tab.";
    }
}

void TabManager::setupTabWidgetConnections(DraggableTabWidget* tabWidget)
{
    if (!tabWidget)
        return;

    connect(tabWidget, &DraggableTabWidget::tabMoved, this, &TabManager::onTabMoved);
    connect(tabWidget, &DraggableTabWidget::tabRemoved, this, &TabManager::onTabRemoved);
    connect(tabWidget, &DraggableTabWidget::tabCloseRequested, this, &TabManager::onTabCloseRequested);
    connect(tabWidget, &DraggableTabWidget::createNewWindow, this, [this](const QRect& rect, const DraggableTabWidget::TabInfo& tabInfo){
        auto tab = qobject_cast<SessionTabWidget*>(tabInfo.widget);
        if (!tab) {
            qWarning() << "[TabManager] createNewWindow: widget is not a SessionTabWidget";
            return;
        }
        createDetachedWindowWithTab(rect, tabInfo);
    });
}

void TabManager::onTabMoved(QWidget* widget, DraggableTabWidget* /*oldParent*/, DraggableTabWidget* /*newParent*/)
{
    if (!widget) {
        qWarning() << "[TabManager] onTabMoved called with null widget";
        return;
    }

    QString sessionPath;
    for (auto it = m_openSessions.begin(); it != m_openSessions.end(); ++it) {
        if (it.value() == widget) {
            sessionPath = it.key();
            break;
        }
    }

    if (sessionPath.isEmpty()) {
        auto sessionTab = qobject_cast<SessionTabWidget*>(widget);
        if (sessionTab) {
            sessionPath = sessionTab->sessionFilePath();
            if (!sessionPath.isEmpty()) {
                m_openSessions.insert(sessionPath, sessionTab);
                qDebug() << "[TabManager] onTabMoved: Added new session tab to openSessions:" << sessionPath;
            } else {
                qWarning() << "[TabManager] onTabMoved: SessionTabWidget has empty sessionFilePath";
            }
        } else {
            qWarning() << "[TabManager] onTabMoved: Widget not a SessionTabWidget";
        }
    } else {
        m_openSessions[sessionPath] = qobject_cast<SessionTabWidget*>(widget);
        qDebug() << "[TabManager] Updated openSessions for session:" << sessionPath;
    }

    ensureProjectTabPresent();
}

void TabManager::onTabRemoved(QWidget* widget)
{
    if (!widget) {
        qWarning() << "[TabManager] onTabRemoved called with null widget";
        return;
    }

    // Check if widget is still in any tab widget
    bool stillInTabs = false;
    QWidget* parentWidget = widget->parentWidget();
    while (parentWidget) {
        if (auto tabWidget = qobject_cast<QTabWidget*>(parentWidget)) {
            if (tabWidget->indexOf(widget) != -1) {
                stillInTabs = true;
                break;
            }
        }
        parentWidget = parentWidget->parentWidget();
    }

    if (stillInTabs) {
        qDebug() << "[TabManager] onTabRemoved: Widget still present in a tab widget, ignoring removal:" << widget;
        return;
    }

    QString sessionPathToRemove;
    for (auto it = m_openSessions.begin(); it != m_openSessions.end(); ++it) {
        if (it.value() == widget) {
            sessionPathToRemove = it.key();
            break;
        }
    }

    if (!sessionPathToRemove.isEmpty()) {
        m_openSessions.remove(sessionPathToRemove);
        qDebug() << "[TabManager] Removed session from openSessions due to tabRemoved:" << sessionPathToRemove;
        emit sessionClosed(sessionPathToRemove);
    }

    ensureProjectTabPresent();

    // Close detached window if empty
    auto tabWidget = qobject_cast<DraggableTabWidget*>(widget->parentWidget());
    if (tabWidget && tabWidget != m_mainTabWidget) {
        QTimer::singleShot(0, [tabWidget, this]() {
            if (tabWidget->count() == 0) {
                QWidget* w = tabWidget->window();
                if (w) {
                    qDebug() << "[TabManager] Closing detached window due to empty tabs:" << w;
                    w->close();
                }
            }
        });
    }
}

void TabManager::onTabCloseRequested(int index)
{
    auto tabWidget = qobject_cast<DraggableTabWidget*>(sender());
    if (!tabWidget)
        return;

    QWidget* widget = tabWidget->widget(index);
    if (!widget)
        return;

    // Prevent closing Project tab
    if (widget == m_projectTab) {
        qDebug() << "[TabManager] Close requested on Project tab - ignored";
        return;
    }

    // Use the widget-based removeTab method for safety
    tabWidget->removeTab(widget);

    // Delete the widget after removal
    widget->deleteLater();

    // onTabRemoved will handle map cleanup and window closing
}

void TabManager::createDetachedWindowWithTab(const QRect& winRect, const DraggableTabWidget::TabInfo& tabInfo)
{
    if (!tabInfo.widget)
        return;

    // // Remove tab from current tab widget first
    // auto oldTabWidget = qobject_cast<DraggableTabWidget*>(tabInfo.widget->parentWidget());
    // if (oldTabWidget) {
    //     int idx = oldTabWidget->indexOf(tabInfo.widget);
    //     if (idx != -1) {
    //         oldTabWidget->removeTab(idx);
    //     }
    // }

    // Create new detached window
    DetachedWindow* newWindow = new DetachedWindow(this);
    m_detachedWindows.insert(newWindow);

    // Add tab to detached window's tab widget with proper info
    newWindow->addTabFromInfo(tabInfo);
    newWindow->setGeometry(winRect);
    newWindow->show();

    // Connect signals on detached window's tab widget
    DraggableTabWidget* dwTabWidget = newWindow->findChild<DraggableTabWidget*>();
    if (dwTabWidget) {
        setupTabWidgetConnections(dwTabWidget);
    }

    // Remove from set when window destroyed
    connect(newWindow, &QObject::destroyed, this, &TabManager::onDetachedWindowDestroyed);

    qDebug() << "[TabManager] Created detached window with tab:" << tabInfo.widget;
}

void TabManager::onDetachedWindowDestroyed(QObject* obj)
{
    DetachedWindow* dw = qobject_cast<DetachedWindow*>(obj);
    if (!dw)
        return;

    m_detachedWindows.remove(dw);

    // Clean up openSessions entries whose widgets belong to this window
    QList<QString> keysToRemove;
    for (auto it = m_openSessions.begin(); it != m_openSessions.end(); ++it) {
        QWidget* w = it.value();
        if (!w)
            continue;
        QWidget* p = w->parentWidget();
        bool belongsToWindow = false;
        while (p) {
            if (p == dw) {
                belongsToWindow = true;
                break;
            }
            p = p->parentWidget();
        }
        if (belongsToWindow) {
            keysToRemove.append(it.key());
        }
    }
    for (const QString& key : keysToRemove) {
        m_openSessions.remove(key);
        qDebug() << "[TabManager] Removed session from openSessions due to detached window close:" << key;
        emit sessionClosed(key);
    }

    ensureProjectTabPresent();
}
