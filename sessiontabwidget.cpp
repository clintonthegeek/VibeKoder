#include "sessiontabwidget.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>

#include <QDebug>
#include <qsplitter.h>

SessionTabWidget::SessionTabWidget(const QString& sessionPath, Project* project, QWidget *parent)
    : QWidget(parent)
    , m_sessionFilePath(sessionPath)
    , m_project(project)
    , m_session(project)
{
    if (!m_session.load(m_sessionFilePath)) {
        qWarning() << "Failed loading session file " << m_sessionFilePath;
    }

    //setup OpenAI connections
    m_openAIRequest = new OpenAIRequest(this);
    connect(m_openAIRequest, &OpenAIRequest::requestFinished, this, &SessionTabWidget::onOpenAIResponse);
    connect(m_openAIRequest, &OpenAIRequest::requestError, this, &SessionTabWidget::onOpenAIError);

    // UI setup
    auto mainLayout = new QVBoxLayout(this);

    auto splitter = new QSplitter(Qt::Horizontal, this);
    mainLayout->addWidget(splitter);

    // Left: full session markdown editor
    m_sessionEditor = new QPlainTextEdit(splitter);
    m_sessionEditor->setPlainText("");
    m_sessionEditor->setLineWrapMode(QPlainTextEdit::NoWrap);

    // Right: control panel
    QWidget *rightPanel = new QWidget(splitter);
    auto rightLayout = new QVBoxLayout(rightPanel);

    // Prompt slice tree widget
    m_promptSliceTree = new QTreeWidget(rightPanel);
    m_promptSliceTree->setHeaderLabels({ "Timestamp", "Role", "Summary" });
    m_promptSliceTree->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    rightLayout->addWidget(m_promptSliceTree);

    // Fork button
    m_forkButton = new QPushButton("Fork Session Here", rightPanel);
    m_forkButton->setEnabled(false);
    rightLayout->addWidget(m_forkButton);

    // Command pipe output box
    m_commandOutput = new QTextEdit(rightPanel);
    m_commandOutput->setReadOnly(true);
    m_commandOutput->setMinimumHeight(100);
    rightLayout->addWidget(m_commandOutput);

    // Append user prompt input
    m_appendUserPrompt = new QPlainTextEdit(rightPanel);
    m_appendUserPrompt->setPlaceholderText("Type new user prompt slice here...");
    m_appendUserPrompt->setFixedHeight(80);
    rightLayout->addWidget(m_appendUserPrompt);

    // Send button
    m_sendButton = new QPushButton("Send", rightPanel);
    rightLayout->addWidget(m_sendButton);

    // Connections
    connect(m_forkButton, &QPushButton::clicked, this, &SessionTabWidget::onForkClicked);
    connect(m_sendButton, &QPushButton::clicked, this, &SessionTabWidget::onSendClicked);
    connect(m_promptSliceTree, &QTreeWidget::itemSelectionChanged, this, &SessionTabWidget::onPromptSliceSelected);

    loadSession();
}

SessionTabWidget::~SessionTabWidget()
{
    // Cleanup if needed
}

void SessionTabWidget::loadSession()
{
    if (!m_session.load(m_sessionFilePath)) {
        QMessageBox::warning(this, "Error", "Failed to load session file.");
        return;
    }

    // Show content in session editor
    m_sessionEditor->setPlainText(m_session.compiledRawMarkdown());

    // Build prompt slice tree
    buildPromptSliceTree();
}


void SessionTabWidget::onOpenAIResponse(const QString &responseText)
{
    qDebug() << "SessionTabWidget received OpenAI response, length:" << responseText.length();

    m_session.appendAssistantSlice(responseText);

    // Update editor and prompt tree
    m_sessionEditor->setPlainText(m_session.compiledRawMarkdown());
    buildPromptSliceTree();

    // Show in command output box, if any
    if (m_commandOutput) {
        m_commandOutput->append(responseText);
    }

    if (!saveSession()) {
        QMessageBox::warning(this, "Error", "Failed to save session after assistant response.");
    }

    // Re-enable send button
    if (m_sendButton)
        m_sendButton->setEnabled(true);
}

