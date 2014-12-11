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
#include "otf2importer.h"
#include <QString>
#include <QElapsedTimer>
#include <iostream>
#include <cmath>
#include "general_util.h"
#include "rawtrace.h"
#include "commrecord.h"
#include "eventrecord.h"
#include "collectiverecord.h"
#include "taskgroup.h"
#include "otfcollective.h"
#include "function.h"
#include "task.h"

OTF2Importer::OTF2Importer()
    : ticks_per_second(0),
      time_offset(0),
      time_conversion_factor(0),
      num_processes(0),
      second_magnitude(1),
      entercount(0),
      exitcount(0),
      sendcount(0),
      recvcount(0),
      enforceMessageSize(false),
      otfReader(NULL),
      global_def_callbacks(NULL),
      global_evt_callbacks(NULL),
      stringMap(new QMap<OTF2_StringRef, QString>()),
      locationMap(new QMap<OTF2_LocationRef, OTF2Location *>()),
      locationGroupMap(new QMap<OTF2_LocationGroupRef, OTF2LocationGroup *>()),
      regionMap(new QMap<OTF2_RegionRef, OTF2Region *>()),
      groupMap(new QMap<OTF2_GroupRef, OTF2Group *>()),
      commMap(new QMap<OTF2_CommRef, OTF2Comm *>()),
      commIndexMap(new QMap<OTF2_CommRef, int>()),
      regionIndexMap(new QMap<OTF2_RegionRef, int>()),
      locationIndexMap(new QMap<OTF2_LocationRef, int>()),
      unmatched_recvs(new QVector<QLinkedList<CommRecord *> *>()),
      unmatched_sends(new QVector<QLinkedList<CommRecord *> *>()),
      unmatched_send_requests(new QVector<QLinkedList<CommRecord *> *>()),
      unmatched_send_completes(new QVector<QLinkedList<OTF2IsendComplete *> *>()),
      rawtrace(NULL),
      tasks(NULL),
      functionGroups(NULL),
      functions(NULL),
      taskgroups(NULL),
      collective_definitions(new QMap<int, OTFCollective *>()),
      counters(NULL),
      collectives(NULL),
      collectiveMap(NULL),
      collective_begins(NULL),
      collective_fragments(NULL)
{
    collective_definitions->insert(0, new OTFCollective(0, 1, "Barrier"));
    collective_definitions->insert(1, new OTFCollective(1, 2, "Bcast"));
    collective_definitions->insert(2, new OTFCollective(2, 3, "Gather"));
    collective_definitions->insert(3, new OTFCollective(3, 3, "Gatherv"));
    collective_definitions->insert(4, new OTFCollective(4, 2, "Scatter"));
    collective_definitions->insert(5, new OTFCollective(5, 2, "Scatterv"));
    collective_definitions->insert(6, new OTFCollective(6, 4, "Allgather"));
    collective_definitions->insert(7, new OTFCollective(7, 4, "Allgatherv"));
    collective_definitions->insert(8, new OTFCollective(8, 4, "Alltoall"));
    collective_definitions->insert(9, new OTFCollective(9, 4, "Alltoallv"));
    collective_definitions->insert(10, new OTFCollective(10, 4, "Alltoallw"));
    collective_definitions->insert(11, new OTFCollective(11, 4, "Allreduce"));
    collective_definitions->insert(12, new OTFCollective(12, 3, "Reduce"));
    collective_definitions->insert(13, new OTFCollective(13, 4, "ReduceScatter"));
    collective_definitions->insert(14, new OTFCollective(14, 4, "Scan"));
    collective_definitions->insert(15, new OTFCollective(15, 4, "Exscan"));
    collective_definitions->insert(16, new OTFCollective(16, 4, "ReduceScatterBlock"));
    collective_definitions->insert(17, new OTFCollective(17, 4, "CreateHandle"));
    collective_definitions->insert(18, new OTFCollective(18, 4, "DestroyHandle"));
    collective_definitions->insert(19, new OTFCollective(19, 4, "Allocate"));
    collective_definitions->insert(20, new OTFCollective(20, 4, "Deallocate"));
    collective_definitions->insert(21, new OTFCollective(21, 4, "CreateAllocate"));
    collective_definitions->insert(22, new OTFCollective(22, 4, "DestroyDeallocate"));
}

