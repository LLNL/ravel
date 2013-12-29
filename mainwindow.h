#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QElapsedTimer>
#include <QThread>
#include <QProgressDialog>
#include "viswidget.h"
#include "trace.h"
#include "otfimportoptions.h"
#include "importoptionsdialog.h"
#include "visoptions.h"
#include "visoptionsdialog.h"
#include "colormap.h"
#include "otfimportfunctor.h"

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
    void pushSteps(float start, float stop, bool jump = false);
    void selectEvent(Event * event);
    void handleSplitter(int pos, int index);
    void traceFinished(Trace * trace);
    void updateProgress(int portion, QString msg);

signals:
    void operate(const QString &);
    
private:
    Ui::MainWindow *ui;
    void importOTF(QString dataFileName);
    void activeTraceChanged();

    QVector<Trace *> traces;
    QVector<VisWidget *> viswidgets;
    int activeTrace;

    OTFImportFunctor * importWorker;
    QThread * importThread;
    QProgressDialog * progress;

    // Import Trace options
    OTFImportOptions * otfoptions;
    ImportOptionsDialog * otfdialog;

    // Color stuff & other vis options
    VisOptions * visoptions;
    VisOptionsDialog * visdialog;
};

#endif // MAINWINDOW_H
