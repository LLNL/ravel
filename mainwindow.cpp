#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "overviewvis.h"
#include "stepvis.h"
#include "traditionalvis.h"
#include "otfconverter.h"
#include "importoptionsdialog.h"
#include "general_util.h"

#include <QFileDialog>
#include "qtconcurrentrun.h"
#include <iostream>
#include <string>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    traces(QVector<Trace *>()),
    viswidgets(QVector<VisWidget *>()),
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

    OverviewVis* overview = new OverviewVis(ui->overviewContainer, visoptions);
    //ui->overviewLayout->addWidget(overview);
    //ui->overviewLayout->setStretchFactor(overview, 1);
    ui->overviewContainer->layout()->addWidget(overview);

    connect(overview, SIGNAL(stepsChanged(float, float, bool)), this, SLOT(pushSteps(float, float, bool)));
    connect(overview, SIGNAL(eventClicked(Event *)), this, SLOT(selectEvent(Event *)));
    viswidgets.push_back(overview);

    StepVis* stepvis = new StepVis(ui->stepContainer, visoptions);
    //ui->stepLayout->addWidget(stepvis);
    ui->stepContainer->layout()->addWidget(stepvis);

    connect((stepvis), SIGNAL(stepsChanged(float, float, bool)), this, SLOT(pushSteps(float, float, bool)));
    connect((stepvis), SIGNAL(eventClicked(Event *)), this, SLOT(selectEvent(Event *)));
    viswidgets.push_back(stepvis);

    TraditionalVis* timevis = new TraditionalVis(ui->traditionalContainer, visoptions);
    //ui->traditionalLayout->addWidget(timevis);
    ui->traditionalContainer->layout()->addWidget(timevis);

    connect((timevis), SIGNAL(stepsChanged(float, float, bool)), this, SLOT(pushSteps(float, float, bool)));
    connect((timevis), SIGNAL(eventClicked(Event *)), this, SLOT(selectEvent(Event *)));
    viswidgets.push_back(timevis);

    connect(ui->actionOpen_OTF, SIGNAL(triggered()),this,SLOT(importOTFbyGUI()));

    connect(ui->actionOTF_Importing, SIGNAL(triggered()), this, SLOT(launchOTFOptions()));
    connect(ui->actionVisualization, SIGNAL(triggered()), this, SLOT(launchVisOptions()));


    connect(ui->splitter, SIGNAL(splitterMoved(int, int)), this, SLOT(handleSplitter(int, int)));
    // for testing
    //importOTF("/Users/kate/Documents/trace_files/sdissbinom16/nbc-test.otf");
    //importOTF("/home/kate/llnl/traces/trace_files/data/sdissbinom16/nbc-test.otf");
}

MainWindow::~MainWindow()
{
    delete otfdialog;
    delete ui;
}

void MainWindow::pushSteps(float start, float stop, bool jump)
{
    for(int i = 0; i < viswidgets.size(); i++)
    {
        viswidgets[i]->setSteps(start, stop, jump);
    }
}

void MainWindow::selectEvent(Event * event)
{
    for(int i = 0; i < viswidgets.size(); i++)
    {
        viswidgets[i]->selectEvent(event);
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

    delete importThread;
    importThread = new QThread();
    importWorker = new OTFImportFunctor(otfoptions);
    importWorker->moveToThread(importThread);
    connect(this, SIGNAL(operate(QString)), importWorker, SLOT(doImport(QString)));
    connect(importWorker, SIGNAL(done(Trace *)), this, SLOT(traceFinished(Trace *)));
    connect(importWorker, SIGNAL(reportProgress(int, QString)), this, SLOT(updateProgress(int, QString)));

    importThread->start();
    emit(operate(dataFileName));
}

void MainWindow::traceFinished(Trace * trace)
{
    progress->close();

    this->traces.push_back(trace);
    activeTrace = traces.size() - 1;

    delete importWorker;
    delete progress;

    activeTraceChanged();
}

void MainWindow::updateProgress(int portion, QString msg)
{
    progress->setMaximum(100);
    progress->setLabelText(msg);
    progress->setValue(portion);
}

void MainWindow::activeTraceChanged()
{
    for(int i = 0; i < viswidgets.size(); i++)
    {
        viswidgets[i]->setTrace(traces[activeTrace]);
        viswidgets[i]->processVis();
        viswidgets[i]->repaint();
    }

}

void MainWindow::handleSplitter(int pos, int index)
{
    Q_UNUSED(pos);
    Q_UNUSED(index);
    for (int i = 0; i < viswidgets.size(); i++)
    {
        if (viswidgets[i]->container->height() == 0)
            viswidgets[i]->setClosed(true);
        else
            viswidgets[i]->setClosed(false);
    }
}