OTF2Importer::~OTF2Importer()
{
    delete stringMap;
    delete collective_begins;

    for (QVector<QLinkedList<CommRecord *> *>::Iterator eitr
         = unmatched_recvs->begin(); eitr != unmatched_recvs->end(); ++eitr)
    {
        for (QLinkedList<CommRecord *>::Iterator itr = (*eitr)->begin();
             itr != (*eitr)->end(); ++itr)
        {
            delete *itr;
            *itr = NULL;
        }
        delete *eitr;
        *eitr = NULL;
    }
    delete unmatched_recvs;

    for (QVector<QLinkedList<CommRecord *> *>::Iterator eitr
         = unmatched_sends->begin();
         eitr != unmatched_sends->end(); ++eitr)
    {
        // Don't delete, used elsewhere
        /*for (QLinkedList<CommRecord *>::Iterator itr = (*eitr)->begin();
         itr != (*eitr)->end(); ++itr)
        {
            delete *itr;
            *itr = NULL;
        }*/
        delete *eitr;
        *eitr = NULL;
    }
    delete unmatched_sends;


    for (QVector<QLinkedList<CommRecord *> *>::Iterator eitr
         = unmatched_send_requests->begin();
         eitr != unmatched_send_requests->end(); ++eitr)
    {
        // Don't delete, used elsewhere
        /*for (QLinkedList<CommRecord *>::Iterator itr = (*eitr)->begin();
         itr != (*eitr)->end(); ++itr)
        {
            delete *itr;
            *itr = NULL;
        }*/
        delete *eitr;
        *eitr = NULL;
    }
    delete unmatched_send_requests;


    for (QVector<QLinkedList<OTF2IsendComplete *> *>::Iterator eitr
         = unmatched_send_completes->begin(); eitr != unmatched_send_completes->end(); ++eitr)
    {
        for (QLinkedList<OTF2IsendComplete *>::Iterator itr = (*eitr)->begin();
             itr != (*eitr)->end(); ++itr)
        {
            delete *itr;
            *itr = NULL;
        }
        delete *eitr;
        *eitr = NULL;
    }
    delete unmatched_send_completes;

    for(QVector<QLinkedList<OTF2CollectiveFragment *> *>::Iterator eitr
        = collective_fragments->begin();
        eitr != collective_fragments->end(); ++eitr)
    {
        for (QLinkedList<OTF2CollectiveFragment *>::Iterator itr
             = (*eitr)->begin(); itr != (*eitr)->end(); ++itr)
        {
            delete *itr;
            *itr = NULL;
        }
        delete *eitr;
        *eitr = NULL;
    }
    delete collective_fragments;

    for (QMap<OTF2_LocationRef, OTF2Location *>::Iterator eitr
         = locationMap->begin();
         eitr != locationMap->end(); ++eitr)
    {
        delete eitr.value();
    }
    delete locationMap;

    for (QMap<OTF2_LocationGroupRef, OTF2LocationGroup *>::Iterator eitr
         = locationGroupMap->begin();
         eitr != locationGroupMap->end(); ++eitr)
    {
        delete eitr.value();
    }
    delete locationGroupMap;

    for (QMap<OTF2_RegionRef, OTF2Region *>::Iterator eitr
         = regionMap->begin();
         eitr != regionMap->end(); ++eitr)
    {
        delete eitr.value();
    }
    delete regionMap;

    // Don't delete members from OTF2Group, those becomes
    // processes in kept communicators
    for (QMap<OTF2_GroupRef, OTF2Group *>::Iterator eitr
         = groupMap->begin();
         eitr != groupMap->end(); ++eitr)
    {
        delete eitr.value();
    }
    delete groupMap;

    for (QMap<OTF2_CommRef, OTF2Comm *>::Iterator eitr
         = commMap->begin();
         eitr != commMap->end(); ++eitr)
    {
        delete eitr.value();
    }
    delete commMap;
}

