#include "tabmanager.h"
#include "draggabletabwidget.h"
#include "sessiontabwidget.h"
#include "project.h"
#include "detachedwindow.h"

#include <QDebug>
#include <QFileInfo>
#include <QPointer>
#include <QHash>

inline uint qHash(const QPointer<DetachedWindow> &key, uint seed = 0) noexcept
{
    // Hash the raw pointer inside QPointer
    return qHash(static_cast<DetachedWindow*>(key.data()), seed);
}

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
    // connect(tabWidget, &DraggableTabWidget::tabCloseRequested, this, &TabManager::onTabCloseRequested);
    connect(tabWidget, &DraggableTabWidget::createNewWindow, this, [this](const QRect& rect, const DraggableTabWidget::TabInfo& tabInfo){
        auto tab = qobject_cast<SessionTabWidget*>(tabInfo.widget);
        if (!tab) {
            qWarning() << "[TabManager] createNewWindow: widget is not a SessionTabWidget";
            return;
        }
        createDetachedWindowWithTab(rect, tabInfo);
    });
}

void TabManager::onTabMoved(QWidget* widget, DraggableTabWidget* oldParent, DraggableTabWidget* newParent)
{
    qDebug() << "[TabManager] onTabMoved widget" << widget << "from" << oldParent << "to" << newParent;
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
            qDebug() << "[TabManager] onTabMoved: Found session path" << sessionPath;
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
        // Update the tab widget pointer in m_openSessions to the new parent tab widget
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
                qDebug() << "[TabManager] Widget still in tab widget:" << tabWidget << "Tab count:" << tabWidget->count();
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
    } else {
        qDebug() << "[TabManager] onTabRemoved: Widget not found in openSessions map";
    }

    ensureProjectTabPresent();
}

// void TabManager::onTabCloseRequested(int index)
// {
//     auto tabWidget = qobject_cast<DraggableTabWidget*>(sender());
//     if (!tabWidget) {
//         qWarning() << "[TabManager] onTabCloseRequested: sender is not DraggableTabWidget";
//         return;
//     }

//     QWidget* widget = tabWidget->widget(index);
//     if (!widget) {
//         qWarning() << "[TabManager] onTabCloseRequested: no widget at index" << index;
//         return;
//     }

//     if (widget == m_projectTab) {
//         qDebug() << "[TabManager] Close requested on Project tab - ignored";
//         return;
//     }

//     qDebug() << "[TabManager] onTabCloseRequested: Removing tab at index" << index << "widget" << widget;

//     tabWidget->removeTab(widget);

//     qDebug() << "[TabManager] onTabCloseRequested: Deleting widget" << widget;

//     widget->deleteLater();
// }

void TabManager::createDetachedWindowWithTab(const QRect& winRect, const DraggableTabWidget::TabInfo& tabInfo)
{
    if (!tabInfo.widget)
        return;

    // Remove tab from current tab widget first
    // code worked fine when this was commented out;
    // could be redundant to other removal; could cause segfault
    auto oldTabWidget = qobject_cast<DraggableTabWidget*>(tabInfo.widget->parentWidget());
    if (oldTabWidget) {
        qDebug() << "[TabManager] Removing tab widget from old tab widget before detaching:" << tabInfo.widget;
        oldTabWidget->removeTab(tabInfo.widget);
    }

    DetachedWindow* newWindow = new DetachedWindow(this);
    m_detachedWindows.insert(newWindow);

    newWindow->addTabFromInfo(tabInfo);
    newWindow->setGeometry(winRect);
    newWindow->show();

    qDebug() << "[TabManager] Created detached window" << newWindow << "with tab:" << tabInfo.widget;

    DraggableTabWidget* dwTabWidget = newWindow->findChild<DraggableTabWidget*>();
    if (dwTabWidget) {
        setupTabWidgetConnections(dwTabWidget);

        DraggableTabWidget* oldTabWidget = qobject_cast<DraggableTabWidget*>(tabInfo.widget->parentWidget());
        if (oldTabWidget && oldTabWidget != dwTabWidget) {
            emit dwTabWidget->tabMoved(tabInfo.widget, oldTabWidget, dwTabWidget);
        }
    }

    connect(newWindow, &QObject::destroyed, this, &TabManager::onDetachedWindowDestroyed);
}

void TabManager::onDetachedWindowDestroyed(QObject* obj)
{
    DetachedWindow* dw = qobject_cast<DetachedWindow*>(obj);
    if (!dw)
        return;

    qDebug() << "[TabManager] Detached window destroyed:" << dw;

    m_detachedWindows.remove(dw);

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
