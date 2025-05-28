#pragma once

#include <QObject>
#include <QAbstractListModel>
#include <QList>
#include <QString>

#include "openai_message.h"

class OpenAIMessageModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles {
        ContentRole = Qt::UserRole + 1,
        RoleRole
    };

    explicit OpenAIMessageModel(QObject *parent = nullptr);
    ~OpenAIMessageModel();

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    QList<OpenAIMessage *> messages() const;
    void setMessages(const QList<OpenAIMessage *> &messages);

    void addMessage(OpenAIMessage::Role role, const QString &content);
    void addMessage(OpenAIMessage* message);
    void removeMessage(OpenAIMessage* message);
    void clearMessages();

signals:
    void messagesChanged();

private:
    QList<OpenAIMessage*> m_messages;

    QList<OpenAIMessage*> messagesFromJson(const QString &jsonString) const;
    QString jsonFromMessages(const QList<OpenAIMessage*> &messages) const;
};
