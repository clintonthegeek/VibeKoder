#include "openaibackend.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonParseError>
#include <QDateTime>
#include <QDir>
#include <QStandardPaths>
#include <QTemporaryFile>
#include <QDebug>
#include <QRandomGenerator>


OpenAIBackend::OpenAIBackend(QObject *parent)
    : AIBackend(parent)
{
}

OpenAIBackend::~OpenAIBackend()
{
    // Cancel and cleanup all active requests
    for (auto &reqData : m_activeRequests) {
        if (reqData->reply) {
            reqData->reply->abort();
            reqData->reply->deleteLater();
        }
        if (reqData->tempFile.isOpen()) {
            reqData->tempFile.close();
        }
    }
    m_activeRequests.clear();
}

QString OpenAIBackend::generateRequestId()
{
    // Generate a unique request ID using timestamp + random number
    return QStringLiteral("req_%1_%2")
        .arg(QDateTime::currentMSecsSinceEpoch())
        .arg(QRandomGenerator::global()->bounded(INT_MAX));
}

QVariant OpenAIBackend::getConfigValue(const QString &key, const QVariant &defaultValue) const
{
    QMutexLocker locker(&m_configMutex);
    return m_config.value(key, defaultValue);
}

QByteArray OpenAIBackend::buildRequestPayload(const QList<Message> &messages, const QVariantMap &params) const
{
    QJsonObject rootObj;

    // Model
    QString model = params.value("model").toString();
    if (model.isEmpty())
        model = getConfigValue("model", "gpt-4.1-mini").toString();
    rootObj["model"] = model;

    // Messages array
    QJsonArray messagesArray;
    for (const Message &msg : messages) {
        QJsonObject msgObj;
        msgObj["role"] = Message::roleToString(msg.role);
        msgObj["content"] = msg.content;
        messagesArray.append(msgObj);
    }
    rootObj["messages"] = messagesArray;

    // Parameters with defaults from config or params
    auto getDoubleParam = [&](const QString &key, double def) -> double {
        if (params.contains(key))
            return params.value(key).toDouble();
        return getConfigValue(key, def).toDouble();
    };

    auto getIntParam = [&](const QString &key, int def) -> int {
        if (params.contains(key))
            return params.value(key).toInt();
        return getConfigValue(key, def).toInt();
    };

    rootObj["max_tokens"] = getIntParam("max_tokens", 800);
    rootObj["temperature"] = getDoubleParam("temperature", 0.3);
    rootObj["top_p"] = getDoubleParam("top_p", 1.0);
    rootObj["frequency_penalty"] = getDoubleParam("frequency_penalty", 0.0);
    rootObj["presence_penalty"] = getDoubleParam("presence_penalty", 0.0);

    // Enable streaming
    rootObj["stream"] = true;

    // Optional parameters (stop sequences, user, logit_bias, etc.)
    if (params.contains("stop")) {
        rootObj["stop"] = QJsonValue::fromVariant(params.value("stop"));
    }
    if (params.contains("user")) {
        rootObj["user"] = params.value("user").toString();
    }
    if (params.contains("logit_bias")) {
        rootObj["logit_bias"] = QJsonValue::fromVariant(params.value("logit_bias"));
    }

    QJsonDocument doc(rootObj);
    return doc.toJson(QJsonDocument::Compact);
}

