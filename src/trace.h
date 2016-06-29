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
#ifndef TRACE_H
#define TRACE_H

#include <QObject>
#include <QString>
#include <QList>
#include <QMap>
#include <QVector>
#include <QQueue>
#include <QStack>
#include <QSharedPointer>
#include <QElapsedTimer>

class Event;
class CommEvent;
class Function;
class EntityGroup;
class PrimaryEntityGroup;
class OTFCollective;
class CollectiveRecord;

class Trace : public QObject
{
    Q_OBJECT
public:
    Trace(int nt, int np);
    ~Trace();

    void preprocess();
    Event * findEvent(int entity, unsigned long long time);

    QString name;
    QString fullpath;
    int num_entities;
    int num_pes;
    int units;
    qint64 totalTime;

    QList<QString> * metrics;
    QMap<QString, QString> * metric_units;

    // Below set by OTFConverter
    QMap<int, QString> * functionGroups;
    QMap<int, Function *> * functions;

    QMap<int, PrimaryEntityGroup *> * primaries;
    PrimaryEntityGroup * processingElements;
    QMap<int, EntityGroup *> * entitygroups;
    QMap<int, OTFCollective *> * collective_definitions;

    QMap<unsigned long long, CollectiveRecord *> * collectives;
    QVector<QMap<unsigned long long, CollectiveRecord *> *> * collectiveMap;

    QVector<QVector<Event *> *> * events; // This is going to be by entities
    QVector<QVector<Event *> *> * roots; // Roots of call trees per pe

    int mpi_group; // functionGroup index of "MPI" functions

    unsigned long long max_time; // largest time
    unsigned long long min_time; // starting time

signals:
    // This is for progress bars
    void updatePreprocess(int, QString);

private:
    bool isProcessed; // Partitions exist

    QElapsedTimer totalTimer;

    static const bool debug = false;
    static const int partition_portion = 25;
    static const int lateness_portion = 45;
    static const int steps_portion = 30;
    static const QString collectives_string;
};

#endif // TRACE_H
