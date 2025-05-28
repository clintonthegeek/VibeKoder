#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "project.h"
#include "session.h"
#include "openai_request.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>
#include <QDebug>
#include <QTextCursor>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QRegularExpressionMatchIterator>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    m_openAI = new OpenAIRequest(this);
    connect(m_openAI, &OpenAIRequest::requestFinished, this, &MainWindow::onOpenAIResponse);
    connect(m_openAI, &OpenAIRequest::requestError, this, &MainWindow::onOpenAIError);

    // You would add corresponding buttons and connect them to the slots:
    // For brevity, assume ui->actionOpenProject, ui->actionOpenSession etc. exist.
    connect(ui->actionOpenProject, &QAction::triggered, this, &MainWindow::onOpenProject);
    connect(ui->actionOpenSession, &QAction::triggered, this, &MainWindow::onOpenSession);
    connect(ui->actionSaveSession, &QAction::triggered, this, &MainWindow::onSaveSession);
    connect(ui->sendButton, &QPushButton::clicked, this, &MainWindow::onSendPrompt);
}

MainWindow::~MainWindow()
{
    delete ui;
    clearSession();
    delete m_project;
}

void MainWindow::clearSession()
{
    if (m_session) {
        delete m_session;
        m_session = nullptr;
    }
}

void MainWindow::onOpenProject()
{
    QString filename = QFileDialog::getOpenFileName(this, "Open Project TOML", QString(), "TOML Files (*.toml)");
    if (filename.isEmpty())
        return;

    if (m_project)
        delete m_project;
    m_project = new Project();

    if (!m_project->load(filename)) {
        QMessageBox::critical(this, "Error", "Failed to load project file.");
        delete m_project;
        m_project = nullptr;
        return;
    }
    statusBar()->showMessage("Loaded project: " + filename);
}

void MainWindow::onOpenSession()
{
    if (!m_project) {
        QMessageBox::warning(this, "No Project", "Please load a project first.");
        return;
    }

    QString filename = QFileDialog::getOpenFileName(this, "Open Session Markdown", QString(), "Markdown Files (*.md *.markdown)");
    if (filename.isEmpty())
        return;

    clearSession();
    m_session = new Session(m_project);

    if (!m_session->load(filename)) {
        QMessageBox::critical(this, "Error", "Failed to load session file.");
        clearSession();
        return;
    }

    ui->textEditSession->setPlainText(""); // Clear editor
    // Show session slices concatenated for editing in this naive Phase 1
    QString combinedText;
    for (const auto &slice : m_session->slices()) {
        QString role;
        switch (slice.role) {
        case MessageRole::User: role = "User"; break;
        case MessageRole::Assistant: role = "Assistant"; break;
        case MessageRole::System: role = "System"; break;
        }
        combinedText += QString("### %1 (%2)\n```markdown\n%3\n```\n\n")
                            .arg(role).arg(slice.timestamp).arg(slice.content.trimmed());
    }
    ui->textEditSession->setPlainText(combinedText);
    statusBar()->showMessage("Session loaded: " + filename);
}

void MainWindow::onSaveSession()
{
    if (!m_session) {
        QMessageBox::warning(this, "No Session", "Load or create a session first.");
        return;
    }

    // For Phase 1, just parse editor content (naively) and save session file
    const QString editedText = ui->textEditSession->toPlainText();

    if (!m_session->parseSessionMarkdown(editedText)) {
        QMessageBox::warning(this, "Parse Error", "Edited session content is malformed.");
        return;
    }

    if (!m_session->save(QString())) {
        QMessageBox::warning(this, "Save Error", "Failed to save session file.");
        return;
    }
    statusBar()->showMessage("Session saved.");
}