RawTrace * OTF2Importer::importOTF2(const char* otf_file, bool _enforceMessageSize)
{
    enforceMessageSize = _enforceMessageSize;
    entercount = 0;
    exitcount = 0;
    sendcount = 0;
    recvcount = 0;

    QElapsedTimer traceTimer;
    qint64 traceElapsed;

    traceTimer.start();

    // Setup
    otfReader = OTF2_Reader_Open(otf_file);
    OTF2_Reader_SetSerialCollectiveCallbacks(otfReader);
    OTF2_GlobalDefReader * global_def_reader = OTF2_Reader_GetGlobalDefReader(otfReader);
    global_def_callbacks = OTF2_GlobalDefReaderCallbacks_New();

    setDefCallbacks();

    OTF2_Reader_RegisterGlobalDefCallbacks( otfReader,
                                            global_def_reader,
                                            global_def_callbacks,
                                            this ); // Register userdata as this

    OTF2_GlobalDefReaderCallbacks_Delete( global_def_callbacks );

    // Read definitions
    std::cout << "Reading definitions" << std::endl;
    uint64_t definitions_read = 0;
    OTF2_Reader_ReadAllGlobalDefinitions( otfReader,
                                          global_def_reader,
                                          &definitions_read );


    tasks = new QMap<int, Task *>();
    functionGroups = new QMap<int, QString>();
    functions = new QMap<int, Function *>();
    taskgroups = new QMap<int, TaskGroup *>();
    collectives = new QMap<unsigned long long, CollectiveRecord *>();
    counters = new QMap<unsigned int, Counter *>();

    processDefinitions();

    rawtrace = new RawTrace(num_processes);
    rawtrace->tasks = tasks;
    rawtrace->second_magnitude = second_magnitude;
    rawtrace->functions = functions;
    rawtrace->functionGroups = functionGroups;
    rawtrace->taskgroups = taskgroups;
    rawtrace->collective_definitions = collective_definitions;
    rawtrace->collectives = collectives;
    rawtrace->counters = counters;
    rawtrace->events = new QVector<QVector<EventRecord *> *>(num_processes);
    rawtrace->messages = new QVector<QVector<CommRecord *> *>(num_processes);
    rawtrace->messages_r = new QVector<QVector<CommRecord *> *>(num_processes);
    rawtrace->counter_records = new QVector<QVector<CounterRecord *> *>(num_processes);
    rawtrace->collectiveBits = new QVector<QVector<RawTrace::CollectiveBit *> *>(num_processes);

    // Adding locations
    // Use the locationIndexMap to only chooes the ones we can handle (right now just MPI)
    for (QMap<OTF2_LocationRef, int>::Iterator loc = locationIndexMap->begin();
         loc != locationIndexMap->end(); ++loc)
    {
        OTF2_Reader_SelectLocation(otfReader, loc.key());
    }

    bool def_files_success = OTF2_Reader_OpenDefFiles(otfReader) == OTF2_SUCCESS;
    OTF2_Reader_OpenEvtFiles(otfReader);
    for (QMap<OTF2_LocationRef, int>::Iterator loc = locationIndexMap->begin();
         loc != locationIndexMap->end(); ++loc)
    {
        if (def_files_success)
        {
            OTF2_DefReader * def_reader = OTF2_Reader_GetDefReader(otfReader, loc.key());
            if (def_reader)
            {
                uint64_t def_reads = 0;
                OTF2_Reader_ReadAllLocalDefinitions( otfReader,
                                                     def_reader,
                                                     &def_reads );
                OTF2_Reader_CloseDefReader( otfReader, def_reader );
            }
        }
        OTF2_EvtReader * unused = OTF2_Reader_GetEvtReader(otfReader, loc.key());
    }
    if (def_files_success)
        OTF2_Reader_CloseDefFiles(otfReader);

    std::cout << "Reading events" << std::endl;
    delete unmatched_recvs;
    unmatched_recvs = new QVector<QLinkedList<CommRecord *> *>(num_processes);
    delete unmatched_sends;
    unmatched_sends = new QVector<QLinkedList<CommRecord *> *>(num_processes);
    delete unmatched_send_requests;
    unmatched_send_requests = new QVector<QLinkedList<CommRecord *> *>(num_processes);
    delete unmatched_send_completes;
    unmatched_send_completes = new QVector<QLinkedList<OTF2IsendComplete *> *>(num_processes);
    delete collectiveMap;
    collectiveMap = new QVector<QMap<unsigned long long, CollectiveRecord *> *>(num_processes);
    delete collective_begins;
    collective_begins = new QVector<QLinkedList<uint64_t> *>(num_processes);
    delete collective_fragments;
    collective_fragments = new QVector<QLinkedList<OTF2CollectiveFragment *> *>(num_processes);
    for (int i = 0; i < num_processes; i++) {
        (*unmatched_recvs)[i] = new QLinkedList<CommRecord *>();
        (*unmatched_sends)[i] = new QLinkedList<CommRecord *>();
        (*unmatched_send_requests)[i] = new QLinkedList<CommRecord *>();
        (*unmatched_send_completes)[i] = new QLinkedList<OTF2IsendComplete *>();
        (*collectiveMap)[i] = new QMap<unsigned long long, CollectiveRecord *>();
        (*(rawtrace->events))[i] = new QVector<EventRecord *>();
        (*(rawtrace->messages))[i] = new QVector<CommRecord *>();
        (*(rawtrace->messages_r))[i] = new QVector<CommRecord *>();
        (*(rawtrace->counter_records))[i] = new QVector<CounterRecord *>();
        (*collective_begins)[i] = new QLinkedList<uint64_t>();
        (*collective_fragments)[i] = new QLinkedList<OTF2CollectiveFragment *>();
        (*(rawtrace->collectiveBits))[i] = new QVector<RawTrace::CollectiveBit *>();
    }


    OTF2_GlobalEvtReader * global_evt_reader = OTF2_Reader_GetGlobalEvtReader(otfReader);

    global_evt_callbacks = OTF2_GlobalEvtReaderCallbacks_New();

    setEvtCallbacks();

    OTF2_Reader_RegisterGlobalEvtCallbacks( otfReader,
                                            global_evt_reader,
                                            global_evt_callbacks,
                                            this ); // Register userdata as this

    OTF2_GlobalEvtReaderCallbacks_Delete( global_evt_callbacks );
    uint64_t events_read = 0;
    OTF2_Reader_ReadAllGlobalEvents( otfReader,
                                     global_evt_reader,
                                     &events_read );


    processCollectives();

    rawtrace->collectiveMap = collectiveMap;

    OTF2_Reader_CloseGlobalEvtReader( otfReader, global_evt_reader );
    OTF2_Reader_CloseEvtFiles( otfReader );
    OTF2_Reader_Close( otfReader );

    std::cout << "Finish reading" << std::endl;

    int unmatched_recv_count = 0;
    for (QVector<QLinkedList<CommRecord *> *>::Iterator eitr
         = unmatched_recvs->begin();
         eitr != unmatched_recvs->end(); ++eitr)
    {
        for (QLinkedList<CommRecord *>::Iterator itr = (*eitr)->begin();
             itr != (*eitr)->end(); ++itr)
        {
            unmatched_recv_count++;
            std::cout << "Unmatched RECV " << (*itr)->sender << "->"
                      << (*itr)->receiver << " (" << (*itr)->send_time << ", "
                      << (*itr)->recv_time << ")" << std::endl;
        }
    }
    int unmatched_send_count = 0;
    for (QVector<QLinkedList<CommRecord *> *>::Iterator eitr
         = unmatched_sends->begin();
         eitr != unmatched_sends->end(); ++eitr)
    {
        for (QLinkedList<CommRecord *>::Iterator itr = (*eitr)->begin();
             itr != (*eitr)->end(); ++itr)
        {
            unmatched_send_count++;
            std::cout << "Unmatched SEND " << (*itr)->sender << "->"
                      << (*itr)->receiver << " (" << (*itr)->send_time << ", "
                      << (*itr)->recv_time << ")" << std::endl;
        }
    }
    std::cout << unmatched_send_count << " unmatched sends and "
              << unmatched_recv_count << " unmatched recvs." << std::endl;


    traceElapsed = traceTimer.nsecsElapsed();
    std::cout << "OTF Reading: ";
    gu_printTime(traceElapsed);
    std::cout << std::endl;

    return rawtrace;
}

