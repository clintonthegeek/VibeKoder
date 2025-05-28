#include "project.h"
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include "toml.hpp"

using namespace toml;

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
    try {
        auto result = parse(content.toStdString());
        if (!result.succeeded()) {
            qWarning() << "Toml parsing failed:" << QString::fromStdString(result.error().description());
            return false;
        }
        const table &root = result.table();

        // Parse [api]
        if (auto apiNode = root.get("api")) {
            if (auto t = apiNode->as_table()) {
                m_accessToken = t->get("access_token") ? t->get("access_token")->value<std::string>().value_or("") : "";
                m_model = t->get("model") ? t->get("model")->value<std::string>().value_or("gpt-4") : "gpt-4";

                m_maxTokens = t->get("max_tokens") ? *t->get("max_tokens")->value<int>() : 1000;
                m_temperature = t->get("temperature") ? *t->get("temperature")->value<double>() : 0.3;
                m_topP = t->get("top_p") ? *t->get("top_p")->value<double>() : 1.0;
                m_frequencyPenalty = t->get("frequency_penalty") ? *t->get("frequency_penalty")->value<double>() : 0.0;
                m_presencePenalty = t->get("presence_penalty") ? *t->get("presence_penalty")->value<double>() : 0.0;
            }
        }

        // Parse [folders]
        if (auto foldersNode = root.get("folders")) {
            if (auto t = foldersNode->as_table()) {
                m_rootFolder = t->get("root") ? QString::fromStdString(t->get("root")->value<std::string>().value_or("")) : "";
                if (auto arr = t->get("include_docs") ? t->get("include_docs")->as_array() : nullptr) {
                    m_includeDocs.clear();
                    for (const auto& el : *arr) {
                        if (el.is_value() && el.type() == node_type::string) {
                            m_includeDocs << QString::fromStdString(el.value<std::string>().value());
                        }
                    }
                }
                if (auto arr = t->get("src") ? t->get("src")->as_array() : nullptr) {
                    m_sourceFolders.clear();
                    for (const auto& el : *arr) {
                        if (el.is_value() && el.type() == node_type::string) {
                            m_sourceFolders << QString::fromStdString(el.value<std::string>().value());
                        }
                    }
                } else if (t->get("src") && t->get("src")->is_value() && t->get("src")->type() == node_type::string) {
                    // Support string src too
                    m_sourceFolders.clear();
                    m_sourceFolders << QString::fromStdString(t->get("src")->value<std::string>().value());
                }
            }
        }

        // Parse [filetypes]
        if (auto ftNode = root.get("filetypes")) {
            if (auto t = ftNode->as_table()) {
                if (auto arr = t->get("source") ? t->get("source")->as_array() : nullptr) {
                    m_sourceFileTypes.clear();
                    for (const auto& el : *arr) {
                        if (el.is_value() && el.type() == node_type::string) {
                            m_sourceFileTypes << QString::fromStdString(el.value<std::string>().value());
                        }
                    }
                }
                if (auto arr = t->get("docs") ? t->get("docs")->as_array() : nullptr) {
                    m_docFileTypes.clear();
                    for (const auto& el : *arr) {
                        if (el.is_value() && el.type() == node_type::string) {
                            m_docFileTypes << QString::fromStdString(el.value<std::string>().value());
                        }
                    }
                }
            }
        }

        // Parse [command_pipes]
        if (auto cmdNode = root.get("command_pipes")) {
            if (auto t = cmdNode->as_table()) {
                m_commandPipes.clear();
                for (auto& [key, val] : *t) {
                    if (val.is_value() && val.type() == node_type::string) {
                        m_commandPipes.insert(QString::fromStdString(key), QString::fromStdString(val.value<std::string>().value()));
                    }
                }
            }
        }

        return true;
    } catch (const parse_error &err) {
        qWarning() << "TOML parsing exception:" << err.what();
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
