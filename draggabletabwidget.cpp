#include "draggabletabwidget.h"

#include <QTabBar>
#include <QMouseEvent>
#include <QDrag>
#include <QMimeData>
#include <QApplication>
#include <QStyleOptionTab>
#include <QPainter>
#include <QMainWindow>
#include <QDebug>

    // === DraggableTabBar Implementation ===

    DraggableTabBar::DraggableTabBar(QWidget* parent)
    : QTabBar(parent)
{
    setAcceptDrops(true);
    setMovable(true);
}

void DraggableTabBar::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        int tabIndex = tabAt(event->pos());
        if (tabIndex < 0) {
            QTabBar::mousePressEvent(event);
            return;
        }

        // Prevent dragging Project tab
        auto tabWidget = qobject_cast<DraggableTabWidget*>(parentWidget());
        if (tabWidget && tabWidget->isProjectTab(tabIndex)) {
            m_dragStartPos = -1;
            m_draggedTabIndex = -1;
            // Allow selection but no drag
            QTabBar::mousePressEvent(event);
            return;
        }

        m_dragStartPos = event->pos().x();
        m_draggedTabIndex = tabIndex;
    }
    QTabBar::mousePressEvent(event);
}

void DraggableTabBar::mouseMoveEvent(QMouseEvent* event)
{
    if (!(event->buttons() & Qt::LeftButton) || m_draggedTabIndex < 0) {
        QTabBar::mouseMoveEvent(event);
        return;
    }

    int distance = (event->pos() - QPoint(m_dragStartPos, event->pos().y())).manhattanLength();
    if (distance < QApplication::startDragDistance()) {
        QTabBar::mouseMoveEvent(event);
        return;
    }

    QDrag* drag = new QDrag(this);
    QMimeData* mimeData = new QMimeData;

    QByteArray tabIndexData = QByteArray::number(m_draggedTabIndex);
    mimeData->setData("application/x-tab-index", tabIndexData);

    drag->setMimeData(mimeData);

    QPixmap pixmap = createDragPixmap(m_draggedTabIndex);
    drag->setPixmap(pixmap);
    drag->setHotSpot(QPoint(pixmap.width() / 2, pixmap.height() / 2));

    Qt::DropAction dropAction = drag->exec(Qt::MoveAction);

    auto tabWidget = qobject_cast<DraggableTabWidget*>(parentWidget());
    if (!tabWidget) {
        m_draggedTabIndex = -1;
        m_dragStartPos = -1;
        return;
    }

    if (dropAction == Qt::IgnoreAction) {
        // Dragged outside any tab bar

        // Disallow detaching last tab from non-main tab widget
        if (!tabWidget->isMainTabWidget() && tabWidget->count() == 1) {
            QApplication::beep();
            m_draggedTabIndex = -1;
            m_dragStartPos = -1;
            return;
        }

        // Prevent detaching Project tab
        if (tabWidget->isProjectTab(m_draggedTabIndex)) {
            QApplication::beep();
            m_draggedTabIndex = -1;
            m_dragStartPos = -1;
            return;
        }

        // Emit detachTabRequested to create new window
        QPoint globalPos = mapToGlobal(event->pos());
        emit detachTabRequested(m_draggedTabIndex, globalPos);
    }

    m_draggedTabIndex = -1;
    m_dragStartPos = -1;
}

