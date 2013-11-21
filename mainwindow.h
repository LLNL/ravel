#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "viswidget.h"
#include "trace.h"

#include "json/json.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

public slots:
    void importJSON();
    void importOTFbyGUI();
    void pushSteps(float start, float stop);
    
private:
    Ui::MainWindow *ui;
    void importOTF(QString dataFileName);

    QVector<Trace *> traces;
    QVector<VisWidget *> viswidgets;
};

#endif // MAINWINDOW_H
