#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "overviewvis.h"
#include "stepvis.h"
#include "traditionalvis.h"
#include "clustervis.h"
#include "clustertreevis.h"
#include "otfconverter.h"
#include "importoptionsdialog.h"
#include "general_util.h"
#include "verticallabel.h"
#include "viswidget.h"
#include "trace.h"
#include "otfimportoptions.h"
#include "visoptions.h"
#include "visoptionsdialog.h"
#include "otfimportfunctor.h"
#include "otf2exportfunctor.h"

#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include "qtconcurrentrun.h"
#include <iostream>
#include <string>
#include <QElapsedTimer>
#include <QThread>
#include <QProgressDialog>
#include <QSettings>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    traces(QList<Trace *>()),
    viswidgets(QVector<VisWidget *>()),
    visactions(QList<QAction *>()),
    splitterMap(QVector<int>()),
    splitterActions(QVector<QAction *>()),
    activeTrace(-1),
    importWorker(NULL),
    importThread(NULL),
    progress(NULL),
    otfoptions(new OTFImportOptions()),
    otfdialog(NULL),
    visoptions(new VisOptions()),
    visdialog(NULL),
    activetracename(""),
    activetraces(QStack<QString>()),
    dataDirectory(""),
    otf1Support(false)
{
    readSettings();
    ui->setupUi(this);
    #ifdef OTF1LIB
        otf1Support = true;
        std::cout << "OTF1 SUPPORT" << std::endl;
    #endif

    // Overview
    OverviewVis* overview = new OverviewVis(ui->overviewContainer, visoptions);
    ui->overviewContainer->layout()->addWidget(overview);
    ui->overviewLabelWidget->setLayout(new QVBoxLayout());
    ui->overviewLabelWidget->layout()->addWidget(new VerticalLabel("Overview",
                                                                   ui->overviewLabelWidget));

    connect(overview, SIGNAL(stepsChanged(float, float, bool)), this,
            SLOT(pushSteps(float, float, bool)));
    viswidgets.push_back(overview);
    splitterMap.push_back(3);
    splitterActions.push_back(ui->actionMetric_Overview);

    // Logical Timeline
    StepVis* stepvis = new StepVis(ui->stepContainer, visoptions);
    ui->stepContainer->layout()->addWidget(stepvis);
    ui->logicalLabelWidget->setLayout(new QVBoxLayout());
    ui->logicalLabelWidget->layout()->addWidget(new VerticalLabel("Logical",
                                                                  ui->logicalLabelWidget));

    connect((stepvis), SIGNAL(stepsChanged(float, float, bool)), this,
            SLOT(pushSteps(float, float, bool)));
    connect((stepvis), SIGNAL(eventClicked(Event *, bool, bool)), this,
            SLOT(selectEvent(Event *, bool, bool)));
    viswidgets.push_back(stepvis);
    splitterMap.push_back(0);
    splitterActions.push_back(ui->actionLogical_Steps);

    // Physical Timeline
    TraditionalVis* timevis = new TraditionalVis(ui->traditionalContainer,
                                                 visoptions);
    timevis->setClosed(true);
    ui->traditionalContainer->layout()->addWidget(timevis);
    ui->traditionalLabelWidget->setLayout(new QVBoxLayout());
    ui->traditionalLabelWidget->layout()->addWidget(new VerticalLabel("Physical",
                                                                      ui->traditionalLabelWidget));
    QLabel * timescaleLabel = new QLabel(ui->traditionalLabelWidget);
    timescaleLabel->setFont(QFont("Helvetica", 10));
    ui->traditionalLabelWidget->layout()->addWidget(timescaleLabel);
    timescaleLabel->setAlignment(Qt::AlignRight | Qt::AlignBottom);

    connect((timevis), SIGNAL(stepsChanged(float, float, bool)), this,
            SLOT(pushSteps(float, float, bool)));
    connect((timevis), SIGNAL(eventClicked(Event *, bool, bool)), this,
            SLOT(selectEvent(Event *, bool, bool)));
    connect((timevis), SIGNAL(timeScaleString(QString)), timescaleLabel,
            SLOT(setText(QString)));
    viswidgets.push_back(timevis);
    splitterMap.push_back(2);
    splitterActions.push_back(ui->actionPhysical_Time);

    // Cluster View
    ClusterTreeVis* clustertreevis = new ClusterTreeVis(ui->stepContainer,
                                                        visoptions);
    ui->treeContainer->layout()->addWidget(clustertreevis);

    ClusterVis* clustervis = new ClusterVis(clustertreevis, ui->stepContainer,
                                            visoptions);
    ui->clusterContainer->layout()->addWidget(clustervis);
    ui->clusterLabelWidget->setLayout(new QVBoxLayout());
    ui->clusterLabelWidget->layout()->addWidget(new VerticalLabel("Clusters",
                                                                  ui->clusterLabelWidget));

    connect((clustervis), SIGNAL(stepsChanged(float, float, bool)), this,
            SLOT(pushSteps(float, float, bool)));
    connect((clustervis), SIGNAL(eventClicked(Event *, bool, bool)), this,
            SLOT(selectEvent(Event *, bool, bool)));
    connect((clustervis), SIGNAL(tasksSelected(QList<int>, Gnome*)), this,
            SLOT(selectTasks(QList<int>, Gnome*)));

    connect((clustervis), SIGNAL(focusGnome()), clustertreevis,
            SLOT(repaint()));
    connect((clustervis), SIGNAL(clusterChange()), clustertreevis,
            SLOT(clusterChanged()));
    connect((clustertreevis), SIGNAL(clusterChange()), clustervis,
            SLOT(clusterChanged()));
    connect((clustervis), SIGNAL(neighborChange(int)), clustertreevis,
            SLOT(repaint()));

    viswidgets.push_back(clustervis);
    viswidgets.push_back(clustertreevis);
    splitterMap.push_back(1);
    splitterActions.push_back(ui->actionClustered_Logical_Steps);

    // Sliders
    connect(ui->verticalSlider, SIGNAL(valueChanged(int)), clustervis,
            SLOT(changeNeighborRadius(int)));
    connect((clustervis), SIGNAL(neighborChange(int)), ui->verticalSlider,
            SLOT(setValue(int)));

    // Menu
    connect(ui->actionOpen_OTF, SIGNAL(triggered()),this,
            SLOT(importOTFbyGUI()));
    ui->actionOpen_OTF->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_O));

    connect(ui->actionSave, SIGNAL(triggered()), this,
            SLOT(saveCurrentTrace()));
    ui->actionSave->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_S));
    ui->actionSave->setEnabled(false);

    connect(ui->actionClose, SIGNAL(triggered()), this,
            SLOT(closeTrace()));
    ui->actionClose->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_C));
    ui->actionClose->setEnabled(false);

    connect(ui->actionOTF_Importing, SIGNAL(triggered()), this,
            SLOT(launchOTFOptions()));
    ui->actionOTF_Importing->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_I));
    connect(ui->actionVisualization, SIGNAL(triggered()), this,
            SLOT(launchVisOptions()));
    ui->actionVisualization->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_V));


    connect(ui->actionLogical_Steps, SIGNAL(triggered()), this,
            SLOT(toggleLogicalSteps()));
    ui->actionLogical_Steps->setShortcut(QKeySequence(Qt::ALT + Qt::Key_L));
    connect(ui->actionClustered_Logical_Steps, SIGNAL(triggered()), this,
            SLOT(toggleClusteredSteps()));
    ui->actionClustered_Logical_Steps->setShortcut(QKeySequence(Qt::ALT + Qt::Key_C));
    connect(ui->actionPhysical_Time, SIGNAL(triggered()), this,
            SLOT(togglePhysicalTime()));
    ui->actionPhysical_Time->setShortcut(QKeySequence(Qt::ALT + Qt::Key_P));
    connect(ui->actionMetric_Overview, SIGNAL(triggered()), this,
            SLOT(toggleMetricOverview()));
    ui->actionMetric_Overview->setShortcut(QKeySequence(Qt::ALT + Qt::Key_M));
    visactions.append(ui->actionLogical_Steps);
    visactions.append(ui->actionClustered_Logical_Steps);
    visactions.append(ui->actionPhysical_Time);
    visactions.append(ui->actionMetric_Overview);

    connect(ui->actionQuit, SIGNAL(triggered()), this, SLOT(close()));
    ui->actionQuit->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Q));

    ui->menuTraces->setEnabled(false);
    connect(ui->menuTraces, SIGNAL(triggered(QAction *)), this,
            SLOT(traceTriggered(QAction *)));

    // Splitters
    connect(ui->splitter, SIGNAL(splitterMoved(int, int)), this,
            SLOT(handleSplitter(int, int)));
    connect(ui->sideSplitter, SIGNAL(splitterMoved(int, int)), this,
            SLOT(handleSideSplitter(int, int)));
    ui->splitter->setStyleSheet("QSplitter::handle { background-color: black; }");
    ui->sideSplitter->setStyleSheet("QSplitter::handle { background-color: black; }");

    // Initial splitter sizes
    QList<int> sizes = QList<int>();
    sizes.append(500);
    sizes.append(500);
    sizes.append(500);
    sizes.append(70);
    ui->splitter->setSizes(sizes);
    ui->sideSplitter->setSizes(sizes);

}

