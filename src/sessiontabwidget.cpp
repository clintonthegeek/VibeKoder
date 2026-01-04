#include "sessiontabwidget.h"
#include "appconfig.h"
#include "descriptiongenerator.h"

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
#include <QStatusBar>
#include <QApplication>
#include <QDialog>
#include <QLineEdit>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QFileDialog>
#include <QDateTime>
#include <QLabel>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QRegularExpression>  // make sure this is included
#include <QEventLoop>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QToolTip>

SessionTabWidget::SessionTabWidget(const QString& sessionPath, Project* project, QWidget *parent, bool isTempSession, QStatusBar* statusBar)
    : QWidget(parent)
    , m_sessionFilePath(sessionPath)
    , m_project(project)
    , m_session(project)
    , m_updatingEditor(false)
    , m_isTempSession(isTempSession)
    , m_unsavedChanges(false)
    , m_statusBar(statusBar)
{
    qDebug() << "[SessionTabWidget] Constructor for session:" << sessionPath << "Widget:" << this;

    // Create OpenAIBackend instance
    m_aiBackend = new OpenAIBackend(this);

    QVariantMap config;

    if (m_project) {
        config["access_token"] = m_project->accessToken();
        config["model"] = m_project->model();
        config["max_tokens"] = m_project->maxTokens();
        config["temperature"] = m_project->temperature();
        config["top_p"] = m_project->topP();
        config["frequency_penalty"] = m_project->frequencyPenalty();
        config["presence_penalty"] = m_project->presencePenalty();
        qDebug() << "[SessionTabWidget] Loaded aiBackend config from Project config.";

    } else {
        QVariantMap appDefaults = AppConfig::instance().getConfigMap();
        if (appDefaults.contains("default_project_settings")) {
            QVariantMap defaultProj = appDefaults.value("default_project_settings").toMap();
            if (defaultProj.contains("api") && defaultProj.value("api").canConvert<QVariantMap>()) {
                config = defaultProj.value("api").toMap();
            } else {
                config = defaultProj;
            }
        }
        qDebug() << "[SessionTabWidget] No Project found! Loaded aiBackend config from App Defaults";

    }

    m_aiBackend->setConfig(config);

    // Connect AI backend signals
    connect(m_aiBackend, &AIBackend::partialResponse, this, &SessionTabWidget::onPartialResponse);
    connect(m_aiBackend, &AIBackend::finished, this, &SessionTabWidget::onFinished);
    connect(m_aiBackend, &AIBackend::errorOccurred, this, &SessionTabWidget::onErrorOccurred);
    connect(m_aiBackend, &AIBackend::statusChanged, this, &SessionTabWidget::onStatusChanged);

    // === UI setup ===
    // === New top button row above slice tree ===
    auto mainLayout = new QVBoxLayout(this);

    m_sessionTitleLabel = new QLabel(this);
    m_sessionTitleLabel->setTextFormat(Qt::RichText);
    m_sessionTitleLabel->setWordWrap(true);
    m_sessionTitleLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_sessionTitleLabel->setStyleSheet("font-weight: bold; font-size: 16px; margin-bottom: 8px;");

    if (mainLayout) {
        mainLayout->insertWidget(0, m_sessionTitleLabel);
    }

    auto topButtonLayout = new QHBoxLayout();
    m_openMarkdownButton = new QPushButton("Open Markdown File", this);
    m_openCacheButton = new QPushButton("Open Cache", this);
    m_refreshButton = new QPushButton("Refresh", this);
    // In SessionTabWidget constructor or UI setup
    m_editTitleDescBtn = new QPushButton("Description", this);
    topButtonLayout->addWidget(m_editTitleDescBtn);
    connect(m_editTitleDescBtn, &QPushButton::clicked, this, &SessionTabWidget::onEditTitleDescClicked);
    m_editTitleDescBtn->installEventFilter(this);


    topButtonLayout->addWidget(m_openMarkdownButton);
    topButtonLayout->addWidget(m_openCacheButton);
    topButtonLayout->addWidget(m_editTitleDescBtn);

    topButtonLayout->addStretch(); // Push buttons to the left
    topButtonLayout->addWidget(m_refreshButton);

    mainLayout->addLayout(topButtonLayout);

    // Connect top buttons
    connect(m_openMarkdownButton, &QPushButton::clicked, this, &SessionTabWidget::onOpenMarkdownFileClicked);
    connect(m_openCacheButton, &QPushButton::clicked, this, &SessionTabWidget::onOpenCacheClicked);
    connect(m_refreshButton, &QPushButton::clicked, this, &SessionTabWidget::onRefreshClicked);

    // === Splitters and widgets as before ===
    auto mainSplitter = new QSplitter(Qt::Vertical, this);
    mainLayout->addWidget(mainSplitter);

    m_promptSliceTree = new QTreeWidget(mainSplitter);
    m_promptSliceTree->setHeaderLabels({ "Timestamp", "Role", "Summary" });
    m_promptSliceTree->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_promptSliceTree->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_promptSliceTree, &QWidget::customContextMenuRequested, this, [this](const QPoint &pos){
        QTreeWidgetItem* item = m_promptSliceTree->itemAt(pos);
        if (!item)
            return;
        if (!m_contextMenu) {
            m_contextMenu = new QMenu(this);
            QAction* forkAction = m_contextMenu->addAction("Fork Session Here");
            connect(forkAction, &QAction::triggered, this, &SessionTabWidget::onForkClicked);
            QAction* deleteAfterAction = m_contextMenu->addAction("Delete All After");
            connect(deleteAfterAction, &QAction::triggered, this, &SessionTabWidget::onDeleteAfterClicked);
        }
        m_promptSliceTree->setCurrentItem(item);
        m_contextMenu->exec(m_promptSliceTree->viewport()->mapToGlobal(pos));
    });

    if (!m_contextMenu) {
        m_contextMenu = new QMenu(this);
        QAction* forkAction = m_contextMenu->addAction("Fork Session Here");
        connect(forkAction, &QAction::triggered, this, &SessionTabWidget::onForkClicked);
        QAction* deleteAfterAction = m_contextMenu->addAction("Delete All After");
        connect(deleteAfterAction, &QAction::triggered, this, &SessionTabWidget::onDeleteAfterClicked);

        // Add new action for saving slice as markdown
        QAction* saveSliceAsMarkdownAction = m_contextMenu->addAction("Export Slice");
        connect(saveSliceAsMarkdownAction, &QAction::triggered, this, &SessionTabWidget::onSaveSliceAsMarkdown);
    }

    // Bottom splitter for slice viewer and append user prompt
    auto bottomSplitter = new QSplitter(Qt::Vertical, mainSplitter);

    m_sliceViewer = new QMarkdownTextEdit(bottomSplitter);
    //m_sliceViewer->setAcceptRichText(false);
    m_sliceViewer->setReadOnly(true);

    m_appendUserPrompt = new QPlainTextEdit(bottomSplitter);
    m_appendUserPrompt->setPlaceholderText("Type new user prompt slice here...");

    mainSplitter->setStretchFactor(0, 1);
    mainSplitter->setStretchFactor(1, 2);
    bottomSplitter->setStretchFactor(0, 3);
    bottomSplitter->setStretchFactor(1, 1);

    // === Bottom button row with Send and Save and Edit===
    auto bottomButtonLayout = new QHBoxLayout();
    m_sendButton = new QPushButton("Send All Slices", this);
    m_saveButton = new QPushButton("Save", this);

    // Edit tool button and space-reserver (initially hidden)
    m_editToolButton = new QToolButton(this);
    m_editToolButton->setText("Edit");
    m_editToolButton->setPopupMode(QToolButton::InstantPopup);
    m_editToolButton->setVisible(false); // hidden initially
    m_editSpacer = new QWidget(this);
    m_editSpacer->setFixedWidth(m_editToolButton->sizeHint().width());
    m_editSpacer->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);

    // Create menu for edit button
    m_editMenu = new QMenu(m_editToolButton);
    QAction* deleteAfterAction = m_editMenu->addAction("Delete All After");
    QAction* forkSessionAction = m_editMenu->addAction("Fork Session Here");
    m_editToolButton->setMenu(m_editMenu);

    // Connect menu actions
    connect(deleteAfterAction, &QAction::triggered, this, &SessionTabWidget::onDeleteAfterClicked);
    connect(forkSessionAction, &QAction::triggered, this, &SessionTabWidget::onForkClicked);

    // Make Send button expand maximally
    m_sendButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    // Save button minimum size only
    m_saveButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
    m_editToolButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);

    bottomButtonLayout->addWidget(m_sendButton);
    bottomButtonLayout->addWidget(m_editSpacer);
    bottomButtonLayout->addWidget(m_editToolButton);
    bottomButtonLayout->addWidget(m_saveButton);
    mainLayout->addLayout(bottomButtonLayout);

    // Connect bottom buttons
    connect(m_sendButton, &QPushButton::clicked, this, &SessionTabWidget::onSendClicked);
    connect(m_saveButton, &QPushButton::clicked, this, &SessionTabWidget::onSaveClicked);

    // Connect slice tree selection
    connect(m_promptSliceTree, &QTreeWidget::itemSelectionChanged, this, &SessionTabWidget::onPromptSliceSelected);

    connect(m_appendUserPrompt, &QPlainTextEdit::textChanged, this, [this]() {
        if (m_updatingEditor)
            return; // Ignore programmatic changes

        const QString currentText = m_appendUserPrompt->toPlainText();
        bool changed = (currentText != m_lastSavedUserPromptText);
        markUnsavedChanges(changed);

        // Enable Send button if non-empty
        bool hasText = !currentText.trimmed().isEmpty();
        m_sendButton->setEnabled(hasText);
    });

    // Install event filter for Shift+Enter send shortcut
    m_appendUserPrompt->installEventFilter(this);

    // Load session file and build UI
    loadSession();
}

