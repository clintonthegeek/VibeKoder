#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    bool loaded = AppConfig::instance().load();
    qDebug() << "[main] AppConfig loaded:" << loaded;

    MainWindow w;
    w.show();
    return a.exec();
}
