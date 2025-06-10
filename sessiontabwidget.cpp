#include "sessiontabwidget.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QKeyEvent>
#include <QSplitter>
#include <QDebug>
#include <QDesktopServices>
#include <QUrl>
#include <QContextMenuEvent>
#include <QRandomGenerator>
#include <QFileDialog>

SessionTabWidget::SessionTabWidget(const QString& sessionPath, Project* project, QWidget *parent, bool isTempSession)
    : QWidget(parent)
    , m_sessionFilePath(sessionPath)
    , m_project(project)
    , m_session(project)
    , m_updatingEditor(false)
    , m_isTempSession(isTempSession)
{
    qDebug() << "[SessionTabWidget] Constructor for session:" << sessionPath << "Widget:" << this;

    // Create OpenAIBackend instance
    m_aiBackend = new OpenAIBackend(this);

    // Set global config from project
    QVariantMap config;
    if (m_project) {
        config["access_token"] = m_project->accessToken();
        config["model"] = m_project->model();
        config["max_tokens"] = m_project->maxTokens();
        config["temperature"] = m_project->temperature();
        config["top_p"] = m_project->topP();
        config["frequency_penalty"] = m_project->frequencyPenalty();
        config["presence_penalty"] = m_project->presencePenalty();
    } else {
        // For temp session, set defaults or empty values
        // Eventually have app config with defaults.
        config["access_token"] = QString();
        config["model"] = QString("gpt-4.1-mini");
        config["max_tokens"] = 20000;
        config["temperature"] = 0.3;
        config["top_p"] = 1.0;
        config["frequency_penalty"] = 0.0;
        config["presence_penalty"] = 0.0;
    }
    m_aiBackend->setConfig(config);

    // Connect signals
    connect(m_aiBackend, &AIBackend::partialResponse, this, &SessionTabWidget::onPartialResponse);
    connect(m_aiBackend, &AIBackend::finished, this, &SessionTabWidget::onFinished);
    connect(m_aiBackend, &AIBackend::errorOccurred, this, &SessionTabWidget::onErrorOccurred);
    connect(m_aiBackend, &AIBackend::statusChanged, this, &SessionTabWidget::onStatusChanged);
    // UI setup
    auto mainLayout = new QVBoxLayout(this);

    // Splitter for prompt slice tree and slice editor
    auto splitter = new QSplitter(Qt::Vertical, this);
    mainLayout->addWidget(splitter);

    // Prompt slice tree widget
    m_promptSliceTree = new QTreeWidget(splitter);
    m_promptSliceTree->setHeaderLabels({ "Timestamp", "Role", "Summary" });
    m_promptSliceTree->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_promptSliceTree->setContextMenuPolicy(Qt::CustomContextMenu);

    // Connect right-click menu signal
    connect(m_promptSliceTree, &QWidget::customContextMenuRequested, this, [this](const QPoint &pos){
        QTreeWidgetItem* item = m_promptSliceTree->itemAt(pos);
        if (!item) {
            // No item under cursor, ignore or disable menu
            return;
        }

        if (!m_contextMenu) {
            m_contextMenu = new QMenu(this);

            QAction* forkAction = m_contextMenu->addAction("Fork Session Here");
            connect(forkAction, &QAction::triggered, this, &SessionTabWidget::onForkClicked);

            QAction* deleteToEndAction = m_contextMenu->addAction("Delete To End");
            connect(deleteToEndAction, &QAction::triggered, this, &SessionTabWidget::onDeleteToEndClicked);
        }

        // Store selected item before showing menu
        m_promptSliceTree->setCurrentItem(item);

        m_contextMenu->exec(m_promptSliceTree->viewport()->mapToGlobal(pos));
    });

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
    connect(m_forkButton, &QPushButton::clicked, this, &SessionTabWidget::onForkClicked);

    // Open Markdown File button
    m_openMarkdownButton = new QPushButton("Open Markdown File", this);
    buttonLayout->addWidget(m_openMarkdownButton);
    connect(m_openMarkdownButton, &QPushButton::clicked, this, &SessionTabWidget::onOpenMarkdownFileClicked);

    // Open Cache button
    m_openCacheButton = new QPushButton("Open Cache", this);
    buttonLayout->addWidget(m_openCacheButton);
    connect(m_openCacheButton, &QPushButton::clicked, this, &SessionTabWidget::onOpenCacheClicked);

    // Refresh from disk button
    auto refreshButton = new QPushButton("Refresh", this);
    buttonLayout->addWidget(refreshButton);
    connect(refreshButton, &QPushButton::clicked, this, &SessionTabWidget::onRefreshClicked);

    // save Temp session button
    if (m_isTempSession) {
        m_saveTempButton = new QPushButton("Save", this);
        buttonLayout->addWidget(m_saveTempButton);
        connect(m_saveTempButton, &QPushButton::clicked, this, &SessionTabWidget::onSaveTempSessionClicked);
    }

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
    connect(m_sendButton, &QPushButton::clicked, this, &SessionTabWidget::onSendClicked);
    connect(m_promptSliceTree, &QTreeWidget::itemSelectionChanged, this, &SessionTabWidget::onPromptSliceSelected);

    // Update slice content when edited
    connect(m_sliceEditor, &QTextEdit::textChanged, this, [this]() {
        if (m_updatingEditor)
            return; // Ignore programmatic updates

        auto selectedItems = m_promptSliceTree->selectedItems();
        if (selectedItems.isEmpty())
            return;

        int idx = selectedItems.first()->data(0, Qt::UserRole).toInt();
        QString newContent = m_sliceEditor->toPlainText();

        // Ignore if newContent is empty and slice content is already empty (no change)
        if (newContent.isEmpty() && m_session.promptSliceContent(idx).isEmpty())
            return;

        // qDebug() << "[m_sliceEditor::textChanged] Updating slice index:" << idx
        //          << "new content preview:\n" << newContent.left(200).replace('\n', "\\n");

        m_session.setPromptSliceContent(idx, newContent);
    });

    // Handle Shift+Enter in m_appendUserPrompt to trigger send
    m_appendUserPrompt->installEventFilter(this);

    loadSession();
}

