#ifndef COMMANDPIPEMANAGER_H
#define COMMANDPIPEMANAGER_H

#pragma once

#include <QObject>
#include <QString>
#include <QStringList>

class Project;

class CommandPipeManager : public QObject
{
    Q_OBJECT
public:
    explicit CommandPipeManager(Project *project, const QString &sessionCacheFolder, QObject *parent = nullptr);

    // Runs the named command pipe synchronously.
    // Returns empty string on success, or error message on failure.
    QString runCommandPipe(const QString &name);

private:
    QString runSrcAmalgamate();

    Project *m_project;
    QString m_sessionCacheFolder;

    // Helper to recursively scan source folder, excluding "build" folder
    QStringList scanSourceFiles() const;

    // Helper to write amalgamated source to src/src.txt in session cache
    bool writeAmalgamatedSource(const QStringList &filePaths, QString &errorOut) const;
};
#endif // COMMANDPIPEMANAGER_H