//for description tooltip
QString wrapText(const QString &text, int maxLength = 80)
{
    QString result;
    int start = 0;
    int len = text.length();

    while (start < len) {
        // Determine the end index for this segment
        int end = start + maxLength;
        if (end >= len) {
            // Remaining text fits within maxLength
            result += text.mid(start);
            break;
        }

        // Look backward from 'end' to find a space or hyphen to break at
        int breakPos = -1;
        for (int i = end; i > start; --i) {
            QChar c = text.at(i);
            if (c.isSpace() || c == QChar('-')) {
                breakPos = i;
                break;
            }
        }

        if (breakPos == -1) {
            // No suitable break found, break at maxLength anyway
            breakPos = end;
        }

        // Append the substring and a newline
        result += text.mid(start, breakPos - start + 1).trimmed() + '\n';

        // Move start index past the break position
        start = breakPos + 1;
    }

    return result;
}

void SessionTabWidget::loadHeaderMetadataToUi()
{
    if (!m_project)
        return;

    QVariantMap header = m_session.headerMetadata();
    m_titleEdit->setText(header.value("title").toString());
    m_descriptionEdit->setPlainText(header.value("description").toString());
}

void SessionTabWidget::saveHeaderMetadataFromUi()
{
    QVariantMap header = m_session.headerMetadata();
    header["title"] = m_titleEdit->text().trimmed();
    header["description"] = m_descriptionEdit->toPlainText().trimmed();
    m_session.setHeaderMetadata(header);

    // Update session title label and description tooltip
    QString title = header.value("title").toString();
    if (title.isEmpty())
        title = "(Untitled Session)";
    if (m_sessionTitleLabel)
        m_sessionTitleLabel->setText(title);

    QString description = header.value("description").toString();
    if (description.isEmpty())
        description = "(No description)";
    if (m_editTitleDescBtn)
        m_editTitleDescBtn->setToolTip(wrapText(description));
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

    // Update session title label text from session metadata
    QString title = m_session.headerTitle();
    if (title.isEmpty())
        title = "(Untitled Session)";
    m_sessionTitleLabel->setText(title);

    // Update description button tooltip as well
    QString description = m_session.headerDescription();
    if (description.isEmpty())
        description = "(No description)";
    if (m_editTitleDescBtn) {
        m_editTitleDescBtn->setToolTip(wrapText(description));
    }

    // Explicitly select last slice and update UI
    int lastIndex = m_session.slices().size() - 1;
    if (lastIndex >= 0) {
        auto lastItem = m_promptSliceTree->topLevelItem(lastIndex);
        if (lastItem) {
            m_promptSliceTree->setCurrentItem(lastItem);
            updateUiForSelectedSlice(lastIndex);
        }
    }
}

