#ifndef OPENAIBACKEND_H
#define OPENAIBACKEND_H

#pragma once

#include "aibackend.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QFile>
#include <QHash>
#include <QTimer>

/**
 * @brief Concrete AIBackend implementation for OpenAI API.
 *
 * Supports streaming chat completions with incremental partial responses.
 */
class OpenAIBackend : public AIBackend
{
    Q_OBJECT
public:
    explicit OpenAIBackend(QObject *parent = nullptr);
    ~OpenAIBackend() override;

    void startRequest(const QList<Message> &messages,
                      const QVariantMap &params = QVariantMap(),
                      const QString &requestId = QString()) override;

    void cancelRequest(const QString &requestId = QString()) override;

    bool supportsStreaming() const override { return true; }
    QString backendName() const override { return QStringLiteral("OpenAI"); }

private slots:
    void onNetworkReadyRead();
    void onNetworkFinished();
    void onNetworkError(QNetworkReply::NetworkError code);

private:
    struct RequestData {
        QNetworkReply *reply = nullptr;
        QByteArray buffer; // Buffer for partial data parsing
        QString requestId;
        QFile tempFile;
        bool finished = false;
    };

    QNetworkAccessManager m_networkManager;

    // Map requestId -> RequestData
    QHash<QString, RequestData*> m_activeRequests;

    // Helper to generate unique request IDs if none provided
    QString generateRequestId();

    // Helper to parse streaming chunks from OpenAI chunked response
    void processStreamData(RequestData &reqData, const QByteArray &chunk);

    // Helper to finalize request: close temp file, emit finished signal, atomic save
    void finalizeRequest(RequestData &reqData, const QString &fullResponse);

    // Helper to build JSON payload for chat completion request
    QByteArray buildRequestPayload(const QList<Message> &messages, const QVariantMap &params) const;

    // Helper to get config parameters with fallback
    QVariant getConfigValue(const QString &key, const QVariant &defaultValue = QVariant()) const;

    // Mutex for thread safety if needed (network callbacks are in main thread)
};

#endif // OPENAIBACKEND_H
