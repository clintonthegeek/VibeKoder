#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <qplaintextedit.h>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class Project;
class Session;
class OpenAIRequest;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onOpenProject();
    void onOpenSession();
    void onSaveSession();
    void onSendPrompt();
    void onOpenAIResponse(const QString &response);
    void onOpenAIError(const QString &error);

private:
    Ui::MainWindow *ui;

    Project *m_project = nullptr;
    Session *m_session = nullptr;
    OpenAIRequest *m_openAI = nullptr;

    void clearSession();
    void moveCursorToUserPrompt(QPlainTextEdit *editor);
};

#endif // MAINWINDOW_H