SessionTabWidget::~SessionTabWidget()
{
    qDebug() << "[SessionTabWidget] Destructor for session:" << m_sessionFilePath << "Widget:" << this;
}

void SessionTabWidget::loadSession()
{
    if (!m_session.load(m_sessionFilePath)) {
        QMessageBox::warning(this, "Error", "Failed to load session file.");
        return;
    }

    buildPromptSliceTree();
}

void SessionTabWidget::onSaveTempSessionClicked()
{
    QString suggestedName = "temp_session.md";
    QString fileName = QFileDialog::getSaveFileName(this, "Save Temporary Session", suggestedName, "Markdown Files (*.md *.markdown);;All Files (*)");
    if (fileName.isEmpty())
        return;

    if (!m_session.save(fileName)) {
        QMessageBox::warning(this, "Save Failed", "Failed to save the session to the selected file.");
        return;
    }

    // Update internal session file path and mark as non-temp (optional)
    m_sessionFilePath = fileName;
    m_isTempSession = false;

    // Optionally disable Save button after saving
    if (m_saveTempButton) {
        m_saveTempButton->setEnabled(false);
        m_saveTempButton->hide();
    }

    // Optionally emit signal to notify MainWindow or TabManager
    emit tempSessionSaved(fileName);

    QMessageBox::information(this, "Saved", "Temporary session saved successfully.");
}

