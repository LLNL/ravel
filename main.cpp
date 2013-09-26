/* Coordination Adjusted Parallel Trace Information Visualization and Analysis Timeline Extractor */
#include <QApplication>
#include "mainwindow.h"

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    MainWindow w;
    w.show();
    return app.exec();
}