void SessionTabWidget::onSaveSliceAsMarkdown()
{
    auto selectedItems = m_promptSliceTree->selectedItems();
    if (selectedItems.isEmpty()) {
        QMessageBox::warning(this, "Save Slice", "No slice selected.");
        return;
    }
    int index = selectedItems.first()->data(0, Qt::UserRole).toInt();
    if (index < 0 || index >= m_session.slices().size()) {
        QMessageBox::warning(this, "Save Slice", "Invalid slice selected.");
        return;
    }
    const PromptSlice &slice = m_session.slices().at(index);

    // Prepare default file name: "Session#SliceTIMESTAMP.md"
    QString timestampSafe = slice.timestamp;
    timestampSafe.replace(QRegularExpression("[^0-9]"), ""); // remove non-digits for filename
    QString baseName = QString("Session%1_Slice%2")
                           .arg(QFileInfo(m_sessionFilePath).completeBaseName())
                           .arg(timestampSafe);
    QString defaultExtension = "md";

    // Default docs folder path
    QString docsFolder;
    if (m_project) {
        docsFolder = m_project->docsFolder();
        if (!QDir(docsFolder).isAbsolute()) {
            docsFolder = QDir(m_project->rootFolder()).filePath(docsFolder);
        }
    }
    if (docsFolder.isEmpty())
        docsFolder = QDir::homePath();

    // Create dialog
    QDialog dlg(this);
    dlg.setWindowTitle("Export Slice");
    auto formLayout = new QFormLayout(&dlg);

    // Filename part 1 (name without extension)
    auto fileNameEdit = new QLineEdit(baseName, &dlg);
    // Filename part 2 (extension)
    auto fileExtEdit = new QLineEdit(defaultExtension, &dlg);
    fileExtEdit->setFixedWidth(50);

    // Container widget for filename parts
    QWidget *fileNameWidget = new QWidget(&dlg);
    auto hLayout = new QHBoxLayout(fileNameWidget);
    hLayout->setContentsMargins(0, 0, 0, 0);
    hLayout->addWidget(fileNameEdit);
    QLabel *dotLabel = new QLabel(".", fileNameWidget);
    dotLabel->setAlignment(Qt::AlignCenter);
    hLayout->addWidget(dotLabel);
    hLayout->addWidget(fileExtEdit);
    formLayout->addRow("File Name:", fileNameWidget);

    // Directory selector with button
    QWidget *dirWidget = new QWidget(&dlg);
    auto dirLayout = new QHBoxLayout(dirWidget);
    dirLayout->setContentsMargins(0, 0, 0, 0);
    auto dirEdit = new QLineEdit(docsFolder, dirWidget);
    auto browseBtn = new QPushButton("Browse...", dirWidget);
    dirLayout->addWidget(dirEdit);
    dirLayout->addWidget(browseBtn);
    formLayout->addRow("Directory:", dirWidget);
    connect(browseBtn, &QPushButton::clicked, this, [dirEdit, this]() {
        QString dir = QFileDialog::getExistingDirectory(this, "Select Directory", dirEdit->text());
        if (!dir.isEmpty()) {
            dirEdit->setText(dir);
        }
    });

    // Formatting group box with two checkboxes
    QGroupBox *formatGroup = new QGroupBox("Formatting", &dlg);
    auto formatLayout = new QVBoxLayout(formatGroup);
    auto trimHeaderCheck = new QCheckBox("Trim header above first ---", formatGroup);
    auto trimFooterCheck = new QCheckBox("Trim footer below last ---", formatGroup);
    trimHeaderCheck->setChecked(false);
    trimFooterCheck->setChecked(false);
    formatLayout->addWidget(trimHeaderCheck);
    formatLayout->addWidget(trimFooterCheck);
    formLayout->addRow(formatGroup);

    // Checkbox for open after save
    auto openCheck = new QCheckBox("Open after saving", &dlg);
    openCheck->setChecked(true);
    formLayout->addRow(openCheck);

    // Dialog buttons
    auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    formLayout->addRow(buttonBox);
    connect(buttonBox, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    // Select all text in filename part when dialog opens
    dlg.open();
    fileNameEdit->selectAll();
    fileNameEdit->setFocus();

    if (dlg.exec() != QDialog::Accepted)
        return;

    QString fileNamePart = fileNameEdit->text().trimmed();
    QString fileExtPart = fileExtEdit->text().trimmed();
    QString directory = dirEdit->text().trimmed();

    if (fileNamePart.isEmpty()) {
        QMessageBox::warning(this, "Save Slice", "File name cannot be empty.");
        return;
    }
    if (fileExtPart.isEmpty()) {
        QMessageBox::warning(this, "Save Slice", "File extension cannot be empty.");
        return;
    }
    if (directory.isEmpty() || !QDir(directory).exists()) {
        QMessageBox::warning(this, "Save Slice", "Directory does not exist.");
        return;
    }

    QString fullFileName = fileNamePart + "." + fileExtPart;
    QString fullPath = QDir(directory).filePath(fullFileName);

    QString contentToSave = slice.content;

    // Apply trimming if requested
    if (trimHeaderCheck->isChecked()) {
        // Regex to match a line that consists exactly of ---
        QRegularExpression headerRe("^---\\s*$", QRegularExpression::CaseInsensitiveOption | QRegularExpression::MultilineOption);

        // Remove everything above first line starting with ---
        int firstHeaderIndex = contentToSave.indexOf(headerRe);
        if (firstHeaderIndex != -1) {
            // Move past the line with ---
            int afterHeaderIndex = contentToSave.indexOf('\n', firstHeaderIndex);
            if (afterHeaderIndex != -1)
                contentToSave = contentToSave.mid(afterHeaderIndex + 1);
            else
                contentToSave.clear();
        }
        // Trim leading empty lines
        contentToSave = contentToSave.trimmed();
    }

    if (trimFooterCheck->isChecked()) {
        QRegularExpression headerRe("^---\\s*$", QRegularExpression::CaseInsensitiveOption | QRegularExpression::MultilineOption);

        // Remove everything below last line starting with ---
        int lastHeaderIndex = contentToSave.lastIndexOf(headerRe);
        if (lastHeaderIndex != -1) {
            contentToSave = contentToSave.left(lastHeaderIndex);
        }
        // Trim trailing empty lines
        contentToSave = contentToSave.trimmed();
    }

    // Save slice content to file
    QFile outFile(fullPath);
    if (!outFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Save Slice", QString("Failed to open file for writing:\n%1").arg(fullPath));
        return;
    }
    QTextStream out(&outFile);
    out << contentToSave;
    outFile.close();

    if (openCheck->isChecked()) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(fullPath));
    }

    if (m_statusBar) {
        m_statusBar->showMessage(QString("Saved slice to %1").arg(fullPath), 3000);
    }
}

