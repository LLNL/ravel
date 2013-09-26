#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
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
    
private:
    Ui::MainWindow *ui;

    QVector<Trace *> traces;
};

#endif // MAINWINDOW_H