void SessionTabWidget::onOpenAIError(const QString &errorString)
{
    QMessageBox::critical(this, "OpenAI API Error", errorString);
    if (m_sendButton)
        m_sendButton->setEnabled(true);
}



void SessionTabWidget::buildPromptSliceTree()
{
    m_promptSliceTree->clear();

    auto slices = m_session.slices();

    for (int i = 0; i < slices.size(); ++i) {
        const PromptSlice &slice = slices[i];
        auto item = new QTreeWidgetItem(m_promptSliceTree);
        item->setData(0, Qt::UserRole, i);  // Store slice index
        item->setText(0, slice.timestamp);
        switch (slice.role) {
        case MessageRole::User: item->setText(1, "User"); break;
        case MessageRole::Assistant: item->setText(1, "Assistant"); break;
        case MessageRole::System: item->setText(1, "System"); break;
        }
        item->setText(2, promptSliceSummary(slice));
    }

    m_forkButton->setEnabled(false);
}

QString SessionTabWidget::promptSliceSummary(const PromptSlice &slice) const
{
    QString s = slice.content.trimmed().left(60);
    s.replace('\n', ' ');
    if (slice.content.length() > 60)
        s += "...";
    return s;
}

void SessionTabWidget::onPromptSliceSelected()
{
    auto selectedItems = m_promptSliceTree->selectedItems();
    m_forkButton->setEnabled(!selectedItems.isEmpty());
}

void SessionTabWidget::onForkClicked()
{
    auto selectedItems = m_promptSliceTree->selectedItems();
    if (selectedItems.isEmpty())
        return;

    int sliceIndex = selectedItems.first()->data(0, Qt::UserRole).toInt();

    // TODO: Implement fork logic here
    // Create new session with slices up to sliceIndex (inclusive)
    QMessageBox::information(this, "Fork", QString("Would fork session at slice %1").arg(sliceIndex));
}

void SessionTabWidget::onSendClicked()
{
    // 1. Reload session file from disk to capture any external edits
    bool reloaded = m_session.load(m_sessionFilePath);
    if (!reloaded) {
        QMessageBox::warning(this, "Error", "Failed to reload session file before sending. Sending aborted.");
        return;
    }

    // 2. Update GUI editor with freshly loaded markdown (optional)
    m_sessionEditor->setPlainText(m_session.compiledRawMarkdown());
    buildPromptSliceTree();

    QString newPrompt = m_appendUserPrompt->toPlainText().trimmed();

    if (!newPrompt.isEmpty()) {
        m_session.appendUserSlice(newPrompt);

        m_sessionEditor->setPlainText(m_session.compiledRawMarkdown());
        buildPromptSliceTree();

        m_appendUserPrompt->clear();

        // Save session file
        if (!saveSession()) {
            QMessageBox::warning(this, "Error", "Failed to save session after adding prompt.");
        }
    }

    QString prompt = m_session.compilePrompt();
    qDebug() << "SessionTabWidget sending prompt (length)" << prompt.length();

    // Prepare OpenAIRequest
    m_openAIRequest->setAccessToken(m_project->accessToken());
    m_openAIRequest->setModel(m_project->model());
    m_openAIRequest->setMaxTokens(m_project->maxTokens());
    m_openAIRequest->setTemperature(m_project->temperature());
    m_openAIRequest->setTopP(m_project->topP());
    m_openAIRequest->setFrequencyPenalty(m_project->frequencyPenalty());
    m_openAIRequest->setPresencePenalty(m_project->presencePenalty());
    m_openAIRequest->setPrompt(prompt);

    m_openAIRequest->execute();

    // Optionally disable UI controls here to prevent multiple sends
    m_sendButton->setEnabled(false);
}

bool SessionTabWidget::saveSession()
{
    if (m_sessionFilePath.isEmpty()) {
        QMessageBox::warning(this, "Error", "Session file path empty.");
        return false;
    }

    if (!m_session.save(m_sessionFilePath)) {
        QMessageBox::warning(this, "Error", "Failed to save session file.");
        return false;
    }

    return true;
}
