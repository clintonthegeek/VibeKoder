#ifndef SESSIONTABWIDGET_H
#define SESSIONTABWIDGET_H

#include <QWidget>
#include <QPlainTextEdit>
#include <QTreeWidget>
#include <QPushButton>
#include <QTextEdit>

#include "project.h"
#include "session.h"
#include "openai_request.h"

class SessionTabWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SessionTabWidget(const QString& sessionPath, Project* project, QWidget *parent=nullptr);
    ~SessionTabWidget();

    bool saveSession();

signals:
    void requestSendPrompt(const QString &compiledPrompt);

private slots:
    void onSendClicked();
    void onForkClicked();
    void onPromptSliceSelected();

    void onOpenAIResponse(const QString &responseText);
    void onOpenAIError(const QString &errorString);

private:
    void loadSession();
    void buildPromptSliceTree();
    QString promptSliceSummary(const PromptSlice &slice) const;

    OpenAIRequest *m_openAIRequest = nullptr;

    QString m_sessionFilePath;
    Project* m_project = nullptr;
    Session m_session;

    // UI widgets
    QPlainTextEdit* m_sessionEditor = nullptr;
    QTreeWidget* m_promptSliceTree = nullptr;
    QPushButton* m_forkButton = nullptr;
    QTextEdit* m_commandOutput = nullptr;
    QPlainTextEdit* m_appendUserPrompt = nullptr;
    QPushButton* m_sendButton = nullptr;
};

#endif // SESSIONTABWIDGET_H
