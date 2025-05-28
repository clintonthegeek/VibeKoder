#include "openai_request.h"
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

OpenAIRequest::OpenAIRequest(QObject *parent)
    : QObject(parent),
    m_networkAccessManager(new QNetworkAccessManager(this)),
    m_maxTokens(1000),
    m_temperature(0.3),
    m_topP(1.0),
    m_frequencyPenalty(0.0),
    m_presencePenalty(0.0),
    m_status(RequestStatus::Idle)
{
}

OpenAIRequest::~OpenAIRequest()
{
    qDeleteAll(m_messages);
    m_messages.clear();
}

void OpenAIRequest::setAccessToken(const QString &token)
{
    m_accessToken = token;
}

QString OpenAIRequest::accessToken() const
{
    return m_accessToken;
}

void OpenAIRequest::setModel(const QString &model)
{
    m_model = model;
}

QString OpenAIRequest::model() const
{
    return m_model;
}

void OpenAIRequest::setPrompt(const QString &prompt)
{
    m_prompt = prompt;
}

QString OpenAIRequest::prompt() const
{
    return m_prompt;
}

void OpenAIRequest::setMaxTokens(int tokens)
{
    m_maxTokens = tokens;
}

int OpenAIRequest::maxTokens() const
{
    return m_maxTokens;
}

void OpenAIRequest::setTemperature(double temp)
{
    m_temperature = temp;
}

double OpenAIRequest::temperature() const
{
    return m_temperature;
}

void OpenAIRequest::setTopP(double topP)
{
    m_topP = topP;
}

double OpenAIRequest::topP() const
{
    return m_topP;
}

void OpenAIRequest::setFrequencyPenalty(double freqPenalty)
{
    m_frequencyPenalty = freqPenalty;
}

double OpenAIRequest::frequencyPenalty() const
{
    return m_frequencyPenalty;
}

void OpenAIRequest::setPresencePenalty(double presPenalty)
{
    m_presencePenalty = presPenalty;
}

double OpenAIRequest::presencePenalty() const
{
    return m_presencePenalty;
}

void OpenAIRequest::setMessages(const QList<OpenAIMessage *> &messages)
{
    qDeleteAll(m_messages);
    m_messages.clear();
    for (auto msg : messages) {
        // Create copy for internal storage
        m_messages.append(new OpenAIMessage(msg->content(), msg->role()));
    }
}

QList<OpenAIMessage*> OpenAIRequest::messages() const
{
    return m_messages;
}

OpenAIRequest::RequestStatus OpenAIRequest::status() const
{
    return m_status;
}

void OpenAIRequest::execute()
{
    if (m_status == RequestStatus::InProgress) {
        qWarning() << "Request already in progress";
        return;
    }

    sendChatRequest();
}

void OpenAIRequest::sendChatRequest()
{
    const QUrl endpoint("https://api.openai.com/v1/chat/completions");

    QNetworkRequest request(endpoint);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", ("Bearer " + m_accessToken).toUtf8());

    QJsonObject rootObj;
    rootObj["model"] = m_model;
    rootObj["max_tokens"] = m_maxTokens;
    rootObj["temperature"] = m_temperature;
    rootObj["top_p"] = m_topP;
    rootObj["frequency_penalty"] = m_frequencyPenalty;
    rootObj["presence_penalty"] = m_presencePenalty;

    QJsonArray messagesArray;
    if (!m_messages.isEmpty()) {
        for (const OpenAIMessage* msg : m_messages) {
            QJsonObject msgObj;
            msgObj["role"] = OpenAIMessage::roleToString(msg->role());
            msgObj["content"] = msg->content();
            messagesArray.append(msgObj);
        }
    } else {
        // fallback to m_prompt user role only (legacy)
        QJsonObject userMsg;
        userMsg["role"] = "user";
        userMsg["content"] = m_prompt;
        messagesArray.append(userMsg);
    }
    rootObj["messages"] = messagesArray;

    QJsonDocument doc(rootObj);
    QByteArray data = doc.toJson();

    QNetworkReply *reply = m_networkAccessManager->post(request, data);
    m_status = RequestStatus::InProgress;
    emit statusChanged();

    connect(reply, &QNetworkReply::finished, this, [=]() {
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray respBytes = reply->readAll();
            QJsonDocument respDoc = QJsonDocument::fromJson(respBytes);

            if (!respDoc.isObject()) {
                emit requestError("Invalid JSON response");
                m_status = RequestStatus::Error;
                emit statusChanged();
                reply->deleteLater();
                return;
            }
            QJsonObject respObj = respDoc.object();
            QJsonArray choices = respObj.value("choices").toArray();

            if (choices.isEmpty()) {
                emit requestError("No choices in response");
                m_status = RequestStatus::Error;
                emit statusChanged();
                reply->deleteLater();
                return;
            }

            QJsonObject firstChoice = choices.at(0).toObject();
            QJsonObject messageObj = firstChoice.value("message").toObject();

            QString content = messageObj.value("content").toString();

            emit requestFinished(content);

            m_status = RequestStatus::Success;
            emit statusChanged();
        } else {
            QString errStr = reply->errorString();
            QByteArray additional = reply->readAll();
            if (!additional.isEmpty())
                errStr += "\n" + QString(additional);
            emit requestError(errStr);

            m_status = RequestStatus::Error;
            emit statusChanged();
        }
        reply->deleteLater();
    });
}
