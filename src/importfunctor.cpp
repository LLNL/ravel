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
#include "importfunctor.h"
#include "ravelutils.h"
#include <QElapsedTimer>

#include "trace.h"
#include "otfconverter.h"
#include "otf2importer.h"

ImportFunctor::ImportFunctor()
    : trace(NULL)
{
}

void ImportFunctor::doImportOTF2(QString dataFileName)
{
    std::cout << "Processing " << dataFileName.toStdString().c_str() << std::endl;
    QElapsedTimer traceTimer;
    qint64 traceElapsed;

    traceTimer.start();

    OTFConverter * importer = new OTFConverter();
    connect(importer, SIGNAL(finishRead()), this, SLOT(finishInitialRead()));
    connect(importer, SIGNAL(matchingUpdate(int, QString)), this,
            SLOT(updateMatching(int, QString)));
    Trace* trace = importer->importOTF2(dataFileName);
    delete importer;

    if (trace)
    {
        connect(trace, SIGNAL(updatePreprocess(int, QString)), this,
                SLOT(updatePreprocess(int, QString)));
        trace->preprocess();
    }

    traceElapsed = traceTimer.nsecsElapsed();
    RavelUtils::gu_printTime(traceElapsed, "Total trace: ");

    emit(done(trace));
}

void ImportFunctor::doImportOTF(QString dataFileName)
{
    #ifdef OTF1LIB
    std::cout << "Processing " << dataFileName.toStdString().c_str() << std::endl;
    QElapsedTimer traceTimer;
    qint64 traceElapsed;

    traceTimer.start();

    OTFConverter * importer = new OTFConverter();
    connect(importer, SIGNAL(finishRead()), this, SLOT(finishInitialRead()));
    connect(importer, SIGNAL(matchingUpdate(int, QString)), this,
            SLOT(updateMatching(int, QString)));
    Trace* trace = importer->importOTF(dataFileName);
    delete importer;

    if (trace)
    {
        connect(trace, SIGNAL(updatePreprocess(int, QString)), this,
                SLOT(updatePreprocess(int, QString)));
        trace->preprocess();
    }

    traceElapsed = traceTimer.nsecsElapsed();
    RavelUtils::gu_printTime(traceElapsed, "Total trace: ");

    emit(done(trace));
    #endif
}

void ImportFunctor::finishInitialRead()
{
    emit(reportProgress(35, "Constructing events..."));
}

void ImportFunctor::updateMatching(int portion, QString msg)
{
    emit(reportProgress(35 + portion, msg));
}

void ImportFunctor::updatePreprocess(int portion, QString msg)
{
    emit(reportProgress(95 + portion / 2.0, msg));
}

void ImportFunctor::switchProgress()
{
    emit(switching());
}
