#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QElapsedTimer>
#include "viswidget.h"
#include "trace.h"
#include "otfimportoptions.h"
#include "importoptionsdialog.h"
#include "visoptions.h"
#include "visoptionsdialog.h"
#include "colormap.h"

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
    void launchVisOptions();
    void importJSON();
    void importOTFbyGUI();
    void pushSteps(float start, float stop);
    void selectEvent(Event * event);
    void handleSplitter(int pos, int index);
    
private:
    Ui::MainWindow *ui;
    void importOTF(QString dataFileName);
    void activeTraceChanged();

    QVector<Trace *> traces;
    QVector<VisWidget *> viswidgets;
    int activeTrace;

    // Import Trace options
    OTFImportOptions * otfoptions;
    ImportOptionsDialog * otfdialog;

    // Color stuff & other vis options
    VisOptions * visoptions;
    VisOptionsDialog * visdialog;
};

#endif // MAINWINDOW_H
