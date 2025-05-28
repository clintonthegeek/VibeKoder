#include "openai_message.h"

OpenAIMessage::OpenAIMessage(QObject *parent)
    : QObject(parent), m_role(System)
{
}

OpenAIMessage::OpenAIMessage(const QString& content, Role role, QObject *parent)
    : QObject(parent), m_content(content), m_role(role)
{
}

OpenAIMessage::~OpenAIMessage()
{
}

QString OpenAIMessage::content() const
{
    return m_content;
}

void OpenAIMessage::setContent(const QString& content)
{
    if (m_content != content) {
        m_content = content;
        emit contentChanged();
    }
}

OpenAIMessage::Role OpenAIMessage::role() const
{
    return m_role;
}

void OpenAIMessage::setRole(OpenAIMessage::Role role)
{
    if (m_role != role) {
        m_role = role;
        emit roleChanged();
    }
}

QString OpenAIMessage::roleToString(OpenAIMessage::Role role)
{
    switch (role) {
    case System: return QStringLiteral("system");
    case User: return QStringLiteral("user");
    case Assistant: return QStringLiteral("assistant");
    default: return QStringLiteral("unknown");
    }
}

OpenAIMessage::Role OpenAIMessage::roleFromString(const QString &roleString)
{
    QString lower = roleString.toLower();
    if (lower == "system") {
        return System;
    } else if (lower == "user") {
        return User;
    } else if (lower == "assistant") {
        return Assistant;
    }
    return System; // default fallback
}
