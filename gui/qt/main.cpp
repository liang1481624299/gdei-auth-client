// gui/qt/main.cpp
// Qt GUI 入口 / Qt GUI entry point.
#include <QApplication>
#include "mainwindow.hpp"

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    MainWindow w;
    w.show();
    return app.exec();
}
