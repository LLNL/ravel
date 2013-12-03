#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "overviewvis.h"
#include "stepvis.h"
#include "traditionalvis.h"
#include "otfconverter.h"
#include "importoptionsdialog.h"

#include <QFileDialog>
#include <iostream>
#include <string>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    traces(QVector<Trace *>()),
    viswidgets(QVector<VisWidget *>()),
    otfoptions(new OTFImportOptions()),
    otfdialog(NULL)
{
    ui->setupUi(this);

    OverviewVis* overview = new OverviewVis(ui->overviewContainer);
    //ui->overviewLayout->addWidget(overview);
    //ui->overviewLayout->setStretchFactor(overview, 1);
    ui->overviewContainer->layout()->addWidget(overview);

    connect(overview, SIGNAL(stepsChanged(float, float)), this, SLOT(pushSteps(float, float)));
    connect(overview, SIGNAL(eventClicked(Event *)), this, SLOT(selectEvent(Event *)));
    viswidgets.push_back(overview);

    StepVis* stepvis = new StepVis(ui->stepContainer);
    //ui->stepLayout->addWidget(stepvis);
    ui->stepContainer->layout()->addWidget(stepvis);

    connect((stepvis), SIGNAL(stepsChanged(float, float)), this, SLOT(pushSteps(float, float)));
    connect((stepvis), SIGNAL(eventClicked(Event *)), this, SLOT(selectEvent(Event *)));
    viswidgets.push_back(stepvis);

    TraditionalVis* timevis = new TraditionalVis(ui->traditionalContainer);
    //ui->traditionalLayout->addWidget(timevis);
    ui->traditionalContainer->layout()->addWidget(timevis);

    connect((timevis), SIGNAL(stepsChanged(float, float)), this, SLOT(pushSteps(float, float)));
    connect((timevis), SIGNAL(eventClicked(Event *)), this, SLOT(selectEvent(Event *)));
    viswidgets.push_back(timevis);

    connect(ui->actionOpen_JSON, SIGNAL(triggered()),this,SLOT(importJSON()));
    connect(ui->actionOpen_OTF, SIGNAL(triggered()),this,SLOT(importOTFbyGUI()));

    connect(ui->actionOTF_Importing, SIGNAL(triggered()), this, SLOT(launchOTFOptions()));


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

void MainWindow::pushSteps(float start, float stop)
{
    for(int i = 0; i < viswidgets.size(); i++)
    {
        viswidgets[i]->setSteps(start, stop);
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

void MainWindow::importOTFbyGUI()
{
    // Now get the OTF File
    QString dataFileName = QFileDialog::getOpenFileName(this, tr("Import OTF Data"),
                                                     "",
                                                     tr("Files (*.otf)"));
    // Guard against Cancel
    if (dataFileName == NULL || dataFileName.length() == 0)
        return;

    importOTF(dataFileName);
}



void MainWindow::importOTF(QString dataFileName){

    OTFConverter importer = OTFConverter();
    Trace* trace = importer.importOTF(dataFileName, otfoptions);
    trace->preprocess(otfoptions);

    this->traces.push_back(trace);

    for(int i = 0; i < viswidgets.size(); i++)
    {
        viswidgets[i]->setTrace(trace);
        viswidgets[i]->processVis();
        viswidgets[i]->repaint();
    }

}

void MainWindow::importJSON()
{
    QString dataFileName = QFileDialog::getOpenFileName(this, tr("Import JSON Data"),
                                                     "",
                                                     tr("Files (*.json)"));

    // Guard against Cancel
    if (dataFileName == NULL || dataFileName.length() == 0)
        return;

    QFile dataFile(dataFileName);

    QFile file(dataFileName);
     if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
         return;

    QString content = file.readAll();
    //dataFile.close();

    Json::Value root;
    Json::Reader reader;
    bool parsingSuccessful = reader.parse(content.toStdString(), root);
    if (!parsingSuccessful)
    {
        std::cout << "Failed to parse JSON\n" << reader.getFormatedErrorMessages().c_str();
        return;
    }

    Trace* trace = new Trace(root["nodes"].asInt(), true);
    Json::Value fxns = root["fxns"];
    for (Json::ValueIterator itr = fxns.begin(); itr != fxns.end(); ++itr) {
        int key = QString(itr.key().asString().c_str()).toInt();
        (*(trace->functions))[key] = new Function(QString((*itr)["name"].asString().c_str()), (*itr)["type"].asInt());
    }

    Json::Value fxnGroups = root["types"];
    for (Json::ValueIterator itr = fxnGroups.begin(); itr != fxnGroups.end(); ++itr)
    {
        int key = QString(itr.key().asString().c_str()).toInt();
        (*(trace->functionGroups))[key] = QString((*itr).asString().c_str());
    }

    QMap<int, Event*> eventmap; // map ID to event
    Json::Value events = root["events"];
    // Create a single partition since this is all the information we have
    Partition * partition = new Partition();
    partition->min_global_step = 0;
    trace->partitions->append(partition);
    int max_step = 0;

    // First pass to create events
    for (Json::ValueIterator itr = events.begin(); itr != events.end(); ++itr) {
        int key = QString(itr.key().asString().c_str()).toInt();
        Event* e = new Event(static_cast<unsigned long long>((*itr)["entertime"].asDouble()),
                static_cast<unsigned long long>((*itr)["leavetime"].asDouble()),
                (*itr)["fxn"].asInt(), (*itr)["process"].asInt() - 1, (*itr)["step"].asInt());
        e->addMetric("Lateness", static_cast<long long>((*itr)["lateness"].asDouble()),
                static_cast<long long>((*itr)["comp_lateness"].asDouble()));
        eventmap[key] = e;
        if (e->step > max_step)
            max_step = e->step;
        if (e->step >= 0)
        {
            if (!(partition->events->contains(e->process)))
                (*(partition->events))[e->process] = new QList<Event *>();
            (*(partition->events))[e->process]->push_back(e);
        }
        (*(trace->events))[e->process]->push_back(e);
    }
    partition->max_global_step = max_step;
    trace->global_max_step = max_step;
    trace->dag_entries = new QList<Partition *>();
    trace->dag_entries->append(partition);

    // Second pass to link parents and children
    for (Json::ValueIterator itr = events.begin(); itr != events.end(); itr++) {
        int key = QString(itr.key().asString().c_str()).toInt();
        Event* e = eventmap[key];
        Json::Value parents = (*itr)["parents"];
        for (unsigned int i = 0; i < parents.size(); ++i) {
            e->parents->push_back(eventmap[parents[i].asInt()]);
        }
        Json::Value children = (*itr)["children"];
        for (unsigned int i = 0; i < children.size(); ++i) {
            e->children->push_back(eventmap[children[i].asInt()]);
        }
    }

    Json::Value messages = root["comms"];
    for (Json::ValueIterator itr = messages.begin(); itr != messages.end(); itr++) {
        Message* m = new Message(static_cast<unsigned long long>((*itr)["sendtime"].asDouble()),
                static_cast<unsigned long long>((*itr)["recvtime"].asDouble()));
        m->sender = eventmap[(*itr)["senderid"].asInt()];
        m->receiver = eventmap[(*itr)["receiverid"].asInt()];
        m->sender->messages->push_back(m);
        m->receiver->messages->push_back(m);
    }


    this->traces.push_back(trace);
    dataFile.close();

    for(int i = 0; i < viswidgets.size(); i++)
    {
        viswidgets[i]->setTrace(trace);
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
