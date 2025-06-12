#include "descriptiongenerator.h"
#include "session.h"
#include "aibackend.h"

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLineEdit>
#include <QTextEdit>
#include <QPushButton>
#include <QLabel>
#include <QMessageBox>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QJsonObject>
#include <QRegularExpression>
#include <QDebug>

class DescriptionGenerator::DialogUi : public QDialog
{
public:
    DialogUi(DescriptionGenerator* parentGen, QWidget* parent = nullptr)
        : QDialog(parent), m_parentGen(parentGen)
    {
        setWindowTitle("Edit Title and Description");
        resize(600, 400);

        auto mainLayout = new QVBoxLayout(this);

        auto formLayout = new QFormLayout();
        mainLayout->addLayout(formLayout);

        titleEdit = new QLineEdit(this);
        formLayout->addRow("Title:", titleEdit);

        descriptionEdit = new QTextEdit(this);
        descriptionEdit->setFixedHeight(120);
        formLayout->addRow("Description:", descriptionEdit);

        statusLabel = new QLabel("Idle", this);
        mainLayout->addWidget(statusLabel);

        auto buttonLayout = new QHBoxLayout();
        mainLayout->addLayout(buttonLayout);

        generateButton = new QPushButton("Generate Title and Description", this);
        resetButton = new QPushButton("Reset", this);
        okButton = new QPushButton("OK", this);
        cancelButton = new QPushButton("Cancel", this);

        buttonLayout->addWidget(generateButton);
        buttonLayout->addWidget(resetButton);
        buttonLayout->addStretch();
        buttonLayout->addWidget(okButton);
        buttonLayout->addWidget(cancelButton);

        connect(generateButton, &QPushButton::clicked, m_parentGen, &DescriptionGenerator::generateTitleAndDescription);
        connect(resetButton, &QPushButton::clicked, this, [this]() {
            if (m_parentGen && m_parentGen->m_session) {
                titleEdit->setText(m_parentGen->m_session->headerTitle());
                descriptionEdit->setPlainText(m_parentGen->m_session->headerDescription());
                statusLabel->setText("Reset to original values");
            }
        });
        connect(okButton, &QPushButton::clicked, this, &DialogUi::accept);
        connect(cancelButton, &QPushButton::clicked, this, &DialogUi::reject);
    }

    QLineEdit* titleEdit = nullptr;
    QTextEdit* descriptionEdit = nullptr;
    QPushButton* generateButton = nullptr;
    QPushButton* resetButton = nullptr;
    QPushButton* okButton = nullptr;
    QPushButton* cancelButton = nullptr;
    QLabel* statusLabel = nullptr;

private:
    DescriptionGenerator* m_parentGen = nullptr;
};

DescriptionGenerator::DescriptionGenerator(Session* session, AIBackend* aiBackend, QObject* parent)
    : QObject(parent), m_session(session), m_aiBackend(aiBackend)
{
    Q_ASSERT(m_session);
    Q_ASSERT(m_aiBackend);

    connect(m_aiBackend, &AIBackend::finished, this, &DescriptionGenerator::onGenerationFinished);
    connect(m_aiBackend, &AIBackend::errorOccurred, this, &DescriptionGenerator::onGenerationError);
}

void DescriptionGenerator::showDialog()
{
    if (!m_dialogUi) {
        m_dialogUi = new DialogUi(this);
        m_dialogUi->titleEdit->setText(m_session->headerTitle());
        m_dialogUi->descriptionEdit->setPlainText(m_session->headerDescription());
        m_dialogUi->statusLabel->setText("Idle");
    }
    m_dialogUi->exec();

    if (m_dialogUi->result() == QDialog::Accepted) {
        m_session->setHeaderTitle(m_dialogUi->titleEdit->text().trimmed());
        m_session->setHeaderDescription(m_dialogUi->descriptionEdit->toPlainText().trimmed());
        m_session->save();
    }
}

void DescriptionGenerator::generateTitleAndDescription()
{
    if (!m_session || !m_aiBackend) {
        emit generationError("Session or AI backend not set.");
        return;
    }

    if (m_dialogUi) {
        m_dialogUi->generateButton->setEnabled(false);
        m_dialogUi->statusLabel->setText("Generating...");
    }

    QString prompt = m_session->compiledRawMarkdown();
    QString systemPrompt = QStringLiteral(
                               "You are a helpful assistant. Given the following session content, "
                               "please provide a JSON object with exactly two fields: "
                               "\"title\" (a concise title) and \"description\" (a brief synopsis of a few lines). "
                               "Respond ONLY with the JSON object, no explanations, no extra text.\n\n"
                               "Session content:\n%1"
                               ).arg(prompt);

    QList<AIBackend::Message> messages;
    messages.append({AIBackend::Message::System, systemPrompt});

    // this should be specified in app settings (or maybe even project settings)
    QVariantMap params;
    params["model"] = "gpt-4.1-nano";

    m_currentRequestId = QStringLiteral("generateTitleDesc_%1").arg(QDateTime::currentMSecsSinceEpoch());

    m_aiBackend->startRequest(messages, params, m_currentRequestId);
}

void DescriptionGenerator::onGenerationFinished(const QString& requestId, const QString& fullResponse)
{
    if (requestId != m_currentRequestId)
        return;

    QString trimmedResponse = stripFencedCodeBlock(fullResponse);

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(trimmedResponse.toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        QString err = QString("Failed to parse JSON: %1").arg(parseError.errorString());
        if (m_dialogUi) {
            m_dialogUi->statusLabel->setText(err);
            m_dialogUi->generateButton->setEnabled(true);
        }
        emit generationError(err);
        return;
    }

    QJsonObject obj = doc.object();
    QString title = obj.value("title").toString();
    QString description = obj.value("description").toString();

    if (m_dialogUi) {
        m_dialogUi->titleEdit->setText(title);
        m_dialogUi->descriptionEdit->setPlainText(description);
        m_dialogUi->statusLabel->setText("Generation complete");
        m_dialogUi->generateButton->setEnabled(true);
    }

    // Update session metadata and save
    m_session->setHeaderTitle(title);
    m_session->setHeaderDescription(description);
    m_session->save();

    emit generationFinished(title, description);
}

void DescriptionGenerator::onGenerationError(const QString& requestId, const QString& errorString)
{
    if (requestId != m_currentRequestId)
        return;

    if (m_dialogUi) {
        m_dialogUi->statusLabel->setText("AI Backend error: " + errorString);
        m_dialogUi->generateButton->setEnabled(true);
    }

    emit generationError(errorString);
}

QString DescriptionGenerator::stripFencedCodeBlock(const QString &text)
{
    QString trimmed = text.trimmed();
    QRegularExpression fenceRe(R"(^```json\s*\n(.*)\n```$)", QRegularExpression::DotMatchesEverythingOption);
    QRegularExpressionMatch match = fenceRe.match(trimmed);
    if (match.hasMatch()) {
        return match.captured(1).trimmed();
    }
    if (trimmed.startsWith("```") && trimmed.endsWith("```")) {
        return trimmed.mid(3, trimmed.length() - 6).trimmed();
    }
    return trimmed;
}
