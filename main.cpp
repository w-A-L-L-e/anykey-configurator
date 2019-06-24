#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    // make windows with retina or 4k still look good
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    return a.exec();
}
