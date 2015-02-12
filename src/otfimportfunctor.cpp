//////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2014, Lawrence Livermore National Security, LLC.
// Produced at the Lawrence Livermore National Laboratory.
//
// This file is part of Ravel.
// Written by Kate Isaacs, kisaacs@acm.org, All rights reserved.
// LLNL-CODE-663885
//
// For details, see https://scalability-llnl.github.io/ravel
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
#include "otfimportfunctor.h"
#include "general_util.h"
#include <QElapsedTimer>

#include "trace.h"
#include "otfconverter.h"
#include "otfimportoptions.h"
#include "otf2importer.h"

OTFImportFunctor::OTFImportFunctor(OTFImportOptions * _options)
    : options(_options),
      trace(NULL)
{
}

void OTFImportFunctor::doImportOTF2(QString dataFileName)
{
    std::cout << "Processing " << dataFileName.toStdString().c_str() << std::endl;
    QElapsedTimer traceTimer;
    qint64 traceElapsed;

    traceTimer.start();

    OTFConverter * importer = new OTFConverter();
    connect(importer, SIGNAL(finishRead()), this, SLOT(finishInitialRead()));
    connect(importer, SIGNAL(matchingUpdate(int, QString)), this,
            SLOT(updateMatching(int, QString)));
    Trace* trace = importer->importOTF2(dataFileName, options);
    delete importer;
    connect(trace, SIGNAL(updatePreprocess(int, QString)), this,
            SLOT(updatePreprocess(int, QString)));
    connect(trace, SIGNAL(updateClustering(int)), this,
            SLOT(updateClustering(int)));
    connect(trace, SIGNAL(startClustering()), this, SLOT(switchProgress()));
    if (trace->options.origin == OTFImportOptions::OF_SAVE_OTF2)
        trace->preprocessFromSaved();
    else
        trace->preprocess(options);

    traceElapsed = traceTimer.nsecsElapsed();
    std::cout << "Total trace: ";
    gu_printTime(traceElapsed);
    std::cout << std::endl;

    emit(done(trace));
}

void OTFImportFunctor::doImportOTF(QString dataFileName)
{
    std::cout << "Processing " << dataFileName.toStdString().c_str() << std::endl;
    QElapsedTimer traceTimer;
    qint64 traceElapsed;

    traceTimer.start();

    OTFConverter * importer = new OTFConverter();
    connect(importer, SIGNAL(finishRead()), this, SLOT(finishInitialRead()));
    connect(importer, SIGNAL(matchingUpdate(int, QString)), this,
            SLOT(updateMatching(int, QString)));
    Trace* trace = importer->importOTF(dataFileName, options);
    delete importer;
    connect(trace, SIGNAL(updatePreprocess(int, QString)), this,
            SLOT(updatePreprocess(int, QString)));
    connect(trace, SIGNAL(updateClustering(int)), this,
            SLOT(updateClustering(int)));
    connect(trace, SIGNAL(startClustering()), this, SLOT(switchProgress()));
    trace->preprocess(options);

    traceElapsed = traceTimer.nsecsElapsed();
    std::cout << "Total trace: ";
    gu_printTime(traceElapsed);
    std::cout << std::endl;

    emit(done(trace));
}

void OTFImportFunctor::finishInitialRead()
{
    emit(reportProgress(25, "Constructing events..."));
}

void OTFImportFunctor::updateMatching(int portion, QString msg)
{
    emit(reportProgress(25 + portion, msg));
}

void OTFImportFunctor::updatePreprocess(int portion, QString msg)
{
    emit(reportProgress(50 + portion / 2.0, msg));
}

void OTFImportFunctor::updateClustering(int portion)
{
    emit(reportClusterProgress(portion, "Clustering..."));
}

void OTFImportFunctor::switchProgress()
{
    emit(switching());
}
