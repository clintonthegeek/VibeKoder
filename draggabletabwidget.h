// draggabletabwidget.h
// Clean, simplified draggable tab widget with detachable tabs and natural Qt ownership.
// Copyright (c) 2019 Akihito Takeuchi
// Modified by Clinton Ignatov, 2025 and refactored 2025

#pragma once

#include <QTabWidget>
#include <QIcon>
#include <QTabBar>

class DraggableTabWidget : public QTabWidget {
    Q_OBJECT
public:
    explicit DraggableTabWidget(QWidget* parent = nullptr);
    ~DraggableTabWidget();

    // Mark this tab widget as the main tab widget (usually in MainWindow)
    void setIsMainTabWidget(bool isMain) { m_isMainTabWidget = isMain; }
    bool isMainTabWidget() const { return m_isMainTabWidget; }

    // Set the special Project tab widget to disallow dragging/closing it
    void setProjectTabWidget(QWidget* projectTab) { m_projectTab = projectTab; }
    QWidget* projectTabWidget() const { return m_projectTab; }

    // Struct to hold tab info when detaching or creating new windows
    struct TabInfo {
        QWidget* widget = nullptr;
        QString text;
        QIcon icon;
        QString toolTip;
        QString whatsThis;
    };

    // Helper to create a new window with a single tab from TabInfo
    QWidget* createNewWindowWidget(const QRect& winRect, const TabInfo& tabInfo);
    // Override removeTab to emit tabRemoved signal
    void removeTab(int index);

signals:
    // Emitted when a tab is dragged outside any tab bar and needs a new window
    void createNewWindow(const QRect& winRect, const TabInfo& tabInfo);
    // New signal: emitted whenever a tab widget is removed (closed or dragged away)
    void tabRemoved(QWidget* widget);
private slots:
    // Slot to handle detach requests from the tab bar
    void onDetachTabRequested(int index, const QPoint& globalPos);

    // Slot to handle tab close requests
    void onTabCloseRequested(int index);

private:
    bool m_isMainTabWidget = false;
    QWidget* m_projectTab = nullptr;
};

// Forward declaration of DraggableTabBar (internal use)
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
