#include "detachedwindow.h"
#include <QDebug>

int DetachedWindow::s_nextInstanceId = 1;

DetachedWindow::DetachedWindow(TabManager* tabManager, QWidget* parent)
    : QMainWindow(parent)
    , m_tabManager(tabManager)
{
    m_instanceId = s_nextInstanceId++;
    qDebug() << QString("[DetachedWindow #%1] Constructor called").arg(m_instanceId);

    m_tabWidget = new DraggableTabWidget(this);
    m_tabWidget->setIsMainTabWidget(false);
    setCentralWidget(m_tabWidget);

    connect(m_tabWidget, &DraggableTabWidget::tabRemoved,
            this, &DetachedWindow::onTabRemoved);

    connect(m_tabWidget, &QTabWidget::tabCloseRequested, this, [this](int index){
        QMetaObject::invokeMethod(this, [this]() {
            if (m_tabWidget->count() == 0) {
                qDebug() << QString("[DetachedWindow #%1] No tabs left after close, closing window").arg(m_instanceId);
                close();
            }
        }, Qt::QueuedConnection);
    });

    setAttribute(Qt::WA_DeleteOnClose);
}


DetachedWindow::~DetachedWindow() {
    qDebug() << QString("[DetachedWindow %1] Destructor called").arg((quintptr)this, QT_POINTER_SIZE * 2, 16, QChar('0'));
}

void DetachedWindow::addTabFromInfo(const DraggableTabWidget::TabInfo& tabInfo)
{
    if (!m_tabWidget)
        return;

    qDebug() << QString("[DetachedWindow #%1] Adding tab '%2' (widget %3)")
                    .arg(m_instanceId).arg(tabInfo.text).arg((quintptr)tabInfo.widget);

    tabInfo.widget->setParent(m_tabWidget);

    int index = m_tabWidget->addTab(tabInfo.widget, tabInfo.icon, tabInfo.text);
    m_tabWidget->setTabToolTip(index, tabInfo.toolTip);
    m_tabWidget->setTabWhatsThis(index, tabInfo.whatsThis);
    m_tabWidget->setCurrentIndex(index);

    logState("After addTabFromInfo");
}

void DetachedWindow::onTabRemoved(QWidget* widget)
{
    qDebug() << QString("[DetachedWindow #%1] Tab removed: widget %2")
    .arg(m_instanceId).arg((quintptr)widget);

    if (m_tabWidget) {
        int count = m_tabWidget->count();
        QStringList tabTexts;
        for (int i = 0; i < count; ++i)
            tabTexts << m_tabWidget->tabText(i);
        qDebug() << QString("[DetachedWindow #%1] Tab count after removal: %2, Tabs: %3")
                        .arg(m_instanceId).arg(count).arg(tabTexts.join(", "));
    } else {
        qDebug() << QString("[DetachedWindow #%1] m_tabWidget is null").arg(m_instanceId);
    }

    // Additionally, print all QTabWidget children of this DetachedWindow,
    // their tab counts and tab texts, to catch any unexpected tab widgets.
    QList<QTabWidget*> tabWidgets = findChildren<QTabWidget*>();
    qDebug() << QString("[DetachedWindow #%1] All QTabWidget children count: %2")
                    .arg(m_instanceId).arg(tabWidgets.size());
    for (QTabWidget* tabw : tabWidgets) {
        int c = tabw->count();
        QStringList texts;
        for (int i = 0; i < c; ++i)
            texts << tabw->tabText(i);
        qDebug() << QString("  TabWidget %1 (%2): count=%3, tabs=[%4]")
                        .arg((quintptr)tabw).arg(tabw->objectName()).arg(c).arg(texts.join(", "));
    }

    // Log parent chain of removed widget for ownership debugging
    QWidget* p = widget;
    QStringList parents;
    while (p) {
        parents << QString("%1(%2)").arg(p->metaObject()->className()).arg((quintptr)p);
        p = p->parentWidget();
    }
    qDebug() << QString("[DetachedWindow #%1] Removed widget parent chain: %2")
                    .arg(m_instanceId).arg(parents.join(" -> "));

    if (m_tabManager) {
        m_tabManager->onTabRemoved(widget);
    }

    qDebug() << QString("[DetachedWindow #%1] Scheduling checkEmptyAndClose").arg(m_instanceId);

    QMetaObject::invokeMethod(this, "checkEmptyAndClose", Qt::QueuedConnection);
}

// Add this new slot to DetachedWindow:
void DetachedWindow::checkEmptyAndClose()
{
    if (!m_tabWidget) {
        qDebug() << "[DetachedWindow #" << m_instanceId << "] checkEmptyAndClose called but m_tabWidget is null";
        return;
    }

    int count = m_tabWidget->count();

    QStringList tabTexts;
    for (int i = 0; i < count; ++i)
        tabTexts << m_tabWidget->tabText(i);

    qDebug() << QString("[DetachedWindow #%1] checkEmptyAndClose called - Tab count: %2, Tabs: %3")
                    .arg(m_instanceId).arg(count).arg(tabTexts.join(", "));

    if (count == 0) {
        qDebug() << QString("[DetachedWindow #%1] No tabs left, calling close()").arg(m_instanceId);
        close();
    } else {
        qDebug() << QString("[DetachedWindow #%1] Tabs remain, not closing").arg(m_instanceId);
    }
}

void DetachedWindow::logState(const QString& prefix) const
{
    if (!m_tabWidget)
        return;

    QStringList tabTexts;
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        tabTexts << m_tabWidget->tabText(i);
    }

    qDebug() << QString("[DetachedWindow #%1] %2 - Tab count: %3, Tabs: %4")
                    .arg(m_instanceId).arg(prefix).arg(m_tabWidget->count()).arg(tabTexts.join(", "));
}