void OTF2Importer::setDefCallbacks()
{
    // String
    OTF2_GlobalDefReaderCallbacks_SetStringCallback(global_def_callbacks,
                                                    callbackDefString);

    // Timer
    OTF2_GlobalDefReaderCallbacks_SetClockPropertiesCallback(global_def_callbacks,
                                                             callbackDefClockProperties);

    // Locations
    OTF2_GlobalDefReaderCallbacks_SetLocationGroupCallback(global_def_callbacks,
                                                           callbackDefLocationGroup);
    OTF2_GlobalDefReaderCallbacks_SetLocationCallback(global_def_callbacks,
                                                      callbackDefLocation);

    // Comm
    OTF2_GlobalDefReaderCallbacks_SetCommCallback(global_def_callbacks,
                                                  callbackDefComm);
    OTF2_GlobalDefReaderCallbacks_SetGroupCallback(global_def_callbacks,
                                                  callbackDefGroup);

    // Region
    OTF2_GlobalDefReaderCallbacks_SetRegionCallback(global_def_callbacks,
                                                    callbackDefRegion);


    // TODO: Metrics might be akin to counters
}

void OTF2Importer::processDefinitions()
{
    int index = 0;
    for (QMap<OTF2_RegionRef, OTF2Region *>::Iterator region = regionMap->begin();
         region != regionMap->end(); ++region)
    {
        regionIndexMap->insert(region.key(), index);
        functions->insert(index, new Function(stringMap->value(region.value()->name),
                                              (region.value())->paradigm));
        index++;
    }

    functionGroups->insert(OTF2_PARADIGM_MPI, "MPI");

    index = 0;
    for (QMap<OTF2_CommRef, OTF2Comm *>::Iterator comm = commMap->begin();
         comm != commMap->end(); ++comm)
    {
        commIndexMap->insert(comm.key(), index);
        TaskGroup * t = new TaskGroup(index, stringMap->value((comm.value())->name));
        delete t->tasks;
        t->tasks = groupMap->value((comm.value())->group)->members;
        for (int i = 0; i < t->tasks->size(); i++)
            t->taskorder->insert(t->tasks->at(i), i);
        taskgroups->insert(index, t);
        index++;
    }

    // Grab only the MPI locations
    for (QMap<OTF2_LocationRef, OTF2Location *>::Iterator loc = locationMap->begin();
         loc != locationMap->end(); ++loc)
    {
        OTF2_LocationGroupType group = (locationGroupMap->value((loc.value())->group))->type;
        if (group == OTF2_LOCATION_GROUP_TYPE_PROCESS)
        {
            OTF2_LocationType type = (loc.value())->type;
            if (type == OTF2_LOCATION_TYPE_CPU_THREAD)
            {
                int task = (loc.value())->self;
                tasks->insert(task, new Task(task, stringMap->value(loc.value()->name)));
                locationIndexMap->insert(loc.key(), task);
                num_processes++;
            }
        }
    }

}

void OTF2Importer::setEvtCallbacks()
{
    // Enter / Leave
    OTF2_GlobalEvtReaderCallbacks_SetEnterCallback(global_evt_callbacks,
                                                   &OTF2Importer::callbackEnter);
    OTF2_GlobalEvtReaderCallbacks_SetLeaveCallback(global_evt_callbacks,
                                                   &OTF2Importer::callbackLeave);


    // P2P
    OTF2_GlobalEvtReaderCallbacks_SetMpiSendCallback(global_evt_callbacks,
                                                     &OTF2Importer::callbackMPISend);
    OTF2_GlobalEvtReaderCallbacks_SetMpiIsendCallback(global_evt_callbacks,
                                                      &OTF2Importer::callbackMPIIsend);
    OTF2_GlobalEvtReaderCallbacks_SetMpiIsendCompleteCallback(global_evt_callbacks,
                                                              &OTF2Importer::callbackMPIIsendComplete);
    //OTF2_GlobalEvtReaderCallbacks_SetMpiIrecvRequestCallback(global_evt_callbacks,
    //                                                         &OTF2Importer::callbackMPIIrecvRequest);
    OTF2_GlobalEvtReaderCallbacks_SetMpiIrecvCallback(global_evt_callbacks,
                                                      &OTF2Importer::callbackMPIIrecv);
    OTF2_GlobalEvtReaderCallbacks_SetMpiRecvCallback(global_evt_callbacks,
                                                     &OTF2Importer::callbackMPIRecv);


    // Collective
    OTF2_GlobalEvtReaderCallbacks_SetMpiCollectiveBeginCallback(global_evt_callbacks,
                                                                  callbackMPICollectiveBegin);
    OTF2_GlobalEvtReaderCallbacks_SetMpiCollectiveEndCallback(global_evt_callbacks,
                                                               callbackMPICollectiveEnd);



}

// Find timescale
uint64_t OTF2Importer::convertTime(void* userData, OTF2_TimeStamp time)
{
    return (uint64_t) ((double) (time - ((OTF2Importer *) userData)->time_offset))
            * ((OTF2Importer *) userData)->time_conversion_factor;
}


