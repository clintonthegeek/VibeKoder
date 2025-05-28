#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QList>

#include "openai_message.h"

class OpenAIRequest : public QObject
{
    Q_OBJECT

public:
    explicit OpenAIRequest(QObject *parent = nullptr);
    ~OpenAIRequest() override;

    enum class RequestStatus {
        Idle,
        InProgress,
        Error,
        Success
    };

    // Setters/Getters
    void setAccessToken(const QString &token);
    QString accessToken() const;

    void setModel(const QString &model);
    QString model() const;

    void setPrompt(const QString &prompt);
    QString prompt() const;

    void setMaxTokens(int tokens);
    int maxTokens() const;

    void setTemperature(double temp);
    double temperature() const;

    void setTopP(double topP);
    double topP() const;

    void setFrequencyPenalty(double freqPenalty);
    double frequencyPenalty() const;

    void setPresencePenalty(double presPenalty);
    double presencePenalty() const;

    void setMessages(const QList<OpenAIMessage*> &messages);
    QList<OpenAIMessage*> messages() const;

    void execute();

    RequestStatus status() const;

signals:
    void requestFinished(const QString &generatedText);
    void requestError(const QString &errorString);
    void statusChanged();



private:
    void sendChatRequest();

    QNetworkAccessManager *m_networkAccessManager;

    QString m_accessToken;
    QString m_model;
    QString m_prompt;

    int m_maxTokens;
    double m_temperature;
    double m_topP;
    double m_frequencyPenalty;
    double m_presencePenalty;

    QList<OpenAIMessage*> m_messages;

    RequestStatus m_status;
};
