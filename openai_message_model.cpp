#include "openai_message_model.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>

OpenAIMessageModel::OpenAIMessageModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

OpenAIMessageModel::~OpenAIMessageModel()
{
    clearMessages();
}

int OpenAIMessageModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return m_messages.count();
}

QVariant OpenAIMessageModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_messages.count())
        return QVariant();

    OpenAIMessage *message = m_messages.at(index.row());

    switch (role) {
    case ContentRole:
        return message->content();
    case RoleRole:
        return static_cast<int>(message->role());
    case Qt::DisplayRole:
        return message->content();
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> OpenAIMessageModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[ContentRole] = "content";
    roles[RoleRole] = "role";
    return roles;
}

QList<OpenAIMessage *> OpenAIMessageModel::messages() const
{
    return m_messages;
}

void OpenAIMessageModel::setMessages(const QList<OpenAIMessage *> &messages)
{
    beginResetModel();
    clearMessages();
    m_messages = messages;
    endResetModel();
    emit messagesChanged();
}

void OpenAIMessageModel::addMessage(OpenAIMessage *message)
{
    beginInsertRows(QModelIndex(), m_messages.count(), m_messages.count());
    m_messages.append(message);
    endInsertRows();
    emit messagesChanged();
}

void OpenAIMessageModel::addMessage(OpenAIMessage::Role role, const QString &content)
{
    OpenAIMessage *message = new OpenAIMessage(content, role, this);
    addMessage(message);
}

void OpenAIMessageModel::removeMessage(OpenAIMessage *message)
{
    int pos = m_messages.indexOf(message);
    if (pos != -1) {
        beginRemoveRows(QModelIndex(), pos, pos);
        m_messages.removeAt(pos);
        endRemoveRows();
        emit messagesChanged();
        message->deleteLater();
    }
}

void OpenAIMessageModel::clearMessages()
{
    beginResetModel();
    for (OpenAIMessage *m : m_messages) {
        m->deleteLater();
    }
    m_messages.clear();
    endResetModel();
    emit messagesChanged();
}

QList<OpenAIMessage*> OpenAIMessageModel::messagesFromJson(const QString &jsonString) const
{
    QList<OpenAIMessage *> messages;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonString.toUtf8());
    if (jsonDoc.isNull()) {
        qWarning() << "Invalid JSON document for messages";
        return messages;
    }

    QJsonObject rootObj = jsonDoc.object();
    if (!rootObj.contains("messages") || !rootObj["messages"].isArray()) {
        qWarning() << "JSON object missing messages array";
        return messages;
    }

    QJsonArray arr = rootObj["messages"].toArray();
    for (const auto &val : arr) {
        if (!val.isObject())
            continue;
        QJsonObject obj = val.toObject();
        if (!(obj.contains("role") && obj.contains("content")))
            continue;

        QString roleStr = obj["role"].toString();
        QString contentStr = obj["content"].toString();

        OpenAIMessage::Role role = OpenAIMessage::roleFromString(roleStr);
        OpenAIMessage *msg = new OpenAIMessage(contentStr, role);
        messages.append(msg);
    }

    return messages;
}

QString OpenAIMessageModel::jsonFromMessages(const QList<OpenAIMessage *> &messages) const
{
    QJsonArray arr;
    for (const auto &msg : messages) {
        QJsonObject obj;
        obj["role"] = OpenAIMessage::roleToString(msg->role());
        obj["content"] = msg->content();
        arr.append(obj);
    }
    QJsonObject root;
    root["messages"] = arr;

    QJsonDocument doc(root);
    return QString::fromUtf8(doc.toJson(QJsonDocument::Indented));
}
