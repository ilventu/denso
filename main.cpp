#include "denso.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Denso w;
    w.show();
    return a.exec();
}
