/* Coordination Adjusted Parallel Timeline Information Visualization and Analysis Timeline Extractor */
/* Coordination Adjusted Parallel Trace Information Visualization and Analysis Timeline Explorer */
#include <QApplication>
#include "mainwindow.h"

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    MainWindow w;
    w.show();
    return app.exec();
}