void MainWindow::onSendPrompt()
{
    if (!m_project) {
        QMessageBox::warning(this, "No Project", "Please load a project first.");
        return;
    }
    if (!m_session) {
        QMessageBox::warning(this, "No Session", "Please load a session first.");
        return;
    }

    // Step 1: Reload session markdown from the session edit box (full conversation)
    QString sessionMarkdown = ui->textEditSession->toPlainText();

    if (!m_session->parseSessionMarkdown(sessionMarkdown)) {
        QMessageBox::warning(this, "Parse Error", "Failed to parse the session markdown content.");
        return;
    }

    // Step 2: Check user append box for new input
    QString appendText = ui->textAppendUserPromptSlice->toPlainText().trimmed();

    if (!appendText.isEmpty()) {
        // Append new user slice
        m_session->appendUserSlice(appendText);

        // Clear append box for next input
        ui->textAppendUserPromptSlice->clear();

        // Update the main session editor with new full markdown including appended slice
        QString fullMarkdown = m_session->compiledRawMarkdown();

        ui->textEditSession->blockSignals(true);
        ui->textEditSession->setPlainText(fullMarkdown);
        ui->textEditSession->blockSignals(false);
    } else {
        // No text in append box, but user may have edited session editor already
        // Do nothing extra — send current parsed slices "as is"
    }

    // Step 3: Compile prompt and send API request
    QString compiledPrompt = m_session->compilePrompt();

    qDebug().noquote() << "=== Full prompt sent to OpenAI API ===\n" << compiledPrompt;

    m_openAI->setAccessToken(m_project->accessToken());
    m_openAI->setModel(m_project->model());
    m_openAI->setPrompt(compiledPrompt);
    m_openAI->setMaxTokens(m_project->maxTokens());
    m_openAI->setTemperature(m_project->temperature());
    m_openAI->setTopP(m_project->topP());
    m_openAI->setFrequencyPenalty(m_project->frequencyPenalty());
    m_openAI->setPresencePenalty(m_project->presencePenalty());

    // Disable UI to prevent repeated sends
    ui->sendButton->setEnabled(false);
    ui->textEditSession->setReadOnly(true);
    ui->textAppendUserPromptSlice->setReadOnly(true);

    if (ui->textAppendUserPromptSlice)
        ui->textAppendUserPromptSlice->clear();

    m_openAI->execute();

    statusBar()->showMessage("Prompt sent to OpenAI...");
}

void MainWindow::onOpenAIResponse(const QString &response)
{
    // Append the assistant response slice to the session
    QString sanitizedResponse = Session::sanitizeFencesRecursive(response);
    m_session->appendAssistantSlice(sanitizedResponse);

    // Save session file including new assistant slice
    m_session->save();

    // Regenerate full markdown including the new assistant reply
    QString fullMarkdown = m_session->compiledRawMarkdown();

    ui->textEditSession->blockSignals(true);
    ui->textEditSession->setPlainText(fullMarkdown);
    ui->textEditSession->blockSignals(false);

    // Re-enable send button and editors
    ui->sendButton->setEnabled(true);
    ui->textEditSession->setReadOnly(false);
    ui->textAppendUserPromptSlice->setReadOnly(false);

    // Place focus back on append user prompt slice box, ready for next input
    ui->textAppendUserPromptSlice->setFocus();

    statusBar()->showMessage("Response received and session updated");
}

void MainWindow::onOpenAIError(const QString &error)
{
    QMessageBox::critical(this, "OpenAI API Error", error);
    ui->sendButton->setEnabled(true);
    ui->textEditSession->setReadOnly(false);
    statusBar()->showMessage("OpenAI API encountered error");
}

void MainWindow::moveCursorToUserPrompt(QPlainTextEdit *editor)
{
    if (!editor)
        return;

    const QString text = editor->toPlainText();

    // A simple regex to find last "### User" header and fenced block start
    QRegularExpression userHeaderRe(R"(### User\s*(?:\(\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}\))?\s*```markdown\n)");

    // Find all matches, pick last
    QRegularExpressionMatchIterator i = userHeaderRe.globalMatch(text);

    int pos = -1;
    while (i.hasNext()) {
        QRegularExpressionMatch m = i.next();
        pos = m.capturedEnd(0); // position right after the fenced block start
    }

    if (pos < 0) {
        // No user prompt found, place cursor at document end
        editor->moveCursor(QTextCursor::End);
        return;
    }

    // Move cursor at the position 'pos', which is after the "```markdown\n" for last user slice
    QTextCursor cursor = editor->textCursor();
    cursor.setPosition(pos, QTextCursor::MoveAnchor);
    editor->setTextCursor(cursor);
    editor->setFocus();
}
