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

namespace Ui {
class MainWindow;
}

// Mostly GUI handling. Right now also maintains all open traces
// forever, but we need something better to do with that
class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

public slots:
    void launchOTFOptions();
    void launchVisOptions();


    // Signal relays
    void pushSteps(float start, float stop, bool jump = false);
    void selectEvent(Event * event, bool aggregate, bool overdraw);
    void selectTasks(QList<int> tasks, Gnome *gnome);

    // Importing & Progress Bar
    void importOTFbyGUI();
    void traceFinished(Trace * trace);
    void updateProgress(int portion, QString msg);
    void traceSwitch();

    // High level GUI update
    void handleSplitter(int pos, int index);
    void handleSideSplitter(int pos, int index);
    void toggleLogicalSteps();
    void toggleClusteredSteps();
    void togglePhysicalTime();
    void toggleMetricOverview();

    // Change Traces
    void traceTriggered(QAction * action);
    void closeTrace();


signals:
    void operate(const QString &);
    
private:
    Ui::MainWindow *ui;
    void importTrace(QString dataFileName);
    void activeTraceChanged();
    void linkSideSplitter();
    void linkMainSplitter();
    void setVisWidgetState();

    // Saving traces & vis
    QList<Trace *> traces;
    QVector<VisWidget *> viswidgets;
    QList<QAction *> visactions;
    QVector<int> splitterMap;
    int activeTrace;

    // For progress bar
    OTFImportFunctor * importWorker;
    QThread * importThread;
    QProgressDialog * progress;

    // Import Trace options
    OTFImportOptions * otfoptions;
    ImportOptionsDialog * otfdialog;

    // Color stuff & other vis options
    VisOptions * visoptions;
    VisOptionsDialog * visdialog;

    QString activetracename;

    QStack<QString> activetraces;
};

#endif // MAINWINDOW_H
