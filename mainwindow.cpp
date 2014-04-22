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

#include <QFileDialog>
#include "qtconcurrentrun.h"
#include <iostream>
#include <string>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    traces(QVector<Trace *>()),
    viswidgets(QVector<VisWidget *>()),
    visactions(QList<QAction *>()),
    activeTrace(-1),
    importWorker(NULL),
    importThread(NULL),
    progress(NULL),
    otfoptions(new OTFImportOptions()),
    otfdialog(NULL),
    visoptions(new VisOptions()),
    visdialog(NULL)
{
    ui->setupUi(this);

    // Overview
    OverviewVis* overview = new OverviewVis(ui->overviewContainer, visoptions);
    ui->overviewContainer->layout()->addWidget(overview);
    ui->overviewLabelWidget->setLayout(new QVBoxLayout());
    ui->overviewLabelWidget->layout()->addWidget(new VerticalLabel("Overview", ui->overviewLabelWidget));

    connect(overview, SIGNAL(stepsChanged(float, float, bool)), this, SLOT(pushSteps(float, float, bool)));
    viswidgets.push_back(overview);

    // Logical Timeline
    StepVis* stepvis = new StepVis(ui->stepContainer, visoptions);
    ui->stepContainer->layout()->addWidget(stepvis);
    ui->logicalLabelWidget->setLayout(new QVBoxLayout());
    ui->logicalLabelWidget->layout()->addWidget(new VerticalLabel("Logical", ui->logicalLabelWidget));

    connect((stepvis), SIGNAL(stepsChanged(float, float, bool)), this, SLOT(pushSteps(float, float, bool)));
    connect((stepvis), SIGNAL(eventClicked(Event *, bool)), this, SLOT(selectEvent(Event *, bool)));
    viswidgets.push_back(stepvis);

    // Physical Timeline
    TraditionalVis* timevis = new TraditionalVis(ui->traditionalContainer, visoptions);
    timevis->setClosed(true);
    ui->traditionalContainer->layout()->addWidget(timevis);
    ui->traditionalLabelWidget->setLayout(new QVBoxLayout());
    ui->traditionalLabelWidget->layout()->addWidget(new VerticalLabel("Physical", ui->traditionalLabelWidget));

    connect((timevis), SIGNAL(stepsChanged(float, float, bool)), this, SLOT(pushSteps(float, float, bool)));
    connect((timevis), SIGNAL(eventClicked(Event *, bool)), this, SLOT(selectEvent(Event *, bool)));
    viswidgets.push_back(timevis);

    // Cluster View
    ClusterTreeVis* clustertreevis = new ClusterTreeVis(ui->stepContainer, visoptions);
    ui->treeContainer->layout()->addWidget(clustertreevis);

    ClusterVis* clustervis = new ClusterVis(clustertreevis, ui->stepContainer, visoptions);
    ui->clusterContainer->layout()->addWidget(clustervis);
    ui->clusterLabelWidget->setLayout(new QVBoxLayout());
    ui->clusterLabelWidget->layout()->addWidget(new VerticalLabel("Clusters", ui->clusterLabelWidget));

    connect((clustervis), SIGNAL(stepsChanged(float, float, bool)), this, SLOT(pushSteps(float, float, bool)));
    connect((clustervis), SIGNAL(eventClicked(Event *, bool)), this, SLOT(selectEvent(Event *, bool)));
    connect((clustervis), SIGNAL(processesSelected(QList<int>, Gnome*)), this, SLOT(selectProcesses(QList<int>, Gnome*)));

    connect((clustervis), SIGNAL(focusGnome()), clustertreevis, SLOT(repaint()));
    connect((clustervis), SIGNAL(clusterChange()), clustertreevis, SLOT(clusterChanged()));
    connect((clustertreevis), SIGNAL(clusterChange()), clustervis, SLOT(clusterChanged()));
    connect((clustervis), SIGNAL(neighborChange(int)), clustertreevis, SLOT(repaint()));

    viswidgets.push_back(clustervis);
    viswidgets.push_back(clustertreevis);

    // Sliders
    connect(ui->verticalSlider, SIGNAL(valueChanged(int)), clustervis, SLOT(changeNeighborRadius(int)));
    connect((clustervis), SIGNAL(neighborChange(int)), ui->verticalSlider, SLOT(setValue(int)));

    // Menus
    connect(ui->actionOpen_OTF, SIGNAL(triggered()),this,SLOT(importOTFbyGUI()));
    ui->actionOpen_OTF->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_O));

    connect(ui->actionOTF_Importing, SIGNAL(triggered()), this, SLOT(launchOTFOptions()));
    ui->actionOTF_Importing->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_I));
    connect(ui->actionVisualization, SIGNAL(triggered()), this, SLOT(launchVisOptions()));
    ui->actionVisualization->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_V));


    connect(ui->actionLogical_Steps, SIGNAL(triggered()), this, SLOT(toggleLogicalSteps()));
    ui->actionLogical_Steps->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_L));
    connect(ui->actionClustered_Logical_Steps, SIGNAL(triggered()), this, SLOT(toggleClusteredSteps()));
    ui->actionClustered_Logical_Steps->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_C));
    connect(ui->actionPhysical_Time, SIGNAL(triggered()), this, SLOT(togglePhysicalTime()));
    ui->actionPhysical_Time->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_P));
    connect(ui->actionMetric_Overview, SIGNAL(triggered()), this, SLOT(toggleMetricOverview()));
    ui->actionMetric_Overview->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_M));
    visactions.append(ui->actionLogical_Steps);
    visactions.append(ui->actionClustered_Logical_Steps);
    visactions.append(ui->actionPhysical_Time);
    visactions.append(ui->actionMetric_Overview);

    connect(ui->actionQuit, SIGNAL(triggered()), this, SLOT(close()));
    ui->actionQuit->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Q));

    // Splitters
    connect(ui->splitter, SIGNAL(splitterMoved(int, int)), this, SLOT(handleSplitter(int, int)));
    connect(ui->sideSplitter, SIGNAL(splitterMoved(int, int)), this, SLOT(handleSideSplitter(int, int)));
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