MainWindow::~MainWindow()
{
    delete otfdialog;
    delete ui;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    Q_UNUSED(event);
    writeSettings();
}

void MainWindow::writeSettings()
{
    QSettings settings("LLNL", "Ravel");
    settings.beginGroup("Data");
    settings.setValue("directory", dataDirectory);
    settings.endGroup();
}

void MainWindow::readSettings()
{
    QSettings settings("LLNL", "Ravel");

    settings.beginGroup("Data");
    dataDirectory = settings.value("directory", "").toString();
    settings.endGroup();
}

// The following functions relay a signal from one vis (hooked up in the
// constructor) to all of the rest
void MainWindow::pushSteps(float start, float stop, bool jump)
{
    for(int i = 0; i < viswidgets.size(); i++)
    {
        viswidgets[i]->setSteps(start, stop, jump);
    }
}

void MainWindow::selectEvent(Event * event, bool aggregate, bool overdraw)
{
    for(int i = 0; i < viswidgets.size(); i++)
    {
        viswidgets[i]->selectEvent(event, aggregate, overdraw);
    }
}

void MainWindow::selectTasks(QList<int> tasks, Gnome * gnome)
{
    for(int i = 0; i < viswidgets.size(); i++)
    {
        viswidgets[i]->selectTasks(tasks, gnome);
    }
}

