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
#ifndef OTFIMPORTER_H
#define OTFIMPORTER_H

#include <QMap>
#include <QLinkedList>
#include <QString>
#include <stdint.h>
#include "otf.h"

class CommRecord;
class Function;
class EntityGroup;
class OTFCollective;
class Counter;
class CollectiveRecord;
class RawTrace;
class PrimaryEntityGroup;

// Use OTF API to get records
class OTFImporter
{
public:
    OTFImporter();
    ~OTFImporter();
    RawTrace * importOTF(const char* otf_file, bool _enforceMessageSize);

    // Handlers per OTF
    static int handleDefTimerResolution(void * userData, uint32_t stream,
                                        uint64_t ticksPerSecond);
    static int handleDefFunctionGroup(void * userData, uint32_t stream,
                                      uint32_t funcGroup, const char * name);
    static int handleDefFunction(void * userData, uint32_t stream,
                                 uint32_t func, const char* name,
                                 uint32_t funcGroup, uint32_t source);
    static int handleDefProcess(void * userData, uint32_t stream,
                                uint32_t process, const char* name,
                                uint32_t parent);
    static int handleDefCounter(void * userData, uint32_t stream,
                                uint32_t counter, const char* name,
                                uint32_t properties, uint32_t counterGroup,
                                const char* unit);
    static int handleEnter(void * userData, uint64_t time, uint32_t function,
                           uint32_t process, uint32_t source);
    static int handleLeave(void * userData, uint64_t time, uint32_t function,
                           uint32_t process, uint32_t source);
    static int handleSend(void * userData, uint64_t time, uint32_t sender,
                          uint32_t receiver, uint32_t group, uint32_t type,
                          uint32_t length, uint32_t source);
    static int handleRecv(void * userData, uint64_t time, uint32_t receiver,
                          uint32_t sender, uint32_t group, uint32_t type,
                          uint32_t length, uint32_t source);
    static int handleCounter(void * userData, uint64_t time, uint32_t process,
                             uint32_t counter, uint64_t value);
    static int handleDefProcessGroup(void * userData, uint32_t stream,
                                     uint32_t procGroup, const char * name,
                                     uint32_t numberOfProcs,
                                     const uint32_t * procs);
    static int handleDefCollectiveOperation(void * userData, uint32_t stream,
                                            uint32_t collOp, const char * name,
                                            uint32_t type);
    static int handleCollectiveOperation(void * userData, uint64_t time,
                                         uint32_t process, uint32_t collective,
                                         uint32_t procGroup, uint32_t rootProc,
                                         uint32_t sent, uint32_t received,
                                         uint64_t duration, uint32_t source);
    static int handleBeginCollectiveOperation(void * userData, uint64_t time,
                                              uint32_t process,
                                              uint32_t collective,
                                              uint64_t matchingId,
                                              uint32_t procGroup,
                                              uint32_t rootProc, uint32_t sent,
                                              uint32_t received,
                                              uint32_t scltoken,
                                              OTF_KeyValueList * list);
    static int handleEndCollectiveOperation(void * userData, uint64_t time,
                                            uint32_t process,
                                            uint64_t matchingId,
                                            OTF_KeyValueList * list);

    // Match comm record of sender and receiver to find both times
    static bool compareComms(CommRecord * comm, unsigned int sender,
                             unsigned int receiver, unsigned int tag,
                             unsigned int size);
    static bool compareComms(CommRecord * comm, unsigned int sender,
                             unsigned int receiver, unsigned int tag);


    static uint64_t convertTime(void* userData, uint64_t time);

    unsigned long long int ticks_per_second;
    double time_conversion_factor;
    int num_processes;
    int second_magnitude;

    int entercount;
    int exitcount;
    int sendcount;
    int recvcount;

private:
    void setHandlers();

    bool enforceMessageSize;

    OTF_FileManager * fileManager;
    OTF_Reader * otfReader;
    OTF_HandlerArray * handlerArray;

    QVector<QLinkedList<CommRecord *> *> * unmatched_recvs;
    QVector<QLinkedList<CommRecord *> *> * unmatched_sends;

    RawTrace * rawtrace;
    QMap<int, PrimaryEntityGroup *> * primaries;
    QMap<int, QString> * functionGroups;
    QMap<int, Function *> * functions;
    QMap<int, EntityGroup *> * entitygroups;
    QMap<int, OTFCollective *> * collective_definitions;
    QMap<unsigned int, Counter *> * counters;

    QMap<unsigned long long, CollectiveRecord *> * collectives;
    QVector<QMap<unsigned long long, CollectiveRecord *> *> * collectiveMap;

};

#endif // OTFIMPORTER_H
