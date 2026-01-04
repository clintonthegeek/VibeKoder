#ifndef PROJECTSETTINGSWIDGET_NEW_H
#define PROJECTSETTINGSWIDGET_NEW_H

#include <QWidget>
#include "projectconfig.h"

class QTabWidget;
class QLineEdit;
class QSpinBox;
class QDoubleSpinBox;
class QListWidget;
class QTableWidget;
class QPushButton;

/**
 * @brief Widget for editing project settings (without dialog buttons).
 * Can be used standalone or embedded in dialogs.
 */
class ProjectSettingsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ProjectSettingsWidget(QWidget *parent = nullptr);

    // Load project settings into the widget
    void loadSettings(const ProjectConfig &config);

    // Retrieve current settings from the widget
    ProjectConfig getSettings() const;

private slots:
    void onBrowseRootFolder();
    void onAddSourceFileType();
    void onRemoveSourceFileType();
    void onAddDocFileType();
    void onRemoveDocFileType();
    void onAddCommandPipe();
    void onRemoveCommandPipe();

private:
    void setupUi();
    QWidget* createApiTab();
    QWidget* createFoldersTab();
    QWidget* createFiletypesTab();
    QWidget* createCommandPipesTab();

    // API tab widgets
    QLineEdit* m_apiAccessToken;
    QLineEdit* m_apiModel;
    QSpinBox* m_apiMaxTokens;
    QDoubleSpinBox* m_apiTemperature;
    QDoubleSpinBox* m_apiTopP;
    QDoubleSpinBox* m_apiFrequencyPenalty;
    QDoubleSpinBox* m_apiPresencePenalty;

    // Folders tab widgets
    QLineEdit* m_rootFolder;
    QLineEdit* m_docsFolder;
    QLineEdit* m_srcFolder;
    QLineEdit* m_sessionsFolder;
    QLineEdit* m_templatesFolder;

    // Filetypes tab widgets
    QListWidget* m_sourceFileTypesList;
    QPushButton* m_addSourceBtn;
    QPushButton* m_removeSourceBtn;
    QListWidget* m_docFileTypesList;
    QPushButton* m_addDocBtn;
    QPushButton* m_removeDocBtn;

    // Command Pipes tab widgets
    QTableWidget* m_commandPipesTable;
    QPushButton* m_addPipeBtn;
    QPushButton* m_removePipeBtn;

    QTabWidget* m_tabWidget;
};

#endif // PROJECTSETTINGSWIDGET_NEW_H