// May want to save globalOffset and traceLength for max and min
OTF2_CallbackCode OTF2Importer::callbackDefClockProperties(void * userData,
                                                           uint64_t timerResolution,
                                                           uint64_t globalOffset,
                                                           uint64_t traceLength)
{
    Q_UNUSED(traceLength);

    ((OTF2Importer*) userData)->ticks_per_second = timerResolution;
    ((OTF2Importer*) userData)->time_offset = globalOffset;
    ((OTF2Importer*) userData)->second_magnitude
            = (int) floor(log10(timerResolution));

    // Use the timer resolution to convert to seconds and
    // multiply by the magnitude of this factor to get into
    // fractions of a second befitting the recorded unit.
    double conversion_factor;
    conversion_factor = pow(10,
                            ((OTF2Importer*) userData)->second_magnitude) // Convert to ms, ns, fs etc
            / ((double) timerResolution); // Convert to seconds

    ((OTF2Importer*) userData)->time_conversion_factor = conversion_factor;
    return OTF2_CALLBACK_SUCCESS;
}

OTF2_CallbackCode OTF2Importer::callbackDefString(void * userData,
                                                  OTF2_StringRef self,
                                                  const char * string)
{
    ((OTF2Importer*) userData)->stringMap->insert(self, QString(string));
    return OTF2_CALLBACK_SUCCESS;
}


OTF2_CallbackCode OTF2Importer::callbackDefLocationGroup(void * userData,
                                                         OTF2_LocationGroupRef self,
                                                         OTF2_StringRef name,
                                                         OTF2_LocationGroupType locationGroupType,
                                                         OTF2_SystemTreeNodeRef systemTreeParent)
{
    OTF2LocationGroup * g = new OTF2LocationGroup(self, name, locationGroupType,
                                                  systemTreeParent);
    (*(((OTF2Importer*) userData)->locationGroupMap))[self] = g;
    return OTF2_CALLBACK_SUCCESS;
}


OTF2_CallbackCode OTF2Importer::callbackDefLocation(void * userData,
                                                    OTF2_LocationRef self,
                                                    OTF2_StringRef name,
                                                    OTF2_LocationType locationType,
                                                    uint64_t numberOfEvents,
                                                    OTF2_LocationGroupRef locationGroup)
{
    OTF2Location * loc = new OTF2Location(self, name, locationType,
                                          numberOfEvents, locationGroup);
    (*(((OTF2Importer*) userData)->locationMap))[self] = loc;

    return OTF2_CALLBACK_SUCCESS;
}

OTF2_CallbackCode OTF2Importer::callbackDefComm(void * userData,
                                                OTF2_CommRef self,
                                                OTF2_StringRef name,
                                                OTF2_GroupRef group,
                                                OTF2_CommRef parent)
{

    OTF2Comm * c = new OTF2Comm(self, name, group, parent);
    (*(((OTF2Importer*) userData)->commMap))[self] = c;

    return OTF2_CALLBACK_SUCCESS;
}

OTF2_CallbackCode OTF2Importer::callbackDefRegion(void * userData,
                                                  OTF2_RegionRef self,
                                                  OTF2_StringRef name,
                                                  OTF2_StringRef canonicalName,
                                                  OTF2_StringRef description,
                                                  OTF2_RegionRole regionRole,
                                                  OTF2_Paradigm paradigm,
                                                  OTF2_RegionFlag regionFlag,
                                                  OTF2_StringRef sourceFile,
                                                  uint32_t beginLineNumber,
                                                  uint32_t endLineNumber)
{
    Q_UNUSED(description);
    OTF2Region * r = new OTF2Region(self, name, canonicalName, regionRole,
                                    paradigm, regionFlag, sourceFile,
                                    beginLineNumber, endLineNumber);
     (*(((OTF2Importer*) userData)->regionMap))[self] = r;
    return OTF2_CALLBACK_SUCCESS;
}

OTF2_CallbackCode OTF2Importer::callbackDefGroup(void* userData,
                                                 OTF2_GroupRef self,
                                                 OTF2_StringRef name,
                                                 OTF2_GroupType groupType,
                                                 OTF2_Paradigm paradigm,
                                                 OTF2_GroupFlag groupFlags,
                                                 uint32_t numberOfMembers,
                                                 const uint64_t* members)
{
    OTF2Group * g = new OTF2Group(self, name, groupType, paradigm, groupFlags);
    for (uint32_t i = 0; i < numberOfMembers; i++)
        g->members->append(members[i]);
    (*(((OTF2Importer*) userData)->groupMap))[self] = g;
    return OTF2_CALLBACK_SUCCESS;
}

OTF2_CallbackCode OTF2Importer::callbackEnter(OTF2_LocationRef locationID,
                                              OTF2_TimeStamp time,
                                              void * userData,
                                              OTF2_AttributeList * attributeList,
                                              OTF2_RegionRef region)
{
    Q_UNUSED(attributeList);
    int process = ((OTF2Importer *) userData)->locationIndexMap->value(locationID);
    int function = ((OTF2Importer *) userData)->regionIndexMap->value(region);
    ((*((((OTF2Importer*) userData)->rawtrace)->events))[process])->append(new EventRecord(process,
                                                                                           convertTime(userData,
                                                                                                       time),
                                                                                           function,
                                                                                           true));
    return OTF2_CALLBACK_SUCCESS;
}

OTF2_CallbackCode OTF2Importer::callbackLeave(OTF2_LocationRef locationID,
                                              OTF2_TimeStamp time,
                                              void * userData,
                                              OTF2_AttributeList * attributeList,
                                              OTF2_RegionRef region)
{
    Q_UNUSED(attributeList);
    int process = ((OTF2Importer *) userData)->locationIndexMap->value(locationID);
    int function = ((OTF2Importer *) userData)->regionIndexMap->value(region);
    ((*((((OTF2Importer*) userData)->rawtrace)->events))[process])->append(new EventRecord(process,
                                                                                           convertTime(userData,
                                                                                                       time),
                                                                                           function,
                                                                                           false));
    return OTF2_CALLBACK_SUCCESS;
}


