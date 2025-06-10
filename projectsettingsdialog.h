#ifndef PROJECTSETTINGSDIALOG_H
#define PROJECTSETTINGSDIALOG_H

#include <QDialog>
#include <QVariantMap>
#include <QStringList>
#include <QMap>

class QLineEdit;
class QTableWidget;
class QTabWidget;

class ProjectSettingsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ProjectSettingsDialog(QWidget* parent = nullptr);

    // Load settings from QVariantMaps (e.g., from Project or AppConfig)
    void loadSettings(const QVariantMap& apiSettings,
                      const QVariantMap& folders,
                      const QStringList& sourceFileTypes,
                      const QStringList& docFileTypes,
                      const QMap<QString, QStringList>& commandPipes);

    // Extract current settings from UI
    void getSettings(QVariantMap& apiSettings,
                     QVariantMap& folders,
                     QStringList& sourceFileTypes,
                     QStringList& docFileTypes,
                     QMap<QString, QStringList>& commandPipes) const;

private slots:
    void onAddCommandPipe();
    void onRemoveCommandPipe();

private:
    void setupUi();
    void populateCommandPipesTable();

    QVariantMap m_apiSettings;
    QVariantMap m_folders;
    QStringList m_sourceFileTypes;
    QStringList m_docFileTypes;
    QMap<QString, QStringList> m_commandPipes;

    QTabWidget* m_tabWidget = nullptr;

    // API tab widgets
    QLineEdit* m_accessTokenEdit = nullptr;
    QLineEdit* m_modelEdit = nullptr;
    QLineEdit* m_maxTokensEdit = nullptr;
    QLineEdit* m_temperatureEdit = nullptr;
    QLineEdit* m_topPEdit = nullptr;
    QLineEdit* m_frequencyPenaltyEdit = nullptr;
    QLineEdit* m_presencePenaltyEdit = nullptr;

    // Folders tab widgets
    QLineEdit* m_rootFolderEdit = nullptr;
    QLineEdit* m_docsFolderEdit = nullptr;
    QLineEdit* m_srcFolderEdit = nullptr;
    QLineEdit* m_sessionsFolderEdit = nullptr;
    QLineEdit* m_templatesFolderEdit = nullptr;
    QLineEdit* m_includeDocsFolderEdit = nullptr;

    // Filetypes tab widgets
    QLineEdit* m_sourceFileTypesEdit = nullptr; // comma separated
    QLineEdit* m_docFileTypesEdit = nullptr;    // comma separated

    // Command pipes tab widgets
    QTableWidget* m_commandPipesTable = nullptr;
};

#endif // PROJECTSETTINGSDIALOG_H