void SessionTabWidget::buildPromptSliceTree()
{
    m_promptSliceTree->clear();

    // Clear cached user prompt text to avoid stale input after any rebuild
    m_pendingUserPromptText.clear();

    const auto &slices = m_session.slices();
    qDebug() << "[buildPromptSliceTree] Total slices:" << slices.size();

    for (int i = 0; i < slices.size(); ++i) {
        const PromptSlice &slice = slices[i];
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

    // Optionally, select last slice by default after rebuild
    int count = m_promptSliceTree->topLevelItemCount();
    if (count > 0) {
        m_promptSliceTree->setCurrentItem(m_promptSliceTree->topLevelItem(count - 1));
    }

    // Reset unsaved changes tracking since we just rebuilt UI from saved state
    m_unsavedChanges = false;
    m_saveButton->setEnabled(false);
}

QString SessionTabWidget::promptSliceSummary(const PromptSlice &slice) const
{
    QString s = slice.content.trimmed();
    s = s.left(60);
    s.replace('\n', ' ');
    if (slice.content.length() > 60)
        s += "...";
    return s;
}

void SessionTabWidget::onPromptSliceSelected()
{
    auto selectedItems = m_promptSliceTree->selectedItems();
    if (selectedItems.isEmpty()) {
        m_updatingEditor = true;
        m_sliceViewer->clear();
        m_sliceViewer->setEnabled(false);
        m_appendUserPrompt->setEnabled(false);
        m_saveButton->setEnabled(false);
        m_sendButton->setEnabled(false);
        m_updatingEditor = false;
        return;
    }

    int index = selectedItems.first()->data(0, Qt::UserRole).toInt();
    if (index < 0 || index >= m_session.slices().size()) {
        m_updatingEditor = true;
        m_sliceViewer->clear();
        m_sliceViewer->setEnabled(false);
        m_appendUserPrompt->setEnabled(false);
        m_saveButton->setEnabled(false);
        m_sendButton->setEnabled(false);
        m_updatingEditor = false;
        return;
    }

    updateUiForSelectedSlice(index);
}

void SessionTabWidget::updateUiForSelectedSlice(int selectedIndex)
{
    const auto &slices = m_session.slices();
    int lastIndex = slices.size() - 1;

    if (lastIndex < 0) {
        // No slices: disable editors and buttons
        m_updatingEditor = true;
        m_sliceViewer->clear();
        m_sliceViewer->setEnabled(false);
        m_appendUserPrompt->clear();
        m_appendUserPrompt->setEnabled(false);
        m_saveButton->setEnabled(false);
        m_sendButton->setEnabled(false);
        m_updatingEditor = false;
        return;
    }

    const PromptSlice &selectedSlice = slices[selectedIndex];
    const PromptSlice &lastSlice = slices[lastIndex];

    m_updatingEditor = true;
    m_partialResponseBuffer.clear();

    // Save current user input if m_appendUserPrompt is visible and enabled before changing UI
    if (m_appendUserPrompt->isVisible() && m_appendUserPrompt->isEnabled()) {
        m_pendingUserPromptText = m_appendUserPrompt->toPlainText();
    }

    if (lastIndex == 0) {
        const PromptSlice &onlySlice = slices.first();
        if (onlySlice.role == MessageRole::System) {
            // Show system prompt in slice viewer
            m_updatingEditor = true;

            m_sliceViewer->show();
            m_sliceViewer->setEnabled(true);
            m_sliceViewer->setPlainText(onlySlice.content);

            m_appendUserPrompt->show();
            m_appendUserPrompt->setEnabled(true);

            m_sendButton->setEnabled(false);
            m_saveButton->setEnabled(false);

            m_editToolButton->setVisible(false);
            m_editSpacer->setVisible(true);

            m_updatingEditor = false;
            return;
        }
    }

    if (selectedIndex == lastIndex) {
        // Last slice selected
        if (lastSlice.role == MessageRole::User) {
            // Last slice is user-role: show m_appendUserPrompt full height, hide m_sliceViewer
            m_sliceViewer->hide();
            m_sliceViewer->clear();
            m_sliceViewer->setEnabled(false);

            m_appendUserPrompt->show();
            m_appendUserPrompt->setEnabled(true);

            // Restore pending text if any, else load from slice content
            if (!m_pendingUserPromptText.isEmpty()) {
                m_appendUserPrompt->setPlainText(m_pendingUserPromptText);
            } else {
                m_appendUserPrompt->setPlainText(selectedSlice.content);
            }

            m_lastSavedUserPromptText = m_appendUserPrompt->toPlainText();
            markUnsavedChanges(false);

            // Buttons enabled only if m_appendUserPrompt non-empty
            bool hasText = !m_appendUserPrompt->toPlainText().trimmed().isEmpty();
            m_sendButton->setEnabled(hasText);
            m_saveButton->setEnabled(false); // no unsaved changes yet

        } else if (lastSlice.role == MessageRole::Assistant) {
            // Last slice is assistant-role: split view with m_sliceViewer and m_appendUserPrompt
            m_sliceViewer->show();
            m_sliceViewer->setEnabled(true);
            m_sliceViewer->setPlainText(selectedSlice.content);

            m_appendUserPrompt->show();
            m_appendUserPrompt->setEnabled(true);

            // Restore pending text if any, else clear for new input
            if (!m_pendingUserPromptText.isEmpty()) {
                m_appendUserPrompt->setPlainText(m_pendingUserPromptText);
            } else {
                m_appendUserPrompt->clear();
            }

            m_lastSavedUserPromptText.clear();

            // Send enabled only if m_appendUserPrompt non-empty
            bool hasText = !m_appendUserPrompt->toPlainText().trimmed().isEmpty();
            m_sendButton->setEnabled(hasText);
            m_saveButton->setEnabled(false);
        }
    } else if (selectedIndex == lastIndex - 1 && lastSlice.role == MessageRole::User) {
        // Penultimate slice selected and last slice is user-role
        // Show split view: penultimate slice in m_sliceViewer (read-only), last user slice in m_appendUserPrompt (editable)
        m_sliceViewer->show();
        m_sliceViewer->setEnabled(true);
        m_sliceViewer->setPlainText(selectedSlice.content);

        m_appendUserPrompt->show();
        m_appendUserPrompt->setEnabled(true);

        const PromptSlice &lastUserSlice = slices[lastIndex];

        // Restore pending text if any, else load from last user slice content
        if (!m_pendingUserPromptText.isEmpty()) {
            m_appendUserPrompt->setPlainText(m_pendingUserPromptText);
        } else {
            m_appendUserPrompt->setPlainText(lastUserSlice.content);
        }

        m_lastSavedUserPromptText = lastUserSlice.content;

        // Send enabled only if m_appendUserPrompt non-empty (must be true by design)
        bool hasText = !m_appendUserPrompt->toPlainText().trimmed().isEmpty();
        m_sendButton->setEnabled(hasText);

        // Save enabled only if m_appendUserPrompt differs from last saved (initially false)
        m_saveButton->setEnabled(false);

    } else {
        // Any other slice selected: show m_sliceViewer read-only, hide m_appendUserPrompt
        m_sliceViewer->show();
        m_sliceViewer->setEnabled(true);
        m_sliceViewer->setPlainText(selectedSlice.content);

        // Save current user input before hiding
        if (m_appendUserPrompt->isVisible() && m_appendUserPrompt->isEnabled()) {
            m_pendingUserPromptText = m_appendUserPrompt->toPlainText();
        }

        m_appendUserPrompt->hide();
        m_appendUserPrompt->setEnabled(false);

        // Disable Send and Save buttons
        m_sendButton->setEnabled(false);
        m_saveButton->setEnabled(false);
    }

    // Show Edit button only if selected slice is a user-role slice and NOT the last slice
    bool showEditButton = false;
    if (selectedSlice.role == MessageRole::User && selectedIndex != lastIndex) {
        showEditButton = true;
    }
    m_editToolButton->setVisible(showEditButton);
    m_editSpacer->setVisible(!showEditButton);

    m_updatingEditor = false;
}

void SessionTabWidget::markUnsavedChanges(bool changed)
{
    m_unsavedChanges = changed;
    m_saveButton->setEnabled(changed);

    if (m_refreshButton) {
        if (changed) {
            // Use a bright system color, e.g., yellow background
            m_refreshButton->setStyleSheet("background-color: palette(highlight); color: palette(highlighted-text);");
        } else {
            // Reset to default style
            m_refreshButton->setStyleSheet("");
        }
    }
}

bool SessionTabWidget::confirmDiscardUnsavedChanges()
{
    if (!m_unsavedChanges)
        return true;

    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        "Unsaved Changes",
        "You have unsaved changes and/or deletions. Discard them?",
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        // User discards changes, reset refresh button style
        if (m_refreshButton) {
            m_refreshButton->setStyleSheet("");
        }
        m_unsavedChanges = false;
        m_saveButton->setEnabled(false);
    }

    return (reply == QMessageBox::Yes);
}

