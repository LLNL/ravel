#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "overviewvis.h"

#include <QFileDialog>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    OverviewVis* overview = new OverviewVis();
    ui->overviewLayout->addWidget(overview);

    viswidgets.push_back(overview);

    connect(ui->actionOpen_JSON, SIGNAL(triggered()),this,SLOT(importJSON()));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::importJSON()
{
    QString dataFileName = QFileDialog::getOpenFileName(this, tr("Import JSON Data"),
                                                     "",
                                                     tr("Files (*.*)"));
    QFile dataFile(dataFileName);

    QFile file(dataFileName);
     if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
         return;

    QString content = file.readAll();
    //dataFile.close();

    Json::Value root;
    Json::Reader reader;
    std::cout << "pre-parse" << std::endl;
    bool parsingSuccessful = reader.parse(content.toStdString(), root);
    if (!parsingSuccessful)
    {
        std::cout << "Failed to parse JSON\n" << reader.getFormatedErrorMessages();
        return;
    }

    Trace* trace = new Trace(root["nodes"].asInt());
    Json::Value fxns = root["fxns"];
    for (Json::ValueIterator itr = fxns.begin(); itr != fxns.end(); itr++) {
        int key = QString(itr.key().asString().c_str()).toInt();
        (*(trace->functions))[key] = QString((*itr)["name"].asString().c_str());
    }

    QMap<int, Event*> eventmap;
    Json::Value events = root["events"];
    // First pass to create events
    for (Json::ValueIterator itr = events.begin(); itr != events.end(); itr++) {
        int key = QString(itr.key().asString().c_str()).toInt();
        Event* e = new Event(static_cast<unsigned long long>((*itr)["entertime"].asDouble()),
                static_cast<unsigned long long>((*itr)["leavetime"].asDouble()),
                (*itr)["fxn"].asInt(), (*itr)["process"].asInt(), (*itr)["step"].asInt(),
                static_cast<long long>((*itr)["lateness"].asDouble()));
        eventmap[key] = e;
        trace->events->push_back(e);
    }
    // Second pass to link parents and children
    for (Json::ValueIterator itr = events.begin(); itr != events.end(); itr++) {
        int key = QString(itr.key().asString().c_str()).toInt();
        Event* e = eventmap[key];
        Json::Value parents = (*itr)["parents"];
        for (int i = 0; i < parents.size(); ++i) {
            e->parents->push_back(eventmap[parents[i].asInt()]);
        }
        Json::Value children = (*itr)["children"];
        for (int i = 0; i < children.size(); ++i) {
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
    std::cout << "Num events: " << trace->events->size() << std::endl;

    for(int i = 0; i < viswidgets.size(); i++)
    {
        viswidgets[i]->setTrace(trace);
        viswidgets[i]->processVis();
    }
}