void MainWindow::launchOTFOptions()
{
    delete otfdialog;
    otfdialog = new ImportOptionsDialog(this, otfoptions);
    otfdialog->show();
}

void MainWindow::launchVisOptions()
{
    delete visdialog;
    if (activeTrace >= 0)
        visdialog = new VisOptionsDialog(this, visoptions,
                                         traces[activeTrace]);
    else
        visdialog = new VisOptionsDialog(this, visoptions);
    visdialog->show();

    for(int i = 0; i < viswidgets.size(); i++)
        viswidgets[i]->repaint();
}

void MainWindow::saveCurrentTrace()
{
    // Get save file name
    QFileInfo traceInfo = QFileInfo(traces[activeTrace]->fullpath);
    QString dataFileName = QFileDialog::getSaveFileName(this,
                                                        tr("Save Ravel Trace Data"),
                                                        QFileInfo(traceInfo.absoluteDir(),
                                                                  traceInfo.baseName()
                                                                  + ".save").absoluteFilePath(),
                                                        tr("Trace Files(*.otf2)"));

    QFileInfo saveFile = QFileInfo(dataFileName);

    progress = new QProgressDialog("Writing Trace...", "", 0, 0, this);
    progress->setWindowTitle("Exporting Trace...");
    progress->setCancelButton(0);
    progress->show();

    exportThread = new QThread();
    exportWorker = new OTF2ExportFunctor();
    exportWorker->moveToThread(exportThread);

    connect(this, SIGNAL(exportTrace(Trace *, QString, QString)), exportWorker,
            SLOT(exportTrace(Trace *, QString, QString)));

    connect(exportWorker, SIGNAL(done()), this,
            SLOT(exportFinished()));

    exportThread->start();
    emit(exportTrace(traces[activeTrace], saveFile.path(), saveFile.fileName()));
    /*
    OTF2Exporter exporter = OTF2Exporter(traces[activeTrace]);
    exporter.exportTrace(saveFile.path(), saveFile.fileName());
    */
}

