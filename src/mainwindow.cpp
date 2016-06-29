//////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2014, Lawrence Livermore National Security, LLC.
// Produced at the Lawrence Livermore National Laboratory.
//
// This file is part of Ravel.
// Written by Kate Isaacs, kisaacs@acm.org, All rights reserved.
// LLNL-CODE-663885
//
// For details, see https://github.com/scalability-llnl/ravel
// Please also see the LICENSE file for our notice and the LGPL.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License (as published by
// the Free Software Foundation) version 2.1 dated February 1999.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the IMPLIED WARRANTY OF
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the terms and
// conditions of the GNU General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program; if not, write to the Free Software Foundation,
// Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
//////////////////////////////////////////////////////////////////////////////
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "overviewvis.h"
#include "traditionalvis.h"
#include "otfconverter.h"
#include "ravelutils.h"
#include "verticallabel.h"
#include "viswidget.h"
#include "trace.h"
#include "visoptions.h"
#include "visoptionsdialog.h"
#include "importfunctor.h"

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
    visoptions(new VisOptions()),
    visdialog(NULL),
    activetracename(""),
    activetraces(QStack<QString>()),
    dataDirectory("")
{
    readSettings();
    ui->setupUi(this);

    // Overview
    OverviewVis* overview = new OverviewVis(ui->overviewContainer, visoptions);
    ui->overviewContainer->layout()->addWidget(overview);
    ui->overviewLabelWidget->setLayout(new QVBoxLayout());
    ui->overviewLabelWidget->layout()->addWidget(new VerticalLabel("Overview",
                                                                   ui->overviewLabelWidget));

    connect(overview, SIGNAL(timeChanged(float, float, bool)), this,
            SLOT(pushTime(float, float, bool)));
    viswidgets.push_back(overview);
    splitterMap.push_back(1);
    splitterActions.push_back(ui->actionMetric_Overview);

    // Physical Timeline
    TraditionalVis* timevis = new TraditionalVis(ui->traditionalContainer,
                                                 visoptions);
    timevis->setClosed(true);
    ui->traditionalContainer->layout()->addWidget(timevis);
    ui->traditionalLabelWidget->layout()->addWidget(new VerticalLabel("Physical",
                                                                      ui->traditionalLabelWidget));
    QLabel * timescaleLabel = new QLabel(ui->traditionalLabelWidget);
    timescaleLabel->setFont(QFont("Helvetica", 10));
    ui->traditionalLabelWidget->layout()->addWidget(timescaleLabel);
    timescaleLabel->setAlignment(Qt::AlignRight | Qt::AlignBottom);

    connect((timevis), SIGNAL(timeChanged(float, float, bool)), this,
            SLOT(pushTime(float, float, bool)));
    connect((timevis), SIGNAL(eventClicked(Event *)), this,
            SLOT(selectEvent(Event *)));
    connect((timevis), SIGNAL(timeScaleString(QString)), timescaleLabel,
            SLOT(setText(QString)));
    viswidgets.push_back(timevis);
    splitterMap.push_back(0);
    splitterActions.push_back(ui->actionPhysical_Time);


    // Menu
    connect(ui->actionOpen_Trace, SIGNAL(triggered()),this,
            SLOT(importTracebyGUI()));
    ui->actionOpen_Trace->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_O));

    connect(ui->actionClose, SIGNAL(triggered()), this,
            SLOT(closeTrace()));
    ui->actionClose->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_C));
    ui->actionClose->setEnabled(false);

    connect(ui->actionVisualization, SIGNAL(triggered()), this,
            SLOT(launchVisOptions()));
    ui->actionVisualization->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_V));

    connect(ui->actionPhysical_Time, SIGNAL(triggered()), this,
            SLOT(togglePhysicalTime()));
    ui->actionPhysical_Time->setShortcut(QKeySequence(Qt::ALT + Qt::Key_P));
    connect(ui->actionMetric_Overview, SIGNAL(triggered()), this,
            SLOT(toggleMetricOverview()));
    ui->actionMetric_Overview->setShortcut(QKeySequence(Qt::ALT + Qt::Key_M));
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
    sizes.append(70);
    ui->splitter->setSizes(sizes);
    ui->sideSplitter->setSizes(sizes);

}

MainWindow::~MainWindow()
{
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
void MainWindow::pushTime(float start, float stop, bool jump)
{
    for(int i = 0; i < viswidgets.size(); i++)
    {
        viswidgets[i]->setTime(start, stop, jump);
    }
}

void MainWindow::selectEvent(Event * event)
{
    for(int i = 0; i < viswidgets.size(); i++)
    {
        viswidgets[i]->selectEvent(event);
    }
}

void MainWindow::selectEntities(QList<int> entities)
{
    for(int i = 0; i < viswidgets.size(); i++)
    {
        viswidgets[i]->selectEntities(entities);
    }
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

void MainWindow::importTracebyGUI()
{
    // Now get the Trace File
    QString dataFileName = "";

    QString fileTypes = "Trace Files (*.otf2 *.sts)";
#ifdef OTF1LIB
    fileTypes = "Trace Files (*.otf2 *.sts *.otf)";
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
    importWorker = new ImportFunctor();
    importWorker->moveToThread(importThread);

    if (dataFileName.endsWith("otf", Qt::CaseInsensitive))
    {
        connect(this, SIGNAL(operate(QString)), importWorker,
                SLOT(doImportOTF(QString)));
    }
    else if (dataFileName.endsWith("otf2", Qt::CaseInsensitive))
    {
        connect(this, SIGNAL(operate(QString)), importWorker,
                SLOT(doImportOTF2(QString)));
    }
    else
    {
        std::cout << "Unrecognized trace format!" << std::endl;
        progress->close();
        delete progress;
        delete importThread;
        delete importWorker;
    }

    connect(importWorker, SIGNAL(done(Trace *)), this,
            SLOT(traceFinished(Trace *)));
    connect(importWorker, SIGNAL(reportProgress(int, QString)), this,
            SLOT(updateProgress(int, QString)));

    importThread->start();
    emit(operate(dataFileName));
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
    if (!traces[activeTrace]->metrics->contains(visoptions->metric))
    {
        visoptions->metric = "None";
        if (traces[activeTrace]->metrics->size() > 0)
            visoptions->metric = traces[activeTrace]->metrics->at(0);
    }

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
        splitter_sizes[splitterMap[TIMEVIS]] = this->height() - 70;
        viswidgets[TIMEVIS]->setClosed(false);

        ui->splitter->setSizes(splitter_sizes);
        ui->sideSplitter->setSizes(splitter_sizes);
        setVisWidgetState();
    }
    ui->actionClose->setEnabled(true);
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
    if (activetraces.size() > 0)
    {
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
    }

    if (index >= 0)
    {
        activeTrace = index;
        activeTraceChanged();
    }
    else // We must have no traces left
    {
        ui->menuTraces->setEnabled(false);
        ui->actionClose->setEnabled(false);
        for(int i = 0; i < viswidgets.size(); i++)
        {
            viswidgets[i]->clear();
        }
        activeTrace = -1;
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

void MainWindow::togglePhysicalTime()
{
    QList<int> sizes = ui->splitter->sizes();
    if (ui->actionPhysical_Time->isChecked())
    {
        sizes[splitterMap[TIMEVIS]] = this->height();
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
