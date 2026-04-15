#include "mainwindow.h"

#include <QApplication>

#if !defined(MODEL_TEST) && !defined(CANVAS_TEST)
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}
#endif
