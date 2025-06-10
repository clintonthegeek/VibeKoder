#include "commandpipemanager.h"
#include "project.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QDebug>
#include <functional>

CommandPipeManager::CommandPipeManager(Project *project, const QString &sessionCacheFolder, QObject *parent)
    : QObject(parent)
    , m_project(project)
    , m_sessionCacheFolder(sessionCacheFolder)
{
    qDebug() << "[CommandPipeManager] Initialized with session cache folder:" << m_sessionCacheFolder;
}

QString CommandPipeManager::runCommandPipe(const QString &name)
{
    qDebug() << "[CommandPipeManager] runCommandPipe called with name:" << name;

    if (name == "amalgamateSrc") {
        QString result = runSrcAmalgamate();
        if (result.isEmpty()) {
            qDebug() << "[CommandPipeManager] runSrcAmalgamate succeeded";
        } else {
            qWarning() << "[CommandPipeManager] runSrcAmalgamate failed with error:" << result;
        }
        return result;
    }

    QString unknownCmd = QString("Unknown command pipe: %1").arg(name);
    qWarning() << "[CommandPipeManager]" << unknownCmd;
    return unknownCmd;
}

QString CommandPipeManager::runSrcAmalgamate()
{
    if (!m_project) {
        QString err = "No project set in CommandPipeManager";
        qWarning() << "[runSrcAmalgamate]" << err;
        return err;
    }

    QString srcFolder = m_project->srcFolder();
    if (!QDir(srcFolder).isAbsolute()) {
        srcFolder = QDir(m_project->rootFolder()).filePath(srcFolder);
    }    if (srcFolder.isEmpty()) {
        QString err = "Project source folder is empty";
        qWarning() << "[runSrcAmalgamate]" << err;
        return err;
    }

    QDir srcDir(srcFolder);
    if (!srcDir.exists()) {
        QString err = QString("Source folder does not exist: %1").arg(srcFolder);
        qWarning() << "[runSrcAmalgamate]" << err;
        return err;
    }

    qDebug() << "[runSrcAmalgamate] Scanning source files in:" << srcFolder;

    QStringList sourceFiles = scanSourceFiles();

    if (sourceFiles.isEmpty()) {
        QString err = "No source files found in source folder";
        qWarning() << "[runSrcAmalgamate]" << err;
        return err;
    }

    qDebug() << "[runSrcAmalgamate] Found" << sourceFiles.size() << "source files.";

    QString errorStr;
    if (!writeAmalgamatedSource(sourceFiles, errorStr)) {
        qWarning() << "[runSrcAmalgamate] Failed to write amalgamated source:" << errorStr;
        return errorStr;
    }

    qDebug() << "[runSrcAmalgamate] Amalgamated source written successfully.";

    return QString(); // success
}

