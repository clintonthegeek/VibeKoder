#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "project.h"
#include "session.h"
#include "openai_request.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>
#include <QDebug>

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
    if (!m_session || !m_project) {
        QMessageBox::warning(this, "Not Ready", "Load a project and session first.");
        return;
    }

    // Naively compile prompt, ignoring @commandpipe tokens (phase 1)
    QString prompt = m_session->compilePrompt();

    m_openAI->setAccessToken(m_project->accessToken());
    m_openAI->setModel(m_project->model());
    m_openAI->setPrompt(prompt);
    m_openAI->setMaxTokens(m_project->maxTokens());
    m_openAI->setTemperature(m_project->temperature());
    m_openAI->setTopP(m_project->topP());
    m_openAI->setFrequencyPenalty(m_project->frequencyPenalty());
    m_openAI->setPresencePenalty(m_project->presencePenalty());

    ui->textEditResponse->clear();
    m_openAI->execute();

    statusBar()->showMessage("Sent prompt to OpenAI...");
}

void MainWindow::onOpenAIResponse(const QString &response)
{
    // Append text in QPlainTextEdit:
    ui->textEditResponse->moveCursor(QTextCursor::End);
    ui->textEditResponse->insertPlainText(response + "\n");

    if (m_session) {
        m_session->appendAssistantSlice(response);
        m_session->save(QString());
    }
    statusBar()->showMessage("Response received and appended.");
}

void MainWindow::onOpenAIError(const QString &error)
{
    QMessageBox::critical(this, "OpenAI Error", error);
    statusBar()->showMessage("OpenAI API error");
}