// Check if two comm records match
// (one that already is a record, one that is just parts)
bool OTF2Importer::compareComms(CommRecord * comm, unsigned int sender,
                                unsigned int receiver, unsigned int tag,
                                unsigned int size)
{
    if ((comm->sender != sender) || (comm->receiver != receiver)
            || (comm->tag != tag) || (comm->size != size))
        return false;
    return true;
}

bool OTF2Importer::compareComms(CommRecord * comm, unsigned int sender,
                                unsigned int receiver, unsigned int tag)
{
    if ((comm->sender != sender) || (comm->receiver != receiver)
            || (comm->tag != tag))
        return false;
    return true;
}


OTF2_CallbackCode OTF2Importer::callbackMPISend(OTF2_LocationRef locationID,
                                                OTF2_TimeStamp time,
                                                void * userData,
                                                OTF2_AttributeList * attributeList,
                                                uint32_t receiver,
                                                OTF2_CommRef communicator,
                                                uint32_t msgTag,
                                                uint64_t msgLength)
{
    Q_UNUSED(attributeList);

    // Every time we find a send, check the unmatched recvs
    // to see if it has a match
    unsigned long long converted_time = convertTime(userData, time);
    int sender = ((OTF2Importer *) userData)->locationIndexMap->value(locationID);
    OTF2Comm * comm = ((OTF2Importer *) userData)->commMap->value(communicator);
    OTF2Group * group = ((OTF2Importer *) userData)->groupMap->value(comm->group);
    int world_receiver = group->members->at(receiver);
    CommRecord * cr = NULL;
    QLinkedList<CommRecord *> * unmatched = (*(((OTF2Importer *) userData)->unmatched_recvs))[sender];
    bool useSize = ((OTF2Importer *) userData)->enforceMessageSize;
    for (QLinkedList<CommRecord *>::Iterator itr = unmatched->begin();
         itr != unmatched->end(); ++itr)
    {
        if (useSize ? OTF2Importer::compareComms((*itr), sender, world_receiver, msgTag, msgLength)
                    : OTF2Importer::compareComms((*itr), sender, world_receiver, msgTag))
        {
            cr = *itr;
            cr->send_time = converted_time;
            ((*((((OTF2Importer*) userData)->rawtrace)->messages))[sender])->append((cr));
            break;
        }
    }


    // If we did find a match, remove it from the unmatched.
    // Otherwise, create a new unmatched send record
    if (cr)
    {
        (*(((OTF2Importer *) userData)->unmatched_recvs))[sender]->removeOne(cr);
    }
    else
    {
        int taskgroup = ((OTF2Importer *) userData)->commIndexMap->value(communicator);
        cr = new CommRecord(sender, converted_time, world_receiver, 0, msgLength, msgTag, taskgroup);
        (*((((OTF2Importer*) userData)->rawtrace)->messages))[sender]->append(cr);
        (*(((OTF2Importer *) userData)->unmatched_sends))[sender]->append(cr);
    }
    return OTF2_CALLBACK_SUCCESS;
}

OTF2_CallbackCode OTF2Importer::callbackMPIIsend(OTF2_LocationRef locationID,
                                                 OTF2_TimeStamp time,
                                                 void * userData,
                                                 OTF2_AttributeList * attributeList,
                                                 uint32_t receiver,
                                                 OTF2_CommRef communicator,
                                                 uint32_t msgTag,
                                                 uint64_t msgLength,
                                                 uint64_t requestID)
{
    Q_UNUSED(attributeList);

    // Every time we find a send, check the unmatched recvs
    // to see if it has a match
    unsigned long long converted_time = convertTime(userData, time);
    int sender = ((OTF2Importer *) userData)->locationIndexMap->value(locationID);
    CommRecord * cr = NULL;
    QLinkedList<CommRecord *> * unmatched = (*(((OTF2Importer *) userData)->unmatched_recvs))[sender];
    bool useSize = ((OTF2Importer *) userData)->enforceMessageSize;
    for (QLinkedList<CommRecord *>::Iterator itr = unmatched->begin();
         itr != unmatched->end(); ++itr)
    {
        if (useSize ? OTF2Importer::compareComms((*itr), sender, receiver, msgTag, msgLength)
                    : OTF2Importer::compareComms((*itr), sender, receiver, msgTag))
        {
            cr = *itr;
            cr->send_time = converted_time;
            ((*((((OTF2Importer*) userData)->rawtrace)->messages))[sender])->append((cr));
            break;
        }
    }

    // If we did find a match, remove it from the unmatched.
    // Otherwise, create a new unmatched send record
    if (cr)
    {
        (*(((OTF2Importer *) userData)->unmatched_recvs))[sender]->removeOne(cr);
    }
    else
    {
        int taskgroup = ((OTF2Importer *) userData)->commIndexMap->value(communicator);
        cr = new CommRecord(sender, converted_time, receiver, 0, msgLength,
                            msgTag, taskgroup, requestID);
        (*((((OTF2Importer*) userData)->rawtrace)->messages))[sender]->append(cr);
        (*(((OTF2Importer *) userData)->unmatched_sends))[sender]->append(cr);
    }

    // Also check the complete time stuff
    OTF2IsendComplete * complete = NULL;
    QLinkedList<OTF2IsendComplete *> * completes
            = (*(((OTF2Importer *) userData)->unmatched_send_completes))[sender];
    for (QLinkedList<OTF2IsendComplete *>::Iterator itr = completes->begin();
         itr != completes->end(); ++itr)
    {
        if (requestID ==(*itr)->request)
        {
            complete = *itr;
            cr->send_complete = (*itr)->time;
            break;
        }
    }

    if (complete)
    {
        (*(((OTF2Importer *) userData)->unmatched_send_completes))[sender]->removeOne(complete);
    }
    else
    {
        (*(((OTF2Importer *) userData)->unmatched_send_requests))[sender]->append(cr);
    }

    return OTF2_CALLBACK_SUCCESS;
}


