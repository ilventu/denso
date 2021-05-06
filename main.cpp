#include "densitometer.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Densitometer w;
    w.show();
    return a.exec();
}