void SessionTabWidget::onSaveClicked()
{
    saveHeaderMetadataFromUi();

    QString currentText = m_appendUserPrompt->toPlainText().trimmed();

    auto &slices = m_session.slices();
    int lastIndex = slices.size() - 1;

    // Handle last slice user prompt update or append if last slice is assistant
    if (lastIndex >= 0 && slices[lastIndex].role == MessageRole::User) {
        // Update last user slice content and timestamp if changed
        if (slices[lastIndex].content != currentText) {
            slices[lastIndex].content = currentText;
            slices[lastIndex].timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
        }
    } else if (lastIndex >= 0 && slices[lastIndex].role == MessageRole::Assistant) {
        if (!currentText.isEmpty()) {
            m_session.appendUserSlice(currentText);
        }
    } else {
        if (!currentText.isEmpty()) {
            m_session.appendUserSlice(currentText);
        }
    }

    // Save the entire session slices, including any deletions already applied
    if (!m_session.save(m_sessionFilePath)) {
        QMessageBox::warning(this, "Save Failed", "Failed to save session file.");
        return;
    }

    // Clear cached prompt text and reset unsaved changes
    m_pendingUserPromptText.clear();
    m_lastSavedUserPromptText = currentText;
    markUnsavedChanges(false);

    // Rebuild the prompt slice tree and select the last slice
    buildPromptSliceTree();
    int newLastIndex = m_session.slices().size() - 1;
    if (newLastIndex >= 0) {
        auto lastItem = m_promptSliceTree->topLevelItem(newLastIndex);
        if (lastItem) {
            m_promptSliceTree->setCurrentItem(lastItem);
            updateUiForSelectedSlice(newLastIndex);
        }
    }

    if (m_statusBar) {
        m_statusBar->showMessage("Session saved successfully.", 3000);
    }
}

