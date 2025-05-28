#include "project.h"
#include <QFile>
#include <QDebug>
#include "toml.hpp"

// Use fully qualified experimental namespace to be explicit:
namespace tomlex = toml::v3::ex;

Project::Project()
    : m_maxTokens(1000)
    , m_temperature(0.3)
    , m_topP(1.0)
    , m_frequencyPenalty(0.0)
    , m_presencePenalty(0.0)
{
}


bool Project::load(const QString &filepath)
{
    m_rawTomlPath = filepath;
    QFile file(filepath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Could not open project file:" << filepath;
        return false;
    }

    QTextStream in(&file);
    QString content = in.readAll();

    return parseToml(content);
}

bool Project::save(const QString &filepath)
{
    // For phase 1, saving not needed; stub
    return false;
}


bool Project::parseToml(const QString &content)
{
    try
    {
        // Experimental API: parse returns table directly (not parse_result)
        toml::table root = tomlex::parse(content.toStdString());

        // No failed() or error(): consider empty table as parse failure
        if (root.empty())
        {
            qWarning() << "TOML parse failed: empty or invalid document";
            return false;
        }

        // [api]
        if (auto apiNode = root.get("api"))
        {
            if (auto apiTable = apiNode->as_table())
            {
                if (auto tokenNode = apiTable->get("access_token"))
                {
                    // In experimental: value<T>() returns raw T, so use as_string()
                    if (auto valStr = tokenNode->as_string())
                        m_accessToken = QString::fromStdString(valStr->get());
                }

                if (auto modelNode = apiTable->get("model"))
                {
                    if (auto valStr = modelNode->as_string())
                        m_model = QString::fromStdString(valStr->get());
                }
                else
                {
                    m_model = "gpt-4";
                }

                if (auto maxTokensNode = apiTable->get("max_tokens"))
                {
                    if (auto valInt = maxTokensNode->as_integer())
                        m_maxTokens = static_cast<int>(valInt->get());
                }

                if (auto tempNode = apiTable->get("temperature"))
                {
                    if (auto valDouble = tempNode->as_floating_point())
                        m_temperature = valDouble->get();
                }

                if (auto topPNode = apiTable->get("top_p"))
                {
                    if (auto valDouble = topPNode->as_floating_point())
                        m_topP = valDouble->get();
                }

                if (auto freqNode = apiTable->get("frequency_penalty"))
                {
                    if (auto valDouble = freqNode->as_floating_point())
                        m_frequencyPenalty = valDouble->get();
                }

                if (auto presNode = apiTable->get("presence_penalty"))
                {
                    if (auto valDouble = presNode->as_floating_point())
                        m_presencePenalty = valDouble->get();
                }
            }
        }

        // [folders]
        if (auto foldersNode = root.get("folders"))
        {
            if (auto foldersTable = foldersNode->as_table())
            {
                if (auto rootNode = foldersTable->get("root"))
                {
                    if (auto valStr = rootNode->as_string())
                        m_rootFolder = QString::fromStdString(valStr->get());
                }

                if (auto includeDocsNode = foldersTable->get("include_docs"))
                {
                    m_includeDocs.clear();
                    if (includeDocsNode->is_array())
                    {
                        for (const auto& el : *includeDocsNode->as_array())
                        {
                            if (auto valStr = el.as_string())
                                m_includeDocs << QString::fromStdString(valStr->get());
                        }
                    }
                    else if (auto valStr = includeDocsNode->as_string())
                    {
                        m_includeDocs << QString::fromStdString(valStr->get());
                    }
                }

                if (auto srcNode = foldersTable->get("src"))
                {
                    m_sourceFolders.clear();
                    if (srcNode->is_array())
                    {
                        for (const auto& el : *srcNode->as_array())
                        {
                            if (auto valStr = el.as_string())
                                m_sourceFolders << QString::fromStdString(valStr->get());
                        }
                    }
                    else if (auto valStr = srcNode->as_string())
                    {
                        m_sourceFolders << QString::fromStdString(valStr->get());
                    }
                }
            }
        }

        // [filetypes]
        if (auto ftNode = root.get("filetypes"))
        {
            if (auto ftTable = ftNode->as_table())
            {
                if (auto sourceNode = ftTable->get("source"))
                {
                    m_sourceFileTypes.clear();
                    if (sourceNode->is_array())
                    {
                        for (const auto& el : *sourceNode->as_array())
                        {
                            if (auto valStr = el.as_string())
                                m_sourceFileTypes << QString::fromStdString(valStr->get());
                        }
                    }
                    else if (auto valStr = sourceNode->as_string())
                    {
                        m_sourceFileTypes << QString::fromStdString(valStr->get());
                    }
                }
                if (auto docsNode = ftTable->get("docs"))
                {
                    m_docFileTypes.clear();
                    if (docsNode->is_array())
                    {
                        for (const auto& el : *docsNode->as_array())
                        {
                            if (auto valStr = el.as_string())
                                m_docFileTypes << QString::fromStdString(valStr->get());
                        }
                    }
                    else if (auto valStr = docsNode->as_string())
                    {
                        m_docFileTypes << QString::fromStdString(valStr->get());
                    }
                }
            }
        }

        // [command_pipes]
        if (auto cmdNode = root.get("command_pipes"))
        {
            if (auto cmdTable = cmdNode->as_table())
            {
                m_commandPipes.clear();
                for (const auto& [keyObj, valNode] : *cmdTable)
                {
                    const std::string keyStr{ keyObj.str() }; // explicit conversion from string_view to string

                    if (auto valStr = valNode.as_string())
                    {
                        m_commandPipes.insert(
                            QString::fromStdString(keyStr),
                            QString::fromStdString(valStr->get())
                            );
                    }
                }
            }
        }

        return true;
    }
    catch (const std::exception &ex)
    {
        qWarning() << "Exception while parsing TOML:" << ex.what();
        return false;
    }
    catch (...)
    {
        qWarning() << "Unknown error while parsing TOML";
        return false;
    }
}

// Getters

QString Project::accessToken() const { return m_accessToken; }
QString Project::model() const { return m_model; }
int Project::maxTokens() const { return m_maxTokens; }
double Project::temperature() const { return m_temperature; }
double Project::topP() const { return m_topP; }
double Project::frequencyPenalty() const { return m_frequencyPenalty; }
double Project::presencePenalty() const { return m_presencePenalty; }

QString Project::rootFolder() const { return m_rootFolder; }
QStringList Project::includeDocs() const { return m_includeDocs; }
QStringList Project::sourceFolders() const { return m_sourceFolders; }
QStringList Project::sourceFileTypes() const { return m_sourceFileTypes; }
QStringList Project::docFileTypes() const { return m_docFileTypes; }
QMap<QString, QString> Project::commandPipes() const { return m_commandPipes; }
