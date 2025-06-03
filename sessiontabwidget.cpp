#include "sessiontabwidget.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QKeyEvent>
#include <QSplitter>
#include <QDebug>

SessionTabWidget::SessionTabWidget(const QString& sessionPath, Project* project, QWidget *parent)
    : QWidget(parent)
    , m_sessionFilePath(sessionPath)
    , m_project(project)
    , m_session(project)
{
    if (!m_session.load(m_sessionFilePath)) {
        qWarning() << "Failed loading session file " << m_sessionFilePath;
    }

    // Setup OpenAI connections
    m_openAIRequest = new OpenAIRequest(this);
    connect(m_openAIRequest, &OpenAIRequest::requestFinished, this, &SessionTabWidget::onOpenAIResponse);
    connect(m_openAIRequest, &OpenAIRequest::requestError, this, &SessionTabWidget::onOpenAIError);

    // UI setup
    auto mainLayout = new QVBoxLayout(this);

    // Splitter for prompt slice tree and slice editor
    auto splitter = new QSplitter(Qt::Vertical, this);
    mainLayout->addWidget(splitter);

    // Prompt slice tree widget
    m_promptSliceTree = new QTreeWidget(splitter);
    m_promptSliceTree->setHeaderLabels({ "Timestamp", "Role", "Summary" });
    m_promptSliceTree->header()->setSectionResizeMode(QHeaderView::ResizeToContents);

    // Editable slice editor (replaces m_commandOutput)
    m_sliceEditor = new QTextEdit(splitter);
    m_sliceEditor->setAcceptRichText(false);
    m_sliceEditor->setPlaceholderText("Edit selected prompt slice here...");
    m_sliceEditor->setEnabled(false); // Disabled until a slice is selected

    // Horizontal layout for buttons
    auto buttonLayout = new QHBoxLayout();
    mainLayout->addLayout(buttonLayout);

    // Fork button
    m_forkButton = new QPushButton("Fork Session Here", this);
    m_forkButton->setEnabled(false);
    buttonLayout->addWidget(m_forkButton);

    // Spacer for future buttons
    buttonLayout->addStretch();

    // Append user prompt input
    m_appendUserPrompt = new QPlainTextEdit(this);
    m_appendUserPrompt->setPlaceholderText("Type new user prompt slice here...");
    m_appendUserPrompt->setFixedHeight(240);
    mainLayout->addWidget(m_appendUserPrompt);

    // Send button
    m_sendButton = new QPushButton("Send", this);
    mainLayout->addWidget(m_sendButton);

    // Connections
    connect(m_forkButton, &QPushButton::clicked, this, &SessionTabWidget::onForkClicked);
    connect(m_sendButton, &QPushButton::clicked, this, &SessionTabWidget::onSendClicked);
    connect(m_promptSliceTree, &QTreeWidget::itemSelectionChanged, this, &SessionTabWidget::onPromptSliceSelected);

    // Update slice content when edited
    connect(m_sliceEditor, &QTextEdit::textChanged, this, [this]() {
        auto selectedItems = m_promptSliceTree->selectedItems();
        if (selectedItems.isEmpty())
            return;

        int idx = selectedItems.first()->data(0, Qt::UserRole).toInt();
        QString newContent = m_sliceEditor->toPlainText();

        qDebug() << "[m_sliceEditor::textChanged] Updating slice index:" << idx
                 << "new content preview:\n" << newContent.left(200).replace('\n', "\\n");

        m_session.setPromptSliceContent(idx, newContent);
    });

    // Handle Shift+Enter in m_appendUserPrompt to trigger send
    m_appendUserPrompt->installEventFilter(this);

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

    buildPromptSliceTree();

    // Clear slice editor and disable until selection
    m_sliceEditor->clear();
    m_sliceEditor->setEnabled(false);
}

void SessionTabWidget::onOpenAIResponse(const QString &responseText)
{
    qDebug() << "SessionTabWidget received OpenAI response, length:" << responseText.length();

    m_session.appendAssistantSlice(responseText);

    buildPromptSliceTree();

    // Show response in slice editor if last slice is assistant and selected
    auto slices = m_session.slices();
    int lastIndex = slices.size() - 1;
    if (lastIndex >= 0) {
        auto selectedItems = m_promptSliceTree->selectedItems();
        if (!selectedItems.isEmpty() && selectedItems.first()->data(0, Qt::UserRole).toInt() == lastIndex) {
            m_sliceEditor->setPlainText(responseText);
        }
    }

    if (!saveSession()) {
        QMessageBox::warning(this, "Error", "Failed to save session after assistant response.");
    }

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

    // Clear and disable slice editor on rebuild
    m_sliceEditor->clear();
    m_sliceEditor->setEnabled(false);
}

QString SessionTabWidget::promptSliceSummary(const PromptSlice &slice) const
{
    QString s = slice.content.trimmed();

    // Truncate to 60 chars for display
    s = s.left(60);
    s.replace('\n', ' ');
    if (slice.content.length() > 60)
        s += "...";

    return s;
}

void SessionTabWidget::onPromptSliceSelected()
{
    auto selectedItems = m_promptSliceTree->selectedItems();
    bool hasSelection = !selectedItems.isEmpty();
    m_forkButton->setEnabled(hasSelection);

    if (!hasSelection) {
        m_sliceEditor->clear();
        m_sliceEditor->setEnabled(false);
        return;
    }

    int idx = selectedItems.first()->data(0, Qt::UserRole).toInt();
    QString content = m_session.promptSliceContent(idx);
    qDebug() << "[onPromptSliceSelected] Loading slice index:" << idx
             << "content preview:\n" << content.left(200).replace('\n', "\\n");

    // Block signals to avoid recursive update when setting text
    m_sliceEditor->blockSignals(true);
    m_sliceEditor->setPlainText(content);
    m_sliceEditor->blockSignals(false);

    m_sliceEditor->setEnabled(true);
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

    buildPromptSliceTree();

    QString newPrompt = m_appendUserPrompt->toPlainText().trimmed();

    if (!newPrompt.isEmpty()) {
        m_session.appendUserSlice(newPrompt);

        buildPromptSliceTree();

        m_appendUserPrompt->clear();

        // Save session file
        if (!saveSession()) {
            QMessageBox::warning(this, "Error", "Failed to save session after adding prompt.");
        }
    }

    // Step 3 addition: Run command pipes before caching includes and compiling prompt
    if (!m_session.runCommandPipes()) {
        QMessageBox::warning(this, "Error", "Failed to run command pipes in session.");
        return;
    }

    // Perform caching of includes in the session slices, update slices, save session again
    if (!m_session.refreshCacheAndSave()) {
        QMessageBox::warning(this, "Error", "Failed to cache includes in session after adding prompt.");
    } else {
        // Reload prompt tree and slice editor to show updated cached content
        loadSession();
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

    // Disable send button to prevent multiple sends
    m_sendButton->setEnabled(false);
}

QString SessionTabWidget::sessionFilePath() const
{
    return m_sessionFilePath;
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

// Event filter to catch Shift+Enter in m_appendUserPrompt
bool SessionTabWidget::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == m_appendUserPrompt && event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
            if (keyEvent->modifiers() & Qt::ShiftModifier) {
                // Trigger send
                if (m_sendButton->isEnabled()) {
                    onSendClicked();
                    return true; // event handled
                }
            }
        }
    }
    return QWidget::eventFilter(obj, event);
}