void SessionTabWidget::buildPromptSliceTree()
{
    m_promptSliceTree->clear();

    auto slices = m_session.slices();
    qDebug() << "[buildPromptSliceTree] Total slices:" << slices.size();

    for (int i = 0; i < slices.size(); ++i) {
        const PromptSlice &slice = slices[i];
        // qDebug() << "[buildPromptSliceTree] Slice" << i
        //          << "role:" << (slice.role == MessageRole::Assistant ? "Assistant" :
        //                             slice.role == MessageRole::User ? "User" : "System")
        //          << "content length:" << slice.content.length()
        //          << "content preview:" << slice.content.left(30);

        auto item = new QTreeWidgetItem(m_promptSliceTree);
        item->setData(0, Qt::UserRole, i);
        item->setText(0, slice.timestamp);
        switch (slice.role) {
        case MessageRole::User: item->setText(1, "User"); break;
        case MessageRole::Assistant: item->setText(1, "Assistant"); break;
        case MessageRole::System: item->setText(1, "System"); break;
        }
        item->setText(2, promptSliceSummary(slice));
    }

    m_forkButton->setEnabled(false);

    if (m_currentRequestId.isEmpty()) {
        m_sliceEditor->blockSignals(true);
        m_sliceEditor->clear();
        m_sliceEditor->setEnabled(false);
        m_sliceEditor->blockSignals(false);
    }

    int count = m_promptSliceTree->topLevelItemCount();
    if (count > 0) {
        m_promptSliceTree->setCurrentItem(m_promptSliceTree->topLevelItem(count - 1));
    }
}

