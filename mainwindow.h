#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "viswidget.h"
#include "trace.h"
#include "otfimportoptions.h"
#include "importoptionsdialog.h"

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
    void launchOTFOptions();
    void importJSON();
    void importOTFbyGUI();
    void pushSteps(float start, float stop);
    void selectEvent(Event * event);
    
private:
    Ui::MainWindow *ui;
    void importOTF(QString dataFileName);

    QVector<Trace *> traces;
    QVector<VisWidget *> viswidgets;

    // Import Trace options
    OTFImportOptions * otfoptions;
    ImportOptionsDialog * otfdialog;
};

#endif // MAINWINDOW_H
