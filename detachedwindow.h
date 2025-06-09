#ifndef DETACHEDWINDOW_H
#define DETACHEDWINDOW_H

#pragma once

#include <QMainWindow>
#include "draggabletabwidget.h"
#include "tabmanager.h"


class DetachedWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit DetachedWindow(TabManager* tabManager = nullptr, QWidget* parent = nullptr);
    ~DetachedWindow();

    // Adds a tab to this window's tab widget from the given TabInfo
    void addTabFromInfo(const DraggableTabWidget::TabInfo& tabInfo);

signals:
    // Emitted when a tab widget removes a tab (optional for external tracking)
    void tabRemoved(QWidget* widget);

private slots:
    // Slot to handle requests to create a new detached window from this window
    void onCreateNewWindow(const QRect& winRect, const DraggableTabWidget::TabInfo& tabInfo);

    // Slot to handle tab removals; closes window if no tabs left
    void onTabRemoved(QWidget* widget);

    // New slot for deferred check
    void checkEmptyAndClose();

private:
    DraggableTabWidget* m_tabWidget = nullptr;

    TabManager* m_tabManager = nullptr;

    static int s_nextInstanceId;
    int m_instanceId = -1;

    void logState(const QString& prefix = QString()) const;
};
#endif // DETACHEDWINDOW_H