void MainWindow::exportFinished()
{
    progress->close();

    delete exportWorker;
    delete progress;
    delete exportThread;
}

void MainWindow::importOTFbyGUI()
{
    // Now get the OTF File
    QString dataFileName = "";
    QString fileTypes = "Trace Files (*.otf2)";
#ifdef OTF1LIB
    fileTypes = "Trace Files (*.otf2 *.otf)";
#endif
    dataFileName = QFileDialog::getOpenFileName(this,
                                                tr("Import Trace Data"),
                                                dataDirectory,
                                                tr(fileTypes.toStdString().c_str()));
    qApp->processEvents();

    // Guard against Cancel
    if (dataFileName == NULL || dataFileName.length() == 0)
        return;

    importTrace(dataFileName);

    QStringList fileinfo = dataFileName.split('\/');
    int fisize = fileinfo.size();
    if (fisize > 1)
        activetracename = fileinfo[fisize-2] + "/" + fileinfo[fisize-1];
    else
        activetracename = dataFileName;
}

void MainWindow::importTrace(QString dataFileName){

    progress = new QProgressDialog("Reading Trace...", "", 0, 0, this);
    progress->setWindowTitle("Importing Trace...");
    progress->setCancelButton(0);
    progress->show();

    importThread = new QThread();
    importWorker = new OTFImportFunctor(otfoptions);
    importWorker->moveToThread(importThread);

    if (dataFileName.endsWith("otf", Qt::CaseInsensitive))
    {
        otfoptions->origin = OTFImportOptions::OF_OTF;
        connect(this, SIGNAL(operate(QString)), importWorker,
                SLOT(doImportOTF(QString)));
    }
    else //(dataFileName.endsWith("otf2", Qt::CaseInsensitive))
    {
        otfoptions->origin = OTFImportOptions::OF_OTF2;
        otfoptions->waitallMerge = false; // Not applicable
        connect(this, SIGNAL(operate(QString)), importWorker,
                SLOT(doImportOTF2(QString)));
    }

    connect(importWorker, SIGNAL(switching()), this, SLOT(traceSwitch()));
    connect(importWorker, SIGNAL(done(Trace *)), this,
            SLOT(traceFinished(Trace *)));
    connect(importWorker, SIGNAL(reportProgress(int, QString)), this,
            SLOT(updateProgress(int, QString)));

    importThread->start();
    emit(operate(dataFileName));
}

// Switch importing progress bar - something weird currently happens here
void MainWindow::traceSwitch()
{
    progress->close();
    delete progress;

    progress = new QProgressDialog("Clustering...","",0,0,this);
    progress->setWindowTitle("Clustering Progress");
    progress->setCancelButton(0);
    progress->show();
}

void MainWindow::traceFinished(Trace * trace)
{
    progress->close();

    delete importWorker;
    delete progress;
    delete importThread;

    if (!trace)
    {
        QMessageBox msgBox;
        msgBox.setText("Error: Trace processing failed.");
        msgBox.exec();
        return;
    }
    else if (trace->partitions->size() < 1)
    {
        QMessageBox msgBox;
        msgBox.setText("Error: No communication phases found. Abandoning trace.");
        msgBox.exec();
        return;
    }

    this->traces.push_back(trace);
    if (activeTrace >= 0)
        ui->menuTraces->actions().at(activeTrace)->setChecked(false);
    activeTrace = traces.size() - 1;

    QFileInfo traceInfo = QFileInfo(traces[activeTrace]->fullpath);
    QDir traceDir = traceInfo.absoluteDir();
    if (!traceDir.cdUp())
    {
        traceDir = traceInfo.absoluteDir();
    }
    dataDirectory = traceDir.absolutePath();

    trace->name = activetracename;
    QAction * action = ui->menuTraces->addAction(activetracename);
    action->setCheckable(true);
    activeTraceChanged(!activetraces.size());
}

void MainWindow::updateProgress(int portion, QString msg)
{
    progress->setMaximum(100);
    progress->setLabelText(msg);
    progress->setValue(portion);
}

