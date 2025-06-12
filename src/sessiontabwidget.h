#ifndef SESSIONTABWIDGET_H
#define SESSIONTABWIDGET_H

#include <QWidget>
#include <QPlainTextEdit>
#include <QTreeWidget>
#include <QPushButton>
#include <QToolButton>
#include <QTextEdit>
#include <QMenu>
#include <QStatusBar>
#include <QLineEdit>
#include <QLabel>

#include "project.h"
#include "session.h"
#include "aibackend.h"
#include "openaibackend.h"
#include "qmarkdowntextedit/qmarkdowntextedit.h"

class SessionTabWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SessionTabWidget(const QString& sessionPath, Project* project,
                              QWidget *parent = nullptr, bool isTempSession = false,
                              QStatusBar* statusBar = nullptr);
    ~SessionTabWidget();

    bool saveSession();
    QString sessionFilePath() const;

    void updateBackendConfig(const QVariantMap &config);

    bool confirmDiscardUnsavedChanges();

    Session& session() { return m_session; }
    AIBackend* aiBackend() const { return m_aiBackend; }
signals:
    void tempSessionSaved(const QString& newFilePath);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;

private slots:
    void onSaveClicked();
    void onSendClicked();
    void onForkClicked();
    void onDeleteAfterClicked();
    void onOpenMarkdownFileClicked();
    void onOpenCacheClicked();
    void onRefreshClicked();
    void onPromptSliceSelected();

    void onPartialResponse(const QString &requestId, const QString &partialText);
    void onFinished(const QString &requestId, const QString &fullResponse);
    void onErrorOccurred(const QString &requestId, const QString &errorString);
    void onStatusChanged(const QString &requestId, const QString &status);

private:
    void loadSession();
    void buildPromptSliceTree();
    QString promptSliceSummary(const PromptSlice &slice) const;
    void updateUiForSelectedSlice(int selectedIndex);
    void updateButtonStates();
    void markUnsavedChanges(bool changed);
    void onEditTitleDescClicked();


    AIBackend *m_aiBackend = nullptr;
    QString m_currentRequestId;

    QString m_sessionFilePath;
    Project* m_project = nullptr;
    Session m_session;

    // UI widgets
    QStatusBar* m_statusBar = nullptr;
    QTreeWidget* m_promptSliceTree = nullptr;
    QPushButton* m_forkButton = nullptr;
    QPushButton* m_openMarkdownButton = nullptr;
    QPushButton* m_openCacheButton = nullptr;
    QMarkdownTextEdit* m_sliceViewer = nullptr;
    QPlainTextEdit* m_appendUserPrompt = nullptr;
    QPushButton* m_sendButton = nullptr;
    QPushButton* m_saveButton = nullptr;
    QPushButton* m_refreshButton = nullptr;
    QPushButton* m_editTitleDescBtn = nullptr;
    QToolButton* m_editToolButton = nullptr;
    QWidget* m_editSpacer = nullptr;

    QMenu* m_editMenu = nullptr;
    QMenu* m_contextMenu = nullptr;

    // Buffer for partial response text (optional)
    QString m_partialResponseBuffer;
    bool m_updatingEditor = false;

    void onSaveSliceAsMarkdown();

    // New Save button for temp sessions
    QPushButton* m_saveTempButton = nullptr;
    bool m_isTempSession = false;

    // Track if there are unsaved changes in m_appendUserPrompt
    bool m_unsavedChanges = false;

    //Unsaved entry into text box cached
    QString m_pendingUserPromptText;
    // Cached text of last saved user prompt slice for comparison
    QString m_lastSavedUserPromptText;

    QLineEdit* m_titleEdit = nullptr;
    QTextEdit* m_descriptionEdit = nullptr;
    QPushButton* m_generateTitleDescBtn = nullptr;
    QLabel* m_sessionTitleLabel = nullptr;

    void setupHeaderEditorUi();
    void loadHeaderMetadataToUi();
    void saveHeaderMetadataFromUi();
};

#endif // SESSIONTABWIDGET_H
