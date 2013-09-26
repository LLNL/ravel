#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QFileDialog>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

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

    std::cout << "Has root" << std::endl;
    std::cout << root["nodes"] << std::endl;
    Trace* trace = new Trace(root["nodes"].asInt());
    Json::Value fxns = root["fxns"];
    for (Json::ValueIterator itr = fxns.begin() ; itr != fxns.end(); itr++) {
        int key = QString(itr.key().asString().c_str()).toInt();
        (*(trace->functions))[key] = QString((*itr)["name"].asString().c_str());
    }

    this->traces.push_back(trace);
    dataFile.close();
}