// Do nothing for now
OTF2_CallbackCode OTF2Importer::callbackMPIIsendComplete(OTF2_LocationRef locationID,
                                                         OTF2_TimeStamp time,
                                                         void * userData,
                                                         OTF2_AttributeList * attributeList,
                                                         uint64_t requestID)
{
    Q_UNUSED(attributeList);

    // Check to see if we have a matching send request
    unsigned long long converted_time = convertTime(userData, time);
    int sender = ((OTF2Importer *) userData)->locationIndexMap->value(locationID);
    CommRecord * cr = NULL;
    QLinkedList<CommRecord *> * unmatched = (*(((OTF2Importer *) userData)->unmatched_send_requests))[sender];
    for (QLinkedList<CommRecord *>::Iterator itr = unmatched->begin();
         itr != unmatched->end(); ++itr)
    {
        if ((*itr)->send_request == requestID)
        {
            cr = *itr;
            cr->send_complete = converted_time;
            break;
        }
    }

    // If we did find a match, remove it from the unmatched.
    // Otherwise, create a new unmatched send record
    if (cr)
    {
        (*(((OTF2Importer *) userData)->unmatched_send_requests))[sender]->removeOne(cr);
    }
    else
    {
        (*(((OTF2Importer *) userData)->unmatched_send_completes))[sender]->append(new OTF2IsendComplete(converted_time,
                                                                                                         requestID));
    }

    return OTF2_CALLBACK_SUCCESS;
}

// Do nothing for now
OTF2_CallbackCode OTF2Importer::callbackMPIIrecvRequest(OTF2_LocationRef locationID,
                                                        OTF2_TimeStamp time,
                                                        void * userData,
                                                        OTF2_AttributeList * attributeList,
                                                        uint64_t requestID)
{
    Q_UNUSED(locationID);
    Q_UNUSED(time);
    Q_UNUSED(userData);
    Q_UNUSED(attributeList);
    Q_UNUSED(requestID);

    return OTF2_CALLBACK_SUCCESS;
}

OTF2_CallbackCode OTF2Importer::callbackMPIRecv(OTF2_LocationRef locationID,
                                                OTF2_TimeStamp time,
                                                void * userData,
                                                OTF2_AttributeList * attributeList,
                                                uint32_t sender,
                                                OTF2_CommRef communicator,
                                                uint32_t msgTag,
                                                uint64_t msgLength)
{
    Q_UNUSED(attributeList);

    // Look for match in unmatched_sends
    unsigned long long converted_time = convertTime(userData, time);
    int receiver = ((OTF2Importer *) userData)->locationIndexMap->value(locationID);
    OTF2Comm * comm = ((OTF2Importer *) userData)->commMap->value(communicator);
    OTF2Group * group = ((OTF2Importer *) userData)->groupMap->value(comm->group);
    int world_sender = group->members->at(sender);
    CommRecord * cr = NULL;
    QLinkedList<CommRecord *> * unmatched = (*(((OTF2Importer*) userData)->unmatched_sends))[world_sender];
    bool useSize = ((OTF2Importer *) userData)->enforceMessageSize;
    for (QLinkedList<CommRecord *>::Iterator itr = unmatched->begin();
         itr != unmatched->end(); ++itr)
    {
        if (useSize ? OTF2Importer::compareComms((*itr), world_sender, receiver, msgTag, msgLength)
                    : OTF2Importer::compareComms((*itr), world_sender, receiver, msgTag))
        {
            cr = *itr;
            cr->recv_time = converted_time;
            break;
        }
    }

    // If match is found, remove it from unmatched_sends, otherwise create
    // a new unmatched recv record
    if (cr)
    {
        (*(((OTF2Importer *) userData)->unmatched_sends))[world_sender]->removeOne(cr);
    }
    else
    {
        int taskgroup = ((OTF2Importer *) userData)->commIndexMap->value(communicator);
        cr = new CommRecord(world_sender, 0, receiver, converted_time, msgLength, msgTag, taskgroup);
        ((*(((OTF2Importer*) userData)->unmatched_recvs))[world_sender])->append(cr);
    }
    (*((((OTF2Importer*) userData)->rawtrace)->messages_r))[receiver]->append(cr);

    return OTF2_CALLBACK_SUCCESS;
}