void DraggableTabBar::dragEnterEvent(QDragEnterEvent* event)
{
    if (event->mimeData()->hasFormat("application/x-tab-index")) {
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

void DraggableTabBar::dragMoveEvent(QDragMoveEvent* event)
{
    if (event->mimeData()->hasFormat("application/x-tab-index")) {
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

void DraggableTabBar::dropEvent(QDropEvent* event)
{
    if (!event->mimeData()->hasFormat("application/x-tab-index")) {
        event->ignore();
        return;
    }

    QByteArray data = event->mimeData()->data("application/x-tab-index");
    bool ok = false;
    int sourceIndex = data.toInt(&ok);
    if (!ok) {
        event->ignore();
        return;
    }

    DraggableTabBar* sourceTabBar = qobject_cast<DraggableTabBar*>(event->source());
    if (!sourceTabBar) {
        event->ignore();
        return;
    }

    QTabWidget* sourceTabWidget = qobject_cast<QTabWidget*>(sourceTabBar->parentWidget());
    QTabWidget* targetTabWidget = qobject_cast<QTabWidget*>(parentWidget());
    if (!sourceTabWidget || !targetTabWidget) {
        event->ignore();
        return;
    }

    QWidget* tabPage = sourceTabWidget->widget(sourceIndex);
    if (!tabPage) {
        event->ignore();
        return;
    }

    // Prevent moving Project tab out of main tab widget
    DraggableTabWidget* sourceDraggable = qobject_cast<DraggableTabWidget*>(sourceTabWidget);
    if (sourceDraggable && sourceDraggable->isProjectTab(sourceIndex)) {
        QApplication::beep();
        event->ignore();
        return;
    }

    int targetIndex = tabAt(event->pos());
    if (targetIndex == -1)
        targetIndex = targetTabWidget->count();

    // Capture tab metadata BEFORE removing the tab
    QString tabText = sourceTabBar->tabText(sourceIndex);
    QIcon tabIcon = sourceTabBar->tabIcon(sourceIndex);
    QString tabToolTip = sourceTabBar->tabToolTip(sourceIndex);
    QString tabWhatsThis = sourceTabBar->tabWhatsThis(sourceIndex);

    // Remove tab by QWidget pointer using DraggableTabWidget::removeTab(QWidget*)
    if (sourceDraggable) {
        sourceDraggable->removeTab(tabPage);
    } else {
        // Fallback to base removeTab(int) if not DraggableTabWidget
        int removeIndex = sourceTabWidget->indexOf(tabPage);
        if (removeIndex != -1) {
            sourceTabWidget->removeTab(removeIndex);
        }
    }

    // Insert tab into target tab widget with preserved metadata
    if (targetIndex > targetTabWidget->count())
        targetIndex = targetTabWidget->count();

    tabPage->setParent(targetTabWidget);
    int insertedIndex = targetTabWidget->insertTab(targetIndex, tabPage, tabIcon, tabText);
    qDebug() << "[dropEvent] Inserted tab into target at index" << insertedIndex << "Target tab count:" << targetTabWidget->count();

    targetTabWidget->setTabToolTip(targetIndex, tabToolTip);
    targetTabWidget->setTabWhatsThis(targetIndex, tabWhatsThis);
    targetTabWidget->setCurrentIndex(targetIndex);

    // Emit tabMoved signal with widget pointer
    emit qobject_cast<DraggableTabWidget*>(targetTabWidget)->tabMoved(
        tabPage,
        sourceDraggable,
        qobject_cast<DraggableTabWidget*>(targetTabWidget));

    event->acceptProposedAction();
}

void DraggableTabBar::dragLeaveEvent(QDragLeaveEvent* event)
{
    event->accept();
}

QPixmap DraggableTabBar::createDragPixmap(int index) const
{
    if (index < 0 || index >= count())
        return QPixmap();

    QRect rect = tabRect(index);
    QPixmap pixmap(rect.size());
    pixmap.fill(Qt::transparent);

    QStyleOptionTab opt;
    initStyleOption(&opt, index);

    QPainter painter(&pixmap);
    style()->drawControl(QStyle::CE_TabBarTabShape, &opt, &painter, this);
    style()->drawControl(QStyle::CE_TabBarTabLabel, &opt, &painter, this);

    return pixmap;
}

// === DraggableTabWidget Implementation ===

DraggableTabWidget::DraggableTabWidget(QWidget* parent)
    : QTabWidget(parent)
    , m_isMainTabWidget(false)
    , m_projectTab(nullptr)
{
    auto tabBar = new DraggableTabBar(this);
    setTabBar(tabBar);
    setMovable(true);
    setTabsClosable(true);

    connect(tabBar, &DraggableTabBar::detachTabRequested, this, &DraggableTabWidget::onDetachTabRequested);
    connect(this, &QTabWidget::tabCloseRequested, this, &DraggableTabWidget::onTabCloseRequested);
}

DraggableTabWidget::~DraggableTabWidget()
{
    qDebug() << "[DraggableTabWidget] Destructor called for" << this;
}

void DraggableTabWidget::onDetachTabRequested(int index, const QPoint& globalPos)
{
    if (index < 0 || index >= count())
        return;

    QWidget* page = widget(index);
    if (!page)
        return;

    if (page == m_projectTab) {
        qDebug() << "[DraggableTabWidget] Detach requested on Project tab - ignored";
        return; // Disallow detaching Project tab
    }

    qDebug() << "[DraggableTabWidget] Detach requested for tab widget:" << page;

    TabInfo tabInfo;
    tabInfo.widget = page;
    tabInfo.text = tabText(index);
    tabInfo.icon = tabIcon(index);
    tabInfo.toolTip = tabToolTip(index);
    tabInfo.whatsThis = tabWhatsThis(index);

    // Remove tab immediately
    removeTab(page);

    emit createNewWindow(QRect(globalPos, QSize(600, 400)), tabInfo);

    if (count() == 0 && !m_isMainTabWidget) {
        QWidget* w = window();
        if (w) {
            qDebug() << "[DraggableTabWidget] No tabs left after detach, closing window:" << w;
            w->close();
        }
    }
}

void DraggableTabWidget::onTabCloseRequested(int index)
{
    if (index < 0 || index >= count())
        return;

    QWidget* page = widget(index);
    if (!page)
        return;

    if (page == m_projectTab) {
        qDebug() << "[DraggableTabWidget] Close requested on Project tab - ignored";
        return; // Disallow closing Project tab
    }

    qDebug() << "[DraggableTabWidget] Close requested for tab widget:" << page;

    removeTab(page);
    page->deleteLater();

    if (count() == 0 && !m_isMainTabWidget) {
        QWidget* w = window();
        if (w) {
            qDebug() << "[DraggableTabWidget] No tabs left after close, closing window:" << w;
            w->close();
        }
    }
}

void DraggableTabWidget::removeTab(QWidget* widget)
{
    if (!widget) {
        qDebug() << "[DraggableTabWidget] removeTab called with null widget";
        return;
    }

    if (widget == m_projectTab) {
        qDebug() << "[DraggableTabWidget] Attempt to remove Project tab ignored";
        return; // Prevent removing Project tab
    }

    int idx = indexOf(widget);
    if (idx == -1) {
        qDebug() << "[DraggableTabWidget] removeTab called but widget not found in tabs";
        return;
    }

    qDebug() << "[DraggableTabWidget] removeTab called for widget:" << widget << "at index:" << idx;

    QTabWidget::removeTab(idx);

    qDebug() << "[DraggableTabWidget] Tab removed from QTabWidget, emitting tabRemoved signal";

    emit tabRemoved(widget);
}

QWidget* DraggableTabWidget::createNewWindowWidget(const QRect& winRect, const TabInfo& tabInfo)
{
    QMainWindow* newWindow = new QMainWindow();
    newWindow->setAttribute(Qt::WA_DeleteOnClose);

    DraggableTabWidget* newTabWidget = new DraggableTabWidget(newWindow);
    newTabWidget->setIsMainTabWidget(false);
    newWindow->setCentralWidget(newTabWidget);

    tabInfo.widget->setParent(newTabWidget);
    newTabWidget->addTab(tabInfo.widget, tabInfo.icon, tabInfo.text);
    newTabWidget->setTabToolTip(0, tabInfo.toolTip);
    newTabWidget->setTabWhatsThis(0, tabInfo.whatsThis);
    newTabWidget->setCurrentIndex(0);

    newWindow->setGeometry(winRect);
    newWindow->show();

    return newWindow;
}
