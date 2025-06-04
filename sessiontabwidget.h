#ifndef SESSIONTABWIDGET_H
#define SESSIONTABWIDGET_H

#include <QWidget>
#include <QPlainTextEdit>
#include <QTreeWidget>
#include <QPushButton>
#include <QTextEdit>
#include <QMenu>

#include "project.h"
#include "session.h"
#include "aibackend.h"
#include "openaibackend.h"

class SessionTabWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SessionTabWidget(const QString& sessionPath, Project* project, QWidget *parent=nullptr);
    ~SessionTabWidget();

    bool saveSession();
    QString sessionFilePath() const;

    void updateBackendConfig(const QVariantMap &config);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;

private slots:
    void onSendClicked();
    void onForkClicked();
    void onDeleteToEndClicked();
    void onOpenMarkdownFileClicked();
    void onOpenCacheClicked();
    void onPromptSliceSelected();

    void onPartialResponse(const QString &requestId, const QString &partialText);
    void onFinished(const QString &requestId, const QString &fullResponse);
    void onErrorOccurred(const QString &requestId, const QString &errorString);
    void onStatusChanged(const QString &requestId, const QString &status);

private:
    void loadSession();
    void buildPromptSliceTree();
    QString promptSliceSummary(const PromptSlice &slice) const;

    AIBackend *m_aiBackend = nullptr;
    QString m_currentRequestId;

    QString m_sessionFilePath;
    Project* m_project = nullptr;
    Session m_session;

    // UI widgets
    QTreeWidget* m_promptSliceTree = nullptr;
    QPushButton* m_forkButton = nullptr;
    QPushButton* m_openMarkdownButton = nullptr;
    QPushButton* m_openCacheButton = nullptr;
    QTextEdit* m_sliceEditor = nullptr;
    QPlainTextEdit* m_appendUserPrompt = nullptr;
    QPushButton* m_sendButton = nullptr;

    QMenu* m_contextMenu = nullptr;

    // Buffer for partial response text (optional)
    QString m_partialResponseBuffer;
};

#endif // SESSIONTABWIDGET_H