QString SessionTabWidget::sessionFilePath() const
{
    return m_sessionFilePath;
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

void SessionTabWidget::onRefreshClicked()
{
    qDebug() << "[onRefreshClicked] User clicked Refresh.";
    if (m_sessionFilePath.isEmpty()) {
        QMessageBox::warning(this, "Refresh", "Session file path is empty.");
        return;
    }

    if (!m_session.load(m_sessionFilePath)) {
        QMessageBox::warning(this, "Refresh", "Failed to reload session file.");
        return;
    }

    buildPromptSliceTree();

    // Clear editor and disable until a slice is selected
    m_sliceEditor->blockSignals(true);
    m_sliceEditor->clear();
    m_sliceEditor->setEnabled(false);
    m_sliceEditor->blockSignals(false);

    m_partialResponseBuffer.clear();

    ///statusTip()->showMessage("Session refreshed from disk.", 2000);
}

void SessionTabWidget::onPromptSliceSelected()
{
    auto selectedItems = m_promptSliceTree->selectedItems();
    bool hasSelection = !selectedItems.isEmpty();
    m_forkButton->setEnabled(hasSelection);

    if (!hasSelection) {
        m_sliceEditor->blockSignals(true);
        m_sliceEditor->clear();
        m_sliceEditor->setEnabled(false);
        m_sliceEditor->blockSignals(false);
        qDebug() << "[onPromptSliceSelected] No selection, editor cleared";
        return;
    }

    int index = selectedItems.first()->data(0, Qt::UserRole).toInt();
    // qDebug() << "[onPromptSliceSelected] Index:" << index << "Slice count:" << m_session.slices().size();
    if (index < 0 || index >= m_session.slices().size()) {
        m_sliceEditor->blockSignals(true);
        m_sliceEditor->clear();
        m_sliceEditor->setEnabled(false);
        m_sliceEditor->blockSignals(false);
        return;
    }

    const PromptSlice &slice = m_session.slices().at(index);
    // qDebug() << "[onPromptSliceSelected] Loading slice index:" << index
    //          << "role:" << (slice.role == MessageRole::Assistant ? "Assistant" :
    //                             slice.role == MessageRole::User ? "User" : "System")
    //          << "content length:" << slice.content.length()
    //          << "content preview:" << slice.content.left(30);

    m_updatingEditor = true;
    m_sliceEditor->blockSignals(true);

    m_sliceEditor->setPlainText(slice.content);
    m_sliceEditor->setEnabled(!slice.content.isEmpty());

    m_sliceEditor->blockSignals(false);
    m_updatingEditor = false;
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

void SessionTabWidget::onDeleteToEndClicked()
{
    auto selectedItems = m_promptSliceTree->selectedItems();
    if (selectedItems.isEmpty())
        return;

    int sliceIndex = selectedItems.first()->data(0, Qt::UserRole).toInt();

    // Stub: Just show a message box for now
    QMessageBox::information(this, "Delete To End", QString("Would delete slices from %1 to end").arg(sliceIndex));

    // TODO: Implement actual deletion logic here
}

void SessionTabWidget::onOpenMarkdownFileClicked()
{
    if (m_sessionFilePath.isEmpty()) {
        QMessageBox::warning(this, "Open Markdown File", "Session file path is empty.");
        return;
    }

    QUrl fileUrl = QUrl::fromLocalFile(m_sessionFilePath);
    bool success = QDesktopServices::openUrl(fileUrl);
    if (!success) {
        QMessageBox::warning(this, "Open Markdown File", "Failed to open session markdown file.");
    }
}

void SessionTabWidget::onOpenCacheClicked()
{
    QString cacheFolder = m_session.sessionCacheFolder();
    if (cacheFolder.isEmpty() || !QDir(cacheFolder).exists()) {
        QMessageBox::warning(this, "Open Cache", "Cache folder does not exist.");
        return;
    }

    QUrl folderUrl = QUrl::fromLocalFile(cacheFolder);
    bool success = QDesktopServices::openUrl(folderUrl);
    if (!success) {
        QMessageBox::warning(this, "Open Cache", "Failed to open cache folder.");
    }
}

// contextMenuEvent implementation (empty stub or remove from header if unused)
void SessionTabWidget::contextMenuEvent(QContextMenuEvent *event)
{
    // If you handle context menu via customContextMenuRequested signal,
    // you can leave this empty or remove it from the header.
    event->ignore();
}

void SessionTabWidget::onSendClicked()
{
    // Reload session file to capture external edits
    if (!m_session.load(m_sessionFilePath)) {
        QMessageBox::warning(this, "Error", "Failed to reload session file before sending. Sending aborted.");
        return;
    }

    buildPromptSliceTree();

    QString newPrompt = m_appendUserPrompt->toPlainText().trimmed();

    if (!newPrompt.isEmpty()) {
        // Append user slice with content
        m_session.appendUserSlice(newPrompt);

        // Save session after appending user slice
        if (!m_session.save(m_sessionFilePath)) {
            QMessageBox::warning(this, "Error", "Failed to save session after adding prompt.");
            return;
        }

        // Run command pipes to generate cached files (e.g., amalgamate source)
        if (!m_session.runCommandPipes()) {
            QMessageBox::warning(this, "Error", "Failed to run command pipes in session.");
            return;
        }

        // Cache includes and save session
        if (!m_session.refreshCacheAndSave()) {
            QMessageBox::warning(this, "Error", "Failed to cache includes in session after running command pipes.");
            return;
        }

        // Clear input after saving
        m_appendUserPrompt->clear();

        // Rebuild slice tree to include new user slice and cached changes
        buildPromptSliceTree();
    } else {
        // No prompt to send, ignore
        return;
    }

    // Append empty assistant slice for streaming response
    m_session.appendAssistantSlice(QString());

    // Save session after adding assistant slice
    if (!m_session.save(m_sessionFilePath)) {
        QMessageBox::warning(this, "Error", "Failed to save session after adding assistant slice.");
        return;
    }

    // Run command pipes to generate cached files (e.g., amalgamate source)
    if (!m_session.runCommandPipes()) {
        QMessageBox::warning(this, "Error", "Failed to run command pipes in session.");
        return;
    }

    // Cache includes and save session
    if (!m_session.refreshCacheAndSave()) {
        QMessageBox::warning(this, "Error", "Failed to cache includes in session after running command pipes.");
        return;
    }

    // Add new item to tree for the assistant slice without rebuilding entire tree
    int lastIndex = m_session.slices().size() - 1;
    if (lastIndex >= 0) {
        auto slice = m_session.slices().at(lastIndex);
        auto item = new QTreeWidgetItem(m_promptSliceTree);
        item->setData(0, Qt::UserRole, lastIndex);
        item->setText(0, slice.timestamp);
        switch (slice.role) {
        case MessageRole::User: item->setText(1, "User"); break;
        case MessageRole::Assistant: item->setText(1, "Assistant"); break;
        case MessageRole::System: item->setText(1, "System"); break;
        }
        item->setText(2, promptSliceSummary(slice));

        m_promptSliceTree->setCurrentItem(item);
        m_sliceEditor->setEnabled(true);
        m_sliceEditor->clear();
        m_partialResponseBuffer.clear();
    }

    // Prepare messages for AIBackend
    QVector<PromptSlice> slices = m_session.expandedSlices();
    QList<AIBackend::Message> messages;
    for (const PromptSlice &slice : slices) {
        AIBackend::Message::Role role;
        switch (slice.role) {
        case MessageRole::System: role = AIBackend::Message::System; break;
        case MessageRole::User: role = AIBackend::Message::User; break;
        case MessageRole::Assistant: role = AIBackend::Message::Assistant; break;
        default: role = AIBackend::Message::Unknown; break;
        }
        messages.append({role, slice.content});
    }

    QVariantMap params; // Add per-request params if needed

    m_currentRequestId = QStringLiteral("session_%1_%2")
                             .arg(QDateTime::currentMSecsSinceEpoch())
                             .arg(QRandomGenerator::global()->bounded(INT_MAX));

    m_aiBackend->startRequest(messages, params, m_currentRequestId);

    m_sendButton->setEnabled(false);
}

void SessionTabWidget::onPartialResponse(const QString &requestId, const QString &partialText)
{
    if (requestId != m_currentRequestId)
        return;

    m_partialResponseBuffer += partialText;

    // Update slice editor without triggering textChanged signal
    m_sliceEditor->blockSignals(true);
    m_updatingEditor = true;
    m_sliceEditor->setPlainText(m_partialResponseBuffer);
    m_updatingEditor = false;
    m_sliceEditor->blockSignals(false);

    // Update assistant slice content in session
    QVector<PromptSlice> &slices = m_session.slices();
    int selectedIndex = m_promptSliceTree->indexOfTopLevelItem(m_promptSliceTree->currentItem());
    if (selectedIndex >= 0 && selectedIndex < slices.size()) {
        PromptSlice &slice = slices[selectedIndex];
        if (slice.role == MessageRole::Assistant) {
            slice.content = m_partialResponseBuffer;
        }
    }

    // Scroll to bottom
    QTextCursor cursor = m_sliceEditor->textCursor();
    cursor.movePosition(QTextCursor::End);
    m_sliceEditor->setTextCursor(cursor);
}

void SessionTabWidget::onFinished(const QString &requestId, const QString &fullResponse)
{
    if (requestId != m_currentRequestId)
        return;

    QVector<PromptSlice> &slices = m_session.slices();
    int selectedIndex = m_promptSliceTree->indexOfTopLevelItem(m_promptSliceTree->currentItem());
    if (selectedIndex >= 0 && selectedIndex < slices.size()) {
        PromptSlice &slice = slices[selectedIndex];
        if (slice.role == MessageRole::Assistant) {
            slice.content = fullResponse;
        }
    }

    buildPromptSliceTree();
    int lastIndex = m_promptSliceTree->topLevelItemCount() - 1;
    if (lastIndex >= 0) {
        m_promptSliceTree->setCurrentItem(m_promptSliceTree->topLevelItem(lastIndex));
    }

    m_sliceEditor->blockSignals(true);
    m_updatingEditor = true;
    m_sliceEditor->setPlainText(fullResponse);
    m_updatingEditor = false;    m_sliceEditor->blockSignals(false);

    if (!saveSession()) {
        QMessageBox::warning(this, "Error", "Failed to save session after assistant response.");
    }

    m_sendButton->setEnabled(true);
    m_currentRequestId.clear();
    m_partialResponseBuffer.clear();
}

void SessionTabWidget::onErrorOccurred(const QString &requestId, const QString &errorString)
{
    if (requestId != m_currentRequestId)
        return;

    QMessageBox::critical(this, "AI Backend Error", errorString);

    m_sendButton->setEnabled(true);
    m_currentRequestId.clear();
    m_partialResponseBuffer.clear();
}

void SessionTabWidget::updateBackendConfig(const QVariantMap &config)
{
    if (m_aiBackend) {
        m_aiBackend->setConfig(config);
    }
}

void SessionTabWidget::onStatusChanged(const QString &requestId, const QString &status)
{
    Q_UNUSED(requestId);
    qDebug() << "[AIBackend] Status changed:" << status;
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