QStringList CommandPipeManager::scanSourceFiles() const
{
    QStringList results;

    if (!m_project) {
        qWarning() << "[scanSourceFiles] No project set";
        return results;
    }

    QString srcFolder = m_project->srcFolder();
    if (!QDir(srcFolder).isAbsolute()) {
        srcFolder = QDir(m_project->rootFolder()).filePath(srcFolder);
    }    QStringList sourceFileTypes = m_project->getValue("filetypes.source").toStringList();

    if (srcFolder.isEmpty()) {
        qWarning() << "[scanSourceFiles] Source folder is empty";
        return results;
    }

    QDir dir(srcFolder);
    if (!dir.exists()) {
        qWarning() << "[scanSourceFiles] Source folder does not exist:" << srcFolder;
        return results;
    }

    // We want to include CMakeLists.txt and .ui files as well, so add them explicitly
    QStringList extraPatterns = { "CMakeLists.txt", "*.ui" };

    // Combine sourceFileTypes and extraPatterns
    QStringList allPatterns = sourceFileTypes;
    for (const QString &pat : extraPatterns) {
        if (!allPatterns.contains(pat))
            allPatterns.append(pat);
    }

    qDebug() << "[scanSourceFiles] Using patterns:" << allPatterns;

    // Recursive iterator excluding "build" folder
    QDirIterator it(dir.absolutePath(), allPatterns, QDir::Files | QDir::NoSymLinks | QDir::Readable,
                    QDirIterator::Subdirectories);

    while (it.hasNext()) {
        QString filePath = it.next();

        //TODO: REPLACE THIS WITH EXCLUDE SETTINGS
        // Exclude any path containing build
        QString normalizedPath = QDir::toNativeSeparators(filePath).toLower();
        if (normalizedPath.contains(QDir::toNativeSeparators("/build/")) ||
            normalizedPath.contains(QDir::toNativeSeparators("\\build\\"))) {
            qDebug() << "[scanSourceFiles] Skipping file in build folder:" << filePath;
            continue;
        }

        // Also exclude if path ends with build folder (e.g. .../build or ...\build)
        QFileInfo fi(filePath);
        QStringList parts = fi.absoluteFilePath().split(QDir::separator());
        if (parts.contains("build")) {
            qDebug() << "[scanSourceFiles] Skipping file due to build folder in path:" << filePath;
            continue;
        }

        if (parts.contains("newconfig.json")) {
            qDebug() << "skipping VK:" << filePath;
            continue;
        }

        results.append(filePath);
    }

    // Sort alphabetically for consistent output
    results.sort();

    qDebug() << "[scanSourceFiles] Total files found:" << results.size();

    return results;
}

bool CommandPipeManager::writeAmalgamatedSource(const QStringList &filePaths, QString &errorOut) const
{

    if (filePaths.isEmpty()) {
        errorOut = "No files to write in amalgamated source";
        qWarning() << "[writeAmalgamatedSource]" << errorOut;
        return false;
    }

    QDir cacheDir(m_sessionCacheFolder);
    if (!cacheDir.exists()) {
        qDebug() << "[writeAmalgamatedSource] Creating session cache folder:" << m_sessionCacheFolder;
        if (!QDir().mkpath(m_sessionCacheFolder)) {
            errorOut = QString("Failed to create session cache folder: %1").arg(m_sessionCacheFolder);
            qWarning() << "[writeAmalgamatedSource]" << errorOut;
            return false;
        }
    }

    QString srcCacheDirPath = cacheDir.filePath("src");
    QDir srcCacheDir(srcCacheDirPath);
    if (!srcCacheDir.exists()) {
        qDebug() << "[writeAmalgamatedSource] Creating src cache folder:" << srcCacheDirPath;
        if (!QDir().mkpath(srcCacheDirPath)) {
            errorOut = QString("Failed to create src cache folder: %1").arg(srcCacheDirPath);
            qWarning() << "[writeAmalgamatedSource]" << errorOut;
            return false;
        }
    }

    QString outputFilePath = srcCacheDir.filePath("src.txt");
    qDebug() << "[writeAmalgamatedSource] Absolute output file path:" << QFileInfo(outputFilePath).absoluteFilePath();

    QFile outFile(outputFilePath);
    if (!outFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        errorOut = QString("Failed to open output file for writing: %1").arg(outputFilePath);
        qWarning() << "[writeAmalgamatedSource]" << errorOut;
        return false;
    }

    QTextStream out(&outFile);

    qDebug() << "[writeAmalgamatedSource] Writing amalgamated source to:" << outputFilePath;

    for (const QString &filePath : filePaths) {
        QFileInfo fi(filePath);
        QString relativePath = QDir(m_project->rootFolder()).relativeFilePath(filePath);

        out << "### `" << relativePath << "`\n";
        out << "```cpp\n";

        QFile inFile(filePath);
        if (!inFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            out << "[Error: Could not open file]\n";
            qWarning() << "[writeAmalgamatedSource] Failed to open source file for reading:" << filePath;
        } else {
            QTextStream in(&inFile);
            while (!in.atEnd()) {
                QString line = in.readLine();
                out << line << "\n";
            }
            inFile.close();
        }

        out << "```\n\n";
    }

    outFile.close();

    qDebug() << "[writeAmalgamatedSource] Finished writing amalgamated source.";

    return true;
}
