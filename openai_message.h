#pragma once

#include <QObject>
#include <QString>

class OpenAIMessage : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString content READ content WRITE setContent NOTIFY contentChanged)
    Q_PROPERTY(Role role READ role WRITE setRole NOTIFY roleChanged)

public:
    enum Role {
        System,
        User,
        Assistant
    };
    Q_ENUM(Role)

    explicit OpenAIMessage(QObject *parent = nullptr);
    OpenAIMessage(const QString& content, Role role, QObject *parent = nullptr);
    ~OpenAIMessage();

    QString content() const;
    void setContent(const QString& content);

    Role role() const;
    void setRole(Role role);

    static QString roleToString(Role role);
    static Role roleFromString(const QString &roleString);

signals:
    void contentChanged();
    void roleChanged();

private:
    QString m_content;
    Role m_role;
};
