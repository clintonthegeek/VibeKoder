#ifndef DESCRIPTIONGENERATOR_H
#define DESCRIPTIONGENERATOR_H

#include <QObject>
#include <QString>

class Session;
class AIBackend;

class DescriptionGenerator : public QObject
{
    Q_OBJECT
public:
    explicit DescriptionGenerator(Session* session, AIBackend* aiBackend, QObject* parent = nullptr);

    // Run generation invisibly, emits signals on completion or error
    void generateTitleAndDescription();

    // Show the dialog UI for interactive editing (optional)
    void showDialog();

signals:
    void generationFinished(const QString& title, const QString& description);
    void generationError(const QString& errorString);

private:
    void onGenerationFinished(const QString& requestId, const QString& fullResponse);
    void onGenerationError(const QString& requestId, const QString& errorString);

    QString stripFencedCodeBlock(const QString &text);

    Session* m_session = nullptr;
    AIBackend* m_aiBackend = nullptr;

    QString m_currentRequestId;

    // UI pointers for dialog mode
    class DialogUi;
    DialogUi* m_dialogUi = nullptr;
};

#endif // DESCRIPTIONGENERATOR_H
