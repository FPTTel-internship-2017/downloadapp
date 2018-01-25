#include "download.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    download w;
    w.show();

    return a.exec();
}
