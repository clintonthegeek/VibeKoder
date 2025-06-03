// draggabletabwidget.h
// Clean, simplified draggable tab widget with detachable tabs and natural Qt ownership.
// Copyright (c) 2019 Akihito Takeuchi
// Modified by Clinton Ignatov, 2025 and refactored 2025

#ifndef DRAGGABLETABWIDGET_H
#define DRAGGABLETABWIDGET_H

#pragma once

#include <QTabWidget>
#include <QIcon>
#include <QTabBar>

class DraggableTabWidget : public QTabWidget {
    Q_OBJECT
public:
    explicit DraggableTabWidget(QWidget* parent = nullptr);
    ~DraggableTabWidget();

    void setIsMainTabWidget(bool isMain) { m_isMainTabWidget = isMain; }
    bool isMainTabWidget() const { return m_isMainTabWidget; }

    void setProjectTabWidget(QWidget* projectTab) { m_projectTab = projectTab; }
    QWidget* projectTabWidget() const { return m_projectTab; }

    bool isProjectTab(int index) const { return widget(index) == m_projectTab; }

    struct TabInfo {
        QWidget* widget = nullptr;
        QString text;
        QIcon icon;
        QString toolTip;
        QString whatsThis;
    };

    QWidget* createNewWindowWidget(const QRect& winRect, const TabInfo& tabInfo);
    void removeTab(int index);

signals:
    void createNewWindow(const QRect& winRect, const TabInfo& tabInfo);
    void tabRemoved(QWidget* widget);

    // New signal to notify tab moved between tab widgets
    void tabMoved(QWidget* widget, DraggableTabWidget* oldParent, DraggableTabWidget* newParent);

private slots:
    void onDetachTabRequested(int index, const QPoint& globalPos);
    void onTabCloseRequested(int index);

private:
    bool m_isMainTabWidget = false;
    QWidget* m_projectTab = nullptr;
};

class DraggableTabBar : public QTabBar {
    Q_OBJECT
public:
    explicit DraggableTabBar(QWidget* parent = nullptr);

signals:
    void detachTabRequested(int index, const QPoint& globalPos);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    void dragLeaveEvent(QDragLeaveEvent* event) override;

private:
    QPixmap createDragPixmap(int index) const;

    int m_dragStartPos = -1;
    int m_draggedTabIndex = -1;
};

#endif // DRAGGABLETABWIDGET_H
