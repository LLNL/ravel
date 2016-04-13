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
#ifndef RAWTRACE_H
#define RAWTRACE_H

#include <QString>
#include <QMap>
#include <QVector>
#include <stdint.h>

class PrimaryEntityGroup;
class EntityGroup;
class CommRecord;
class OTFCollective;
class CollectiveRecord;
class Function;
class Counter;
class CounterRecord;
class EventRecord;
class ImportOptions;

// Trace from OTF without processing
class RawTrace
{
public:
    RawTrace(int nt, int np);
    ~RawTrace();

    class CollectiveBit {
    public:
        CollectiveBit(uint64_t _time, CollectiveRecord * _cr)
            : time(_time), cr(_cr) {}

        uint64_t time;
        CollectiveRecord * cr;
    };

    ImportOptions * options;
    QMap<int, PrimaryEntityGroup *> * primaries;
    QMap<int, QString> * functionGroups;
    QMap<int, Function *> * functions;
    QVector<QVector<EventRecord *> *> * events;
    QVector<QVector<CommRecord *> *> * messages;
    QVector<QVector<CommRecord *> *> * messages_r; // by receiver instead of sender
    QMap<int, EntityGroup *> * entitygroups;
    QMap<int, OTFCollective *> * collective_definitions;
    QMap<unsigned int, Counter *> * counters;
    QVector<QVector<CounterRecord * > *> * counter_records;

    QMap<unsigned long long, CollectiveRecord *> * collectives;
    QVector<QMap<unsigned long long, CollectiveRecord *> *> * collectiveMap;
    QVector<QVector<CollectiveBit *> *> * collectiveBits;
    int num_entities;
    int num_pes;
    int second_magnitude; // seconds are 10^this over the smallest smaple unit
    QString from_saved_version;
    QList<QString> * metric_names;
    QMap<QString, QString> * metric_units;
};

#endif // RAWTRACE_H