void SessionTabWidget::onSendClicked()
{
    QString newPrompt = m_appendUserPrompt->toPlainText().trimmed();
    if (newPrompt.isEmpty()) {
        qDebug() << "[onSendClicked] Empty prompt, ignoring send.";
        return;
    }

    auto &slices = m_session.slices();
    int lastIndex = slices.size() - 1;

    if (lastIndex >= 0 && slices[lastIndex].role == MessageRole::User) {
        // Update last user slice content and timestamp
        slices[lastIndex].content = newPrompt;
        slices[lastIndex].timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    } else {
        // Append new user slice
        m_session.appendUserSlice(newPrompt);
    }

    // Save session after updating or appending user slice
    if (!m_session.save(m_sessionFilePath)) {
        QMessageBox::warning(this, "Error", "Failed to save session after adding prompt.");
        return;
    }

    // Update last saved prompt text and reset unsaved changes tracking
    m_lastSavedUserPromptText = newPrompt;
    markUnsavedChanges(false);

    // Run command pipes and refresh cache
    if (!m_session.runCommandPipes()) {
        QMessageBox::warning(this, "Error", "Failed to run command pipes in session.");
        return;
    }
    if (!m_session.refreshCacheAndSave()) {
        QMessageBox::warning(this, "Error", "Failed to cache includes in session after running command pipes.");
        return;
    }

    // Clear input after saving
    m_appendUserPrompt->clear();

    buildPromptSliceTree();

    // Append empty assistant slice for streaming response
    m_session.appendAssistantSlice(QString());

    if (!m_session.save(m_sessionFilePath)) {
        QMessageBox::warning(this, "Error", "Failed to save session after adding assistant slice.");
        return;
    }

    if (!m_session.runCommandPipes()) {
        QMessageBox::warning(this, "Error", "Failed to run command pipes in session.");
        return;
    }

    if (!m_session.refreshCacheAndSave()) {
        QMessageBox::warning(this, "Error", "Failed to cache includes in session after running command pipes.");
        return;
    }

    // Update UI with assistant slice
    int newLastIndex = m_session.slices().size() - 1;
    if (newLastIndex >= 0) {
        auto slice = m_session.slices().at(newLastIndex);
        auto item = new QTreeWidgetItem(m_promptSliceTree);
        item->setData(0, Qt::UserRole, newLastIndex);
        item->setText(0, slice.timestamp);
        switch (slice.role) {
        case MessageRole::User: item->setText(1, "User"); break;
        case MessageRole::Assistant: item->setText(1, "Assistant"); break;
        case MessageRole::System: item->setText(1, "System"); break;
        }
        item->setText(2, promptSliceSummary(slice));
        m_promptSliceTree->setCurrentItem(item);
        m_sliceViewer->setEnabled(true);
        m_partialResponseBuffer.clear();
    }

    // Prepare and send messages to AI backend
    QVector<PromptSlice> slicesExpanded = m_session.expandedSlices();
    QList<AIBackend::Message> messages;
    for (const PromptSlice &slice : slicesExpanded) {
        AIBackend::Message::Role role;
        switch (slice.role) {
        case MessageRole::System: role = AIBackend::Message::System; break;
        case MessageRole::User: role = AIBackend::Message::User; break;
        case MessageRole::Assistant: role = AIBackend::Message::Assistant; break;
        default: role = AIBackend::Message::Unknown; break;
        }
        messages.append({role, slice.content});
    }

    QVariantMap params;
    m_currentRequestId = QStringLiteral("session_%1_%2")
                             .arg(QDateTime::currentMSecsSinceEpoch())
                             .arg(QRandomGenerator::global()->bounded(INT_MAX));

    m_aiBackend->startRequest(messages, params, m_currentRequestId);

    m_sendButton->setEnabled(false);
    m_saveButton->setEnabled(false);
    m_unsavedChanges = false;
}

