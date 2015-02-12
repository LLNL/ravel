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
#ifndef OTFCONVERTER_H
#define OTFCONVERTER_H

#include <QObject>
#include <QString>
#include <QMap>
#include <QStack>

class RawTrace;
class OTFImporter;
class OTF2Importer;
class OTFImportOptions;
class Trace;
class Partition;
class CommEvent;
class CounterRecord;
class EventRecord;

// Uses the raw records read from the OTF:
// - switches point events into durational events
// - builds call tree
// - matches messages to durational events
class OTFConverter : public QObject
{
    Q_OBJECT
public:
    OTFConverter();
    ~OTFConverter();
    Trace * importOTF(QString filename, OTFImportOptions * _options);
    Trace * importOTF2(QString filename, OTFImportOptions * _options);

signals:
    void finishRead();
    void matchingUpdate(int, QString);

private:
    void convert();
    void matchEvents();
    void matchEventsSaved();
    void makeSingletonPartition(CommEvent * evt);
    void addToSavedPartition(CommEvent * evt, int partition);
    void handleSavedAttributes(CommEvent * evt, EventRecord *er);
    void mergeForWaitall(QList<QList<Partition * > *> * groups);
    int advanceCounters(CommEvent * evt, QStack<CounterRecord *> * counterstack,
                        QVector<CounterRecord *> * counters, int index,
                        QMap<unsigned int, CounterRecord *> * lastcounters);

    RawTrace * rawtrace;
    Trace * trace;
    OTFImportOptions * options;
    int phaseFunction;

    static const int event_match_portion = 24;
    static const int message_match_portion = 0;
    static const QString collectives_string;

};

#endif // OTFCONVERTER_H
