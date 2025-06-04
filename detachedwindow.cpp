#include "detachedwindow.h"
#include <QDebug>

int DetachedWindow::s_nextInstanceId = 1;

DetachedWindow::DetachedWindow(QWidget* parent)
    : QMainWindow(parent)
{
    m_instanceId = s_nextInstanceId++;
    qDebug() << QString("[DetachedWindow #%1] Constructor called").arg(m_instanceId);

    m_tabWidget = new DraggableTabWidget(this);
    m_tabWidget->setIsMainTabWidget(false);
    setCentralWidget(m_tabWidget);

    connect(m_tabWidget, &DraggableTabWidget::createNewWindow,
            this, &DetachedWindow::onCreateNewWindow);

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

DetachedWindow::~DetachedWindow()
{
    qDebug() << QString("[DetachedWindow #%1] Destructor called").arg(m_instanceId);
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

void DetachedWindow::onCreateNewWindow(const QRect& winRect, const DraggableTabWidget::TabInfo& tabInfo)
{
    qDebug() << QString("[DetachedWindow #%1] Creating new detached window for tab '%2' (widget %3)")
    .arg(m_instanceId).arg(tabInfo.text).arg((quintptr)tabInfo.widget);

    DetachedWindow* newWindow = new DetachedWindow();
    newWindow->addTabFromInfo(tabInfo);
    newWindow->setGeometry(winRect);
    newWindow->show();

    qDebug() << QString("[DetachedWindow #%1] Created new detached window %2")
                    .arg(m_instanceId).arg((quintptr)newWindow);
}

void DetachedWindow::onTabRemoved(QWidget* widget)
{
    qDebug() << QString("[DetachedWindow #%1] Tab removed: widget %2")
    .arg(m_instanceId).arg((quintptr)widget);

    if (!m_tabWidget)
        return;

    // Use QMetaObject::invokeMethod with Qt::QueuedConnection to defer the check
    QMetaObject::invokeMethod(this, "checkEmptyAndClose", Qt::QueuedConnection);
}

// Add this new slot to DetachedWindow:
void DetachedWindow::checkEmptyAndClose()
{
    logState("After tabRemoved (queued)");

    if (m_tabWidget->count() == 0) {
        qDebug() << QString("[DetachedWindow #%1] No tabs left, closing window (queued)").arg(m_instanceId);
        close();
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