void OpenAIBackend::startRequest(const QList<Message> &messages,
                                 const QVariantMap &params,
                                 const QString &requestId)
{
    QString reqId = requestId.isEmpty() ? generateRequestId() : requestId;

    if (m_activeRequests.contains(reqId)) {
        emit errorOccurred(reqId, QStringLiteral("Request ID already in use"));
        return;
    }

    QNetworkRequest request(QUrl("https://api.openai.com/v1/chat/completions"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QString apiKey = getConfigValue("access_token").toString();
    if (apiKey.isEmpty()) {
        emit errorOccurred(reqId, QStringLiteral("API key is not set"));
        return;
    }
    request.setRawHeader("Authorization", ("Bearer " + apiKey).toUtf8());

    QByteArray payload = buildRequestPayload(messages, params);

    QNetworkReply *reply = m_networkManager.post(request, payload);

    // Setup RequestData
    RequestData* reqData = new RequestData();
    reqData->reply = reply;
    reqData->requestId = reqId;

    // Create a temporary file for streaming buffer
    QString tempDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    QString tempFileName = QString("vibekoder_openai_%1.txt").arg(reqId);
    QString tempFilePath = QDir(tempDir).filePath(tempFileName);

    reqData->tempFile.setFileName(tempFilePath);
    if (!reqData->tempFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        emit errorOccurred(reqId, QStringLiteral("Failed to open temporary file for streaming"));
        reply->abort();
        reply->deleteLater();
        return;
    }

    m_activeRequests.insert(reqId, reqData);

    // Connect signals
    connect(reply, &QNetworkReply::readyRead, this, [this, reqId]() {
        if (!m_activeRequests.contains(reqId))
            return;
        RequestData* rd = m_activeRequests[reqId];
        QByteArray chunk = rd->reply->readAll();
        processStreamData(*rd, chunk);
    });

    connect(reply, &QNetworkReply::finished, this, [this, reqId]() {
        if (!m_activeRequests.contains(reqId))
            return;
        RequestData* rd = m_activeRequests[reqId];
        onNetworkFinished();
    });

    connect(reply, QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::errorOccurred),
            this, [this, reqId](QNetworkReply::NetworkError code) {
                Q_UNUSED(code);
                if (!m_activeRequests.contains(reqId))
                    return;
                RequestData* rd = m_activeRequests.value(reqId, nullptr);
                if (!rd)
                    return; // or handle error
                QString err = rd->reply->errorString();
                emit errorOccurred(rd->requestId, err);
                rd->reply->deleteLater();
                if (rd->tempFile.isOpen())
                    rd->tempFile.close();
                m_activeRequests.remove(rd->requestId);
            });

    emit statusChanged(reqId, QStringLiteral("started"));
}

void OpenAIBackend::onNetworkReadyRead()
{
    // Not used - handled via lambdas connected to QNetworkReply signals
}

void OpenAIBackend::onNetworkError(QNetworkReply::NetworkError)
{
    // Not used - handled via lambdas connected to QNetworkReply signals
}

void OpenAIBackend::processStreamData(RequestData &reqData, const QByteArray &chunk)
{
    // OpenAI streaming sends data in SSE-like format:
    // data: {json}\n\n
    // The last message is "data: [DONE]\n\n"

    reqData.buffer.append(chunk);

    while (true) {
        int newlineIndex = reqData.buffer.indexOf("\n");
        if (newlineIndex == -1)
            break;

        QByteArray line = reqData.buffer.left(newlineIndex).trimmed();
        reqData.buffer.remove(0, newlineIndex + 1);

        if (line.isEmpty())
            continue;

        if (line == "data: [DONE]") {
            // Stream finished
            reqData.finished = true;

            if (reqData.tempFile.isOpen())
                reqData.tempFile.close();

            // Read full content from temp file
            QString fullResponse;
            QFile file(reqData.tempFile.fileName());
            if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                fullResponse = QString::fromUtf8(file.readAll());
                file.close();
            }

            finalizeRequest(reqData, fullResponse);
            return;
        }

        if (!line.startsWith("data: ")) {
            // Ignore non-data lines
            continue;
        }

        QByteArray jsonData = line.mid(6); // after "data: "

        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(jsonData, &parseError);
        if (parseError.error != QJsonParseError::NoError) {
            // Parsing error, emit error and abort
            emit errorOccurred(reqData.requestId, QString("JSON parse error in stream: %1").arg(parseError.errorString()));
            reqData.reply->abort();
            return;
        }

        if (!doc.isObject())
            continue;

        QJsonObject obj = doc.object();

        // Extract content delta from choices array
        QJsonArray choices = obj.value("choices").toArray();
        if (choices.isEmpty())
            continue;

        QJsonObject firstChoice = choices.at(0).toObject();
        QJsonObject delta = firstChoice.value("delta").toObject();

        QString contentPart = delta.value("content").toString();

        if (!contentPart.isEmpty()) {
            // Append to temp file
            if (reqData.tempFile.isOpen()) {
                QTextStream out(&reqData.tempFile);
                out << contentPart;
                out.flush();
            }

            // Emit partial response signal
            emit partialResponse(reqData.requestId, contentPart);
        }
    }
}

void OpenAIBackend::finalizeRequest(RequestData &reqData, const QString &fullResponse)
{
    emit finished(reqData.requestId, fullResponse);

    if (reqData.reply) {
        reqData.reply->deleteLater();
        reqData.reply = nullptr;
    }

    m_activeRequests.remove(reqData.requestId);

    emit statusChanged(reqData.requestId, QStringLiteral("completed"));
}

void OpenAIBackend::cancelRequest(const QString &requestId)
{
    if (requestId.isEmpty()) {
        // Cancel all
        for (auto it = m_activeRequests.begin(); it != m_activeRequests.end(); ++it) {
            RequestData* rd = it.value();
            if (!rd)
                continue;

            if (rd->reply) {
                rd->reply->abort();
                rd->reply->deleteLater();
                rd->reply = nullptr;
            }
            if (rd->tempFile.isOpen())
                rd->tempFile.close();

            delete rd;
        }
        m_activeRequests.clear();
        return;
    }

    if (!m_activeRequests.contains(requestId))
        return;

    RequestData* reqData = m_activeRequests[requestId];
    if (reqData->reply) {
        reqData->reply->abort();
        reqData->reply->deleteLater();
        reqData->reply = nullptr;
    }
    if (reqData->tempFile.isOpen())
        reqData->tempFile.close();

    m_activeRequests.remove(requestId);

    emit statusChanged(requestId, QStringLiteral("cancelled"));
}

void OpenAIBackend::onNetworkFinished()
{
    // This slot is connected per request via lambda, so no generic implementation here.
    // All finalization is handled in processStreamData when [DONE] is received.
    // This slot can be left empty or used for fallback if needed.
}