// The following functions relay a signal from one vis (hooked up in the constructor)
// to all of the rest
void MainWindow::pushSteps(float start, float stop, bool jump)
{
    for(int i = 0; i < viswidgets.size(); i++)
    {
        viswidgets[i]->setSteps(start, stop, jump);
    }
}

void MainWindow::selectEvent(Event * event, bool aggregate)
{
    for(int i = 0; i < viswidgets.size(); i++)
    {
        viswidgets[i]->selectEvent(event, aggregate);
    }
}

void MainWindow::selectProcesses(QList<int> processes, Gnome * gnome)
{
    for(int i = 0; i < viswidgets.size(); i++)
    {
        viswidgets[i]->selectProcesses(processes, gnome);
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
        visdialog = new VisOptionsDialog(this, visoptions, traces[activeTrace]);
    else
        visdialog = new VisOptionsDialog(this, visoptions);
    visdialog->show();

    for(int i = 0; i < viswidgets.size(); i++)
        viswidgets[i]->repaint();
}

void MainWindow::importOTFbyGUI()
{
    // Now get the OTF File
    QString dataFileName = QFileDialog::getOpenFileName(this, tr("Import OTF Data"),
                                                        "",
                                                        tr("Files (*.otf)")); /*,
                                                        0,
                                                        QFileDialog::DontUseNativeDialog);*/
    qApp->processEvents();

    // Guard against Cancel
    if (dataFileName == NULL || dataFileName.length() == 0)
        return;

    repaint();
    importOTF(dataFileName);
}

void MainWindow::importOTF(QString dataFileName){

    progress = new QProgressDialog("Reading OTF...", "", 0, 0, this);
    progress->setWindowTitle("Importing OTF...");
    progress->setCancelButton(0);
    progress->show();

    importThread = new QThread();
    importWorker = new OTFImportFunctor(otfoptions);
    importWorker->moveToThread(importThread);
    connect(this, SIGNAL(operate(QString)), importWorker, SLOT(doImport(QString)));
    connect(importWorker, SIGNAL(switching()), this, SLOT(traceSwitch()));
    connect(importWorker, SIGNAL(done(Trace *)), this, SLOT(traceFinished(Trace *)));
    connect(importWorker, SIGNAL(reportProgress(int, QString)), this, SLOT(updateProgress(int, QString)));

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

    this->traces.push_back(trace);
    activeTrace = traces.size() - 1;

    delete importWorker;
    delete progress;
    delete importThread;

    activeTraceChanged();
}

void MainWindow::updateProgress(int portion, QString msg)
{
    progress->setMaximum(100);
    progress->setLabelText(msg);
    progress->setValue(portion);
}

// In the future we may want to reset splitters to a save state
// rather than a default... or not reset splitters at all
void MainWindow::activeTraceChanged()
{
    for(int i = 0; i < viswidgets.size(); i++)
    {
        viswidgets[i]->setTrace(traces[activeTrace]);
        viswidgets[i]->processVis();
        viswidgets[i]->repaint();
    }
    QList<int> splitter_sizes = ui->splitter->sizes();
    if (otfoptions->cluster)
    {
        int traditional_height = splitter_sizes[2];
        splitter_sizes[0] += traditional_height / 2;
        splitter_sizes[1] += traditional_height / 2;
        splitter_sizes[2] = 0;
    }
    else
    {
        splitter_sizes[0] += splitter_sizes[1] + splitter_sizes[2];
        splitter_sizes[1] = 0;
        splitter_sizes[2] = 0;
    }
    ui->splitter->setSizes(splitter_sizes);
    ui->sideSplitter->setSizes(splitter_sizes);
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
    for (int i = 0; i < viswidgets.size() - 1; i++)
    {
        if (viswidgets[i]->container->height() == 0 && !viswidgets[i]->isClosed())
        {
            viswidgets[i]->setClosed(true);
            visactions[i]->setChecked(false);
        }
        else if (viswidgets[i]->container->height() != 0 && viswidgets[i]->isClosed())
        {
            viswidgets[i]->setClosed(false);
            visactions[i]->setChecked(true);
        }
    }
}

void MainWindow::toggleLogicalSteps()
{
    QList<int> sizes = ui->splitter->sizes();
    if (ui->actionLogical_Steps->isChecked())
    {
        sizes[0] = this->height() / 3;
        viswidgets[1]->setClosed(false);
    }
    else
    {
        sizes[0] = 0;
        viswidgets[1]->setClosed(true);
    }
    ui->splitter->setSizes(sizes);
    linkSideSplitter();
}

void MainWindow::toggleClusteredSteps()
{
    QList<int> sizes = ui->splitter->sizes();
    if (ui->actionClustered_Logical_Steps->isChecked())
    {
        sizes[1] = this->height() / 3;
        viswidgets[3]->setClosed(false);
        viswidgets[4]->setClosed(false);
    }
    else
    {
        sizes[1] = 0;
        viswidgets[3]->setClosed(true);
        viswidgets[4]->setClosed(true);
    }
    ui->splitter->setSizes(sizes);
    linkSideSplitter();
}

void MainWindow::togglePhysicalTime()
{
    QList<int> sizes = ui->splitter->sizes();
    if (ui->actionPhysical_Time->isChecked())
    {
        sizes[2] = this->height() / 3;
        viswidgets[2]->setClosed(false);
    }
    else
    {
        sizes[2] = 0;
        viswidgets[2]->setClosed(true);
    }
    ui->splitter->setSizes(sizes);
    linkSideSplitter();
}

void MainWindow::toggleMetricOverview()
{
    QList<int> sizes = ui->splitter->sizes();
    if (ui->actionMetric_Overview->isChecked())
    {
        sizes[3] = 70;
        viswidgets[0]->setClosed(false);
    }
    else
    {
        sizes[3] = 0;
        viswidgets[0]->setClosed(true);
    }
    ui->splitter->setSizes(sizes);
    linkSideSplitter();
}