void SessionTabWidget::onPartialResponse(const QString &requestId, const QString &partialText)
{
    //qDebug() << "[onPartialResponse] Received chunk for requestId:" << requestId << "Chunk length:" << partialText.length();

    if (requestId != m_currentRequestId) {
        qDebug() << "[onPartialResponse] Ignoring chunk for unknown requestId.";
        return;
    }

    m_partialResponseBuffer += partialText;

    m_sliceViewer->blockSignals(true);
    m_updatingEditor = true;
    m_sliceViewer->setPlainText(m_partialResponseBuffer);
    m_updatingEditor = false;
    m_sliceViewer->blockSignals(false);

    QVector<PromptSlice> &slices = m_session.slices();
    int selectedIndex = m_promptSliceTree->indexOfTopLevelItem(m_promptSliceTree->currentItem());
    if (selectedIndex >= 0 && selectedIndex < slices.size()) {
        PromptSlice &slice = slices[selectedIndex];
        if (slice.role == MessageRole::Assistant) {
            slice.content = m_partialResponseBuffer;
            //qDebug() << "[onPartialResponse] Updated assistant slice content.";
        }
    }

    QTextCursor cursor = m_sliceViewer->textCursor();
    cursor.movePosition(QTextCursor::End);
    m_sliceViewer->setTextCursor(cursor);
}

void SessionTabWidget::onFinished(const QString &requestId, const QString &fullResponse)
{
    qDebug() << "[onFinished] Received full response for requestId:" << requestId << "Response length:" << fullResponse.length();

    if (requestId != m_currentRequestId) {
        qDebug() << "[onFinished] Ignoring finished signal for unknown requestId.";
        return;
    }

    QVector<PromptSlice> &slices = m_session.slices();
    int selectedIndex = m_promptSliceTree->indexOfTopLevelItem(m_promptSliceTree->currentItem());
    if (selectedIndex >= 0 && selectedIndex < slices.size()) {
        PromptSlice &slice = slices[selectedIndex];
        if (slice.role == MessageRole::Assistant) {
            slice.content = fullResponse;
            qDebug() << "[onFinished] Updated assistant slice content with full response.";
        }
    }

    buildPromptSliceTree();

    int lastIndex = m_promptSliceTree->topLevelItemCount() - 1;
    if (lastIndex >= 0) {
        m_promptSliceTree->setCurrentItem(m_promptSliceTree->topLevelItem(lastIndex));
    }

    m_sliceViewer->blockSignals(true);
    m_updatingEditor = true;
    m_sliceViewer->setPlainText(fullResponse);
    m_updatingEditor = false;
    m_sliceViewer->blockSignals(false);

    if (!saveSession()) {
        QMessageBox::warning(this, "Error", "Failed to save session after assistant response.");
        qWarning() << "[onFinished] Failed to save session after assistant response.";
    } else {
        qDebug() << "[onFinished] Session saved successfully after assistant response.";
    }

    m_sendButton->setEnabled(true);
    m_saveButton->setEnabled(false);
    m_currentRequestId.clear();
    m_partialResponseBuffer.clear();
    m_unsavedChanges = false;
}