// In the future we may want to reset splitters to a save state
// rather than a default... or not reset splitters at all
void MainWindow::activeTraceChanged(bool first)
{
    for(int i = 0; i < viswidgets.size(); i++)
    {
        viswidgets[i]->setTrace(traces[activeTrace]);
        viswidgets[i]->processVis();
        viswidgets[i]->repaint();
    }
    QList<int> splitter_sizes = ui->splitter->sizes();

    // On the first trace, choose these default settings
    if (first)
    {
        viswidgets[STEPVIS]->setClosed(false);
        if (otfoptions->cluster)
        {
            int traditional_height = splitter_sizes[splitterMap[TIMEVIS]];
            splitter_sizes[splitterMap[STEPVIS]] += traditional_height / 2;
            splitter_sizes[splitterMap[CLUSTERVIS]] += traditional_height / 2;
            viswidgets[CLUSTERVIS]->setClosed(false);
        }
        else
        {
            splitter_sizes[splitterMap[STEPVIS]] += splitter_sizes[splitterMap[CLUSTERVIS]]
                                                    + splitter_sizes[splitterMap[TIMEVIS]];
            splitter_sizes[splitterMap[CLUSTERVIS]] = 0;
            viswidgets[CLUSTERVIS]->setClosed(true);
        }
        splitter_sizes[splitterMap[TIMEVIS]] = 0;
        viswidgets[TIMEVIS]->setClosed(true);

        ui->splitter->setSizes(splitter_sizes);
        ui->sideSplitter->setSizes(splitter_sizes);
        setVisWidgetState();
    }
    // If new trace does not have cluster but the cluster is open, close it
    else if (!otfoptions->cluster && !viswidgets[CLUSTERVIS]->isClosed())
    {
        splitter_sizes[splitterMap[CLUSTERVIS]] = 0;
        viswidgets[CLUSTERVIS]->setClosed(true);
        viswidgets[STEPVIS]->setClosed(false);
        if (viswidgets[TIMEVIS]->isClosed())
        {
            splitter_sizes[splitterMap[STEPVIS]] += splitter_sizes[splitterMap[CLUSTERVIS]]
                                                    + splitter_sizes[splitterMap[TIMEVIS]];
            splitter_sizes[splitterMap[TIMEVIS]] = 0;
            viswidgets[TIMEVIS]->setClosed(true);
        }
        else
        {
            splitter_sizes[splitterMap[STEPVIS]] += splitter_sizes[splitterMap[CLUSTERVIS]]/2;
            splitter_sizes[splitterMap[TIMEVIS]] += splitter_sizes[splitterMap[CLUSTERVIS]]/2;
            viswidgets[TIMEVIS]->setClosed(false);
        }

        ui->splitter->setSizes(splitter_sizes);
        ui->sideSplitter->setSizes(splitter_sizes);
        setVisWidgetState();
    }
    ui->actionClose->setEnabled(true);
    ui->actionSave->setEnabled(true);
    ui->menuTraces->setEnabled(true);

    QList<QAction *> actions = ui->menuTraces->actions();
    actions[activeTrace]->setChecked(true);

    activetraces.push(traces[activeTrace]->name);
    setWindowTitle("Ravel - " + traces[activeTrace]->name);

}

void MainWindow::traceTriggered(QAction * action)
{
    QList<QAction *> actions = ui->menuTraces->actions();

    actions[activeTrace]->setChecked(false);

    activeTrace = actions.indexOf(action);
    activeTraceChanged();
}

void MainWindow::closeTrace()
{
    activetraces.pop();
    Trace * trace = traces[activeTrace];
    traces.removeAt(activeTrace);
    ui->menuTraces->removeAction(ui->menuTraces->actions().at(activeTrace));
    delete trace;

    int index = -1;
    QString fallback = activetraces.pop();
    while (index < 0 && !activetraces.isEmpty())
    {
        for (int i = 0; i < traces.size(); i++)
        {
            if (fallback == traces[i]->name)
            {
                index = i;
            }
        }
        if (index < 0)
            fallback = activetraces.pop();
    }

    if (index >= 0)
    {
        activeTrace = index;
        activeTraceChanged();
    }
    else // We must have no traces left
    {
        ui->menuTraces->setEnabled(false);
    }
}

