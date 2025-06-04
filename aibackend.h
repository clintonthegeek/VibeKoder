#ifndef AIBACKEND_H
#define AIBACKEND_H

#pragma once

#include <QObject>
#include <QString>
#include <QList>
#include <QVariantMap>
#include <QMutex>

/**
 * @brief Abstract base class for AI backend implementations.
 *
 * Supports chat-style messages with roles, generic configuration,
 * multiple concurrent requests identified by requestId,
 * streaming partial responses, cancellation, and status updates.
 */
class AIBackend : public QObject
{
    Q_OBJECT
public:
    explicit AIBackend(QObject *parent = nullptr);
    ~AIBackend() override;

    // Message struct representing a single chat message
    struct Message {
        enum Role {
            System,
            User,
            Assistant,
            Unknown
        };

        Role role = Unknown;
        QString content;

        static QString roleToString(Role r);
        static Role stringToRole(const QString &str);
    };

    /**
     * @brief Starts a new request asynchronously.
     *
     * @param messages List of chat messages (system, user, assistant).
     * @param params Generic key-value parameters for this request (e.g., temperature, max_tokens).
     * @param requestId Optional ID to track multiple requests; if empty, backend can assign one internally.
     *
     * Emits signals with this requestId to identify responses.
     */
    virtual void startRequest(const QList<Message> &messages,
                              const QVariantMap &params = QVariantMap(),
                              const QString &requestId = QString()) = 0;

    /**
     * @brief Cancel a specific request by requestId.
     * If requestId is empty, cancel all ongoing requests.
     */
    virtual void cancelRequest(const QString &requestId = QString()) = 0;

    /**
     * @brief Returns whether this backend supports streaming partial responses.
     */
    virtual bool supportsStreaming() const = 0;

    /**
     * @brief Returns the backend name, e.g., "OpenAI", "Ollama"
     */
    virtual QString backendName() const = 0;

    /**
     * @brief Get current global config parameters (e.g., API key, default model).
     */
    QVariantMap config() const;

    /**
     * @brief Set global config parameters.
     */
    void setConfig(const QVariantMap &config);

signals:
    /**
     * @brief Emitted when a chunk of text is received for a request (streaming).
     * @param requestId Identifies which request this belongs to.
     * @param text Partial text chunk.
     */
    void partialResponse(const QString &requestId, const QString &text);

    /**
     * @brief Emitted when a full response is ready for a request.
     * @param requestId Identifies which request this belongs to.
     * @param fullResponse Full response text.
     */
    void finished(const QString &requestId, const QString &fullResponse);

    /**
     * @brief Emitted on error for a request.
     * @param requestId Identifies which request this belongs to.
     * @param errorString Error description.
     */
    void errorOccurred(const QString &requestId, const QString &errorString);

    /**
     * @brief Optional status change signal for a request.
     * @param requestId Identifies which request this belongs to.
     * @param status Status string (e.g., "started", "cancelled", "completed").
     */
    void statusChanged(const QString &requestId, const QString &status);

protected:
    QVariantMap m_config;
    mutable QMutex m_configMutex;
};

#endif // AIBACKEND_H
