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
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QString>
#include <QStack>
#include <QList>
#include <QVector>

class Gnome;
class Event;
class Trace;
class ImportOptions;
class ImportOptionsDialog;
class VisWidget;
class VisOptions;
class VisOptionsDialog;

class QAction;
class ImportFunctor;
class OTF2ExportFunctor;
class QProgressDialog;
class QThread;
class QWidget;
class QCloseEvent;

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

    void closeEvent(QCloseEvent *);

public slots:
    void launchImportOptions();
    void launchVisOptions();

    // Signal relays
    void pushSteps(float start, float stop, bool jump = false);
    void selectEvent(Event * event, bool aggregate, bool overdraw);
    void selectEntities(QList<int> entities, Gnome *gnome);

    // Importing & Progress Bar
    void importTracebyGUI();
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

    // Saving
    void saveCurrentTrace();
    void exportFinished();

    // Settings
    void writeSettings();
    void readSettings();

signals:
    void operate(const QString &);
    void exportTrace(Trace *, const QString&, const QString&);
    
private:
    Ui::MainWindow *ui;
    void importTrace(QString dataFileName);
    void activeTraceChanged(bool first = false);
    void linkSideSplitter();
    void linkMainSplitter();
    void setVisWidgetState();

    // Saving traces & vis
    QList<Trace *> traces;
    QVector<VisWidget *> viswidgets;
    QList<QAction *> visactions;
    QVector<int> splitterMap;
    QVector<QAction *> splitterActions;
    int activeTrace;

    // For progress bar
    ImportFunctor * importWorker;
    QThread * importThread;
    QProgressDialog * progress;
    OTF2ExportFunctor * exportWorker;
    QThread * exportThread;

    // Import Trace options
    ImportOptions * importoptions;
    ImportOptionsDialog * importdialog;

    // Color stuff & other vis options
    VisOptions * visoptions;
    VisOptionsDialog * visdialog;

    QString activetracename;

    QStack<QString> activetraces;
    QString dataDirectory;
    bool otf1Support;

    static const int OVERVIEW = 0;
    static const int STEPVIS = 1;
    static const int TIMEVIS = 2;
    static const int CLUSTERVIS = 3;
    static const int CLUSTERTREEVIS = 4;
};

#endif // MAINWINDOW_H