// Make splitters act together
void MainWindow::linkSideSplitter()
{
    QList<int> splitter_sizes = ui->splitter->sizes();
    ui->sideSplitter->setSizes(splitter_sizes);
}


void MainWindow::linkMainSplitter()
{
    QList<int> side_sizes = ui->sideSplitter->sizes();
    ui->splitter->setSizes(side_sizes);
}


void MainWindow::handleSplitter(int pos, int index)
{
    Q_UNUSED(pos);
    Q_UNUSED(index);
    linkSideSplitter();
    setVisWidgetState();
}

void MainWindow::handleSideSplitter(int pos, int index)
{
    Q_UNUSED(pos);
    Q_UNUSED(index);
    linkMainSplitter();
    setVisWidgetState();

}

// Synchronize menus and splitters
void MainWindow::setVisWidgetState()
{
    QList<int> sizes = ui->splitter->sizes();
    for (int i = 0; i < viswidgets.size() - 1; i++)
    {
        if (sizes[splitterMap[i]] == 0 && !viswidgets[i]->isClosed())
        {
            viswidgets[i]->setClosed(true);
            visactions[i]->setChecked(false);
        }
        else if (sizes[splitterMap[i]] != 0 && viswidgets[i]->isClosed())
        {
            viswidgets[i]->setClosed(false);
            visactions[i]->setChecked(true);
        }

        splitterActions[i]->setDisabled(true);
        if (viswidgets[i]->isClosed())
            splitterActions[i]->setChecked(false);
        else
            splitterActions[i]->setChecked(true);
        splitterActions[i]->setDisabled(false);
    }
}

void MainWindow::toggleLogicalSteps()
{
    QList<int> sizes = ui->splitter->sizes();
    if (ui->actionLogical_Steps->isChecked())
    {
        sizes[splitterMap[STEPVIS]] = this->height() / 3;
        viswidgets[STEPVIS]->setClosed(false);
    }
    else
    {
        sizes[splitterMap[STEPVIS]] = 0;
        viswidgets[STEPVIS]->setClosed(true);
    }
    ui->splitter->setSizes(sizes);
    linkSideSplitter();
    setVisWidgetState();
}

void MainWindow::toggleClusteredSteps()
{
    QList<int> sizes = ui->splitter->sizes();
    if (ui->actionClustered_Logical_Steps->isChecked())
    {
        sizes[splitterMap[CLUSTERVIS]] = this->height() / 3;
        viswidgets[CLUSTERVIS]->setClosed(false);
        viswidgets[CLUSTERTREEVIS]->setClosed(false);
    }
    else
    {
        sizes[splitterMap[CLUSTERVIS]] = 0;
        viswidgets[CLUSTERVIS]->setClosed(true);
        viswidgets[CLUSTERTREEVIS]->setClosed(true);
    }
    ui->splitter->setSizes(sizes);
    linkSideSplitter();
    setVisWidgetState();
}

void MainWindow::togglePhysicalTime()
{
    QList<int> sizes = ui->splitter->sizes();
    if (ui->actionPhysical_Time->isChecked())
    {
        sizes[splitterMap[TIMEVIS]] = this->height() / 3;
        viswidgets[TIMEVIS]->setClosed(false);
    }
    else
    {
        sizes[splitterMap[TIMEVIS]] = 0;
        viswidgets[TIMEVIS]->setClosed(true);
    }
    ui->splitter->setSizes(sizes);
    linkSideSplitter();
    setVisWidgetState();
}

void MainWindow::toggleMetricOverview()
{
    QList<int> sizes = ui->splitter->sizes();
    if (ui->actionMetric_Overview->isChecked())
    {
        sizes[splitterMap[OVERVIEW]] = 70;
        viswidgets[OVERVIEW]->setClosed(false);
    }
    else
    {
        sizes[splitterMap[OVERVIEW]] = 0;
        viswidgets[OVERVIEW]->setClosed(true);
    }
    ui->splitter->setSizes(sizes);
    linkSideSplitter();
    setVisWidgetState();
}
