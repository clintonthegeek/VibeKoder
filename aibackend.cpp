#include "aibackend.h"

AIBackend::AIBackend(QObject *parent)
    : QObject(parent)
{
}

AIBackend::~AIBackend()
{
}

QString AIBackend::Message::roleToString(Role r)
{
    switch (r) {
    case System: return QStringLiteral("system");
    case User: return QStringLiteral("user");
    case Assistant: return QStringLiteral("assistant");
    default: return QStringLiteral("unknown");
    }
}

AIBackend::Message::Role AIBackend::Message::stringToRole(const QString &str)
{
    QString lower = str.toLower();
    if (lower == "system") return System;
    if (lower == "user") return User;
    if (lower == "assistant") return Assistant;
    return Unknown;
}

QVariantMap AIBackend::config() const
{
    QMutexLocker locker(&m_configMutex);
    return m_config;
}

void AIBackend::setConfig(const QVariantMap &config)
{
    QMutexLocker locker(&m_configMutex);
    m_config = config;
}