void SessionTabWidget::onEditTitleDescClicked()
{
    if (!m_sessionFilePath.isEmpty()) {
        DescriptionGenerator* gen = new DescriptionGenerator(&m_session, m_aiBackend, this);

        // Show dialog UI for interactive editing
        gen->showDialog();

        // Update UI after dialog closes
        QString title = m_session.headerTitle();
        if (title.isEmpty())
            title = "(Untitled Session)";
        if (m_sessionTitleLabel)
            m_sessionTitleLabel->setText(title);

        QString description = m_session.headerDescription();
        if (description.isEmpty())
            description = "(No description)";
        if (m_editTitleDescBtn)
            m_editTitleDescBtn->setToolTip(description);
    }
}

void SessionTabWidget::onErrorOccurred(const QString &requestId, const QString &errorString)
{
    qWarning() << "[onErrorOccurred] Error for requestId:" << requestId << "Error:" << errorString;

    if (requestId != m_currentRequestId) {
        qDebug() << "[onErrorOccurred] Ignoring error for unknown requestId.";
        return;
    }

    QMessageBox::critical(this, "AI Backend Error", errorString);

    m_sendButton->setEnabled(true);
    m_saveButton->setEnabled(false);
    m_currentRequestId.clear();
    m_partialResponseBuffer.clear();
    m_unsavedChanges = false;
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

    m_unsavedChanges = false;
    m_saveButton->setEnabled(false);
    return true;
}

bool SessionTabWidget::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == m_appendUserPrompt && event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
        if ((keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) &&
            (keyEvent->modifiers() & Qt::ShiftModifier)) {
            if (m_sendButton->isEnabled()) {
                onSendClicked();
                return true; // event handled
            }
        }
    }
    if (obj == m_editTitleDescBtn) {
        if (event->type() == QEvent::Enter) {
            if (m_editTitleDescBtn) {
                QPoint globalPos = m_editTitleDescBtn->mapToGlobal(m_editTitleDescBtn->rect().bottomLeft());
                QToolTip::showText(globalPos, m_editTitleDescBtn->toolTip(), m_editTitleDescBtn);
            }
        }
    }
    return QWidget::eventFilter(obj, event);
}

void SessionTabWidget::onDeleteAfterClicked()
{
    auto selectedItems = m_promptSliceTree->selectedItems();
    if (selectedItems.isEmpty())
        return;

    int selectedIndex = selectedItems.first()->data(0, Qt::UserRole).toInt();
    if (selectedIndex < 0 || selectedIndex >= m_session.slices().size())
        return;

    int totalSlices = m_session.slices().size();
    if (selectedIndex >= totalSlices - 1) {
        if (m_statusBar) {
            m_statusBar->showMessage("No slices after selected slice to delete.", 3000);
        }        return;
    }

    // Delete slices after selectedIndex
    QVector<PromptSlice> newSlices = m_session.slices().mid(0, selectedIndex + 1);
    m_session.slices() = newSlices;

    // Clear cached user prompt text to avoid stale input
    m_pendingUserPromptText.clear();

    // Update UI
    buildPromptSliceTree();

    // Select the new last slice (which is now selectedIndex)
    if (m_promptSliceTree->topLevelItemCount() > 0) {
        m_promptSliceTree->setCurrentItem(m_promptSliceTree->topLevelItem(selectedIndex));
    }

    // Mark unsaved changes to enable Save button and change Refresh button color
    markUnsavedChanges(true);

    if (m_statusBar) {
        m_statusBar->showMessage("Deleted slices after selected slice. Press Refresh to reload deleted slices from disk.", 3000);
    }
}

void SessionTabWidget::contextMenuEvent(QContextMenuEvent *event)
{
    // If you handle context menu via customContextMenuRequested signal,
    // you can leave this empty or ignore the event.
    event->ignore();
}

QString SessionTabWidget::sessionFilePath() const
{
    return m_sessionFilePath;
}

void SessionTabWidget::onForkClicked()
{
    QMessageBox::information(this, "Fork Session Here", "Lateral branching (forking) is not implemented yet.");
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

void SessionTabWidget::onRefreshClicked()
{
    if (!confirmDiscardUnsavedChanges())
        return;

    if (!m_session.load(m_sessionFilePath)) {
        QMessageBox::warning(this, "Refresh", "Failed to reload session file.");
        return;
    }


    // Clear editors and disable until a slice is selected
    m_updatingEditor = true;
    m_sliceViewer->clear();
    m_sliceViewer->setEnabled(false);
    m_appendUserPrompt->clear();
    m_appendUserPrompt->setEnabled(false);
    m_updatingEditor = false;

    m_partialResponseBuffer.clear();
    m_unsavedChanges = false;
    m_saveButton->setEnabled(false);
    m_sendButton->setEnabled(false);

    buildPromptSliceTree();

    if (m_statusBar) {
        m_statusBar->showMessage("Session refreshed from disk.", 3000);
    }
}