OTF2_CallbackCode OTF2Importer::callbackMPIIrecv(OTF2_LocationRef locationID,
                                                 OTF2_TimeStamp time,
                                                 void * userData,
                                                 OTF2_AttributeList * attributeList,
                                                 uint32_t sender,
                                                 OTF2_CommRef communicator,
                                                 uint32_t msgTag,
                                                 uint64_t msgLength,
                                                 uint64_t requestID)
{
    Q_UNUSED(attributeList);
    Q_UNUSED(requestID);

    // Look for match in unmatched_sends
    unsigned long long converted_time = convertTime(userData, time);
    int receiver = ((OTF2Importer *) userData)->locationIndexMap->value(locationID);
    CommRecord * cr = NULL;
    QLinkedList<CommRecord *> * unmatched = (*(((OTF2Importer*) userData)->unmatched_sends))[sender];
    bool useSize = ((OTF2Importer *) userData)->enforceMessageSize;
    for (QLinkedList<CommRecord *>::Iterator itr = unmatched->begin();
         itr != unmatched->end(); ++itr)
    {
        if (useSize ? OTF2Importer::compareComms((*itr), sender, receiver, msgTag, msgLength)
                    : OTF2Importer::compareComms((*itr), sender, receiver, msgTag))
        {
            cr = *itr;
            cr->recv_time = converted_time;
            break;
        }
    }

    // If match is found, remove it from unmatched_sends, otherwise create
    // a new unmatched recv record
    if (cr)
    {
        (*(((OTF2Importer *) userData)->unmatched_sends))[sender]->removeOne(cr);
    }
    else
    {
        int taskgroup = ((OTF2Importer *) userData)->commIndexMap->value(communicator);
        cr = new CommRecord(sender, 0, receiver, converted_time, msgLength, msgTag, taskgroup);
        ((*(((OTF2Importer*) userData)->unmatched_recvs))[sender])->append(cr);
    }
    (*((((OTF2Importer*) userData)->rawtrace)->messages_r))[receiver]->append(cr);

    return OTF2_CALLBACK_SUCCESS;
}

// We have to just collect the Collective information for now and then go through
// it in order later because we are not guaranteed on order for begin/end and
// interleaving between processes.
OTF2_CallbackCode OTF2Importer::callbackMPICollectiveBegin(OTF2_LocationRef locationID,
                                                           OTF2_TimeStamp time,
                                                           void * userData,
                                                           OTF2_AttributeList * attributeList)
{
    Q_UNUSED(locationID);
    Q_UNUSED(attributeList);

    int process = ((OTF2Importer *) userData)->locationIndexMap->value(locationID);
    uint64_t converted_time = convertTime(userData, time);
    ((OTF2Importer *) userData)->collective_begins->at(process)->append(converted_time);
    return OTF2_CALLBACK_SUCCESS;
}

OTF2_CallbackCode OTF2Importer::callbackMPICollectiveEnd(OTF2_LocationRef locationID,
                                                         OTF2_TimeStamp time,
                                                         void * userData,
                                                         OTF2_AttributeList * attributeList,
                                                         OTF2_CollectiveOp collectiveOp,
                                                         OTF2_CommRef communicator,
                                                         uint32_t root,
                                                         uint64_t sizeSent,
                                                         uint64_t sizeReceived)
{
    Q_UNUSED(attributeList);
    Q_UNUSED(sizeSent);
    Q_UNUSED(sizeReceived);
    //Q_UNUSED(time);

    int process = ((OTF2Importer *) userData)->locationIndexMap->value(locationID);

    ((OTF2Importer *) userData)->collective_fragments->at(process)->append(new OTF2CollectiveFragment(convertTime(userData, time),
                                                                                                      collectiveOp,
                                                                                                      communicator,
                                                                                                      root));
    return OTF2_CALLBACK_SUCCESS;
}

void OTF2Importer::processCollectives()
{
    int id = 0;

    // Have to check each process in case of single process communicator
    // But probably later processes will not have many fragments left
    // after processing by earlier fragments
    for (int i = 0; i < num_processes; i++)
    {
        QLinkedList<OTF2CollectiveFragment *> * fragments = collective_fragments->at(i);
        while (!fragments->isEmpty())
        {
            // Unmatched as of yet fragment becomes a CollectiveRecord
            OTF2CollectiveFragment * fragment = fragments->first();
            CollectiveRecord * cr = new CollectiveRecord(id, fragment->root,
                                                         fragment->op,
                                                         commIndexMap->value(fragment->comm));
            collectives->insert(id, cr);

            // Look through fragment list of other members of communicator for
            // matching fragments
            QList<uint32_t> * members = groupMap->value(commMap->value(fragment->comm)->group)->members;
            for (QList<uint32_t>::Iterator process = members->begin();
                 process != members->end(); ++process)
            {
                OTF2CollectiveFragment * match = NULL;
                for (QLinkedList<OTF2CollectiveFragment *>::Iterator cf
                     = collective_fragments->at(*process)->begin();
                     cf != collective_fragments->at(*process)->end(); ++cf)
                {
                    // A match!
                    if ((*cf)->op == fragment->op
                        && (*cf)->comm == fragment->comm
                        && (*cf)->root == fragment->root)
                    {
                        match = *cf;
                        break;
                    }
                }

                if (!match)
                {
                    std::cout << "Error, no matching collective found for";
                    std::cout << " collective type " << int(fragment->op);
                    std::cout << " on communicator ";
                    std::cout << stringMap->value(commMap->value(fragment->comm)->name).toStdString().c_str();
                    std::cout << " for process " << *process << std::endl;
                }
                else
                {
                    // It's kind of weird that I can't expect the fragments to be in order
                    // but I have to rely on the begin_times being in order... we'll see
                    // if they actually work out.
                    uint64_t begin_time = collective_begins->at(*process)->takeFirst();
                    collective_fragments->at(*process)->removeOne(match);

                    collectiveMap->at(*process)->insert(begin_time, cr);
                    rawtrace->collectiveBits->at(*process)->append(new RawTrace::CollectiveBit(begin_time, cr));
                }
            }

            id++;
        }
    }
}
