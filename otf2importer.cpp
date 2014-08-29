#include "otf2importer.h"
#include <QString>
#include <QElapsedTimer>
#include <iostream>
#include <cmath>
#include "general_util.h"

OTF2Importer::OTF2Importer()
    : ticks_per_second(0),
      time_conversion_factor(0),
      num_processes(0),
      entercount(0),
      exitcount(0),
      sendcount(0),
      recvcount(0),
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
      rawtrace(NULL),
      functionGroups(NULL),
      functions(NULL),
      communicators(NULL),
      collective_definitions(NULL),
      counters(NULL),
      collectives(NULL),
      collectiveMap(NULL)
{

}

OTF2Importer::~OTF2Importer()
{
    delete stringMap;
    delete locationMap;
    delete locationGroupMap;
    delete regionMap;
    delete groupMap;
    delete commMap;
    delete commIndexMap;
    delete regionIndexMap;
    delete locationIndexMap;

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
}

RawTrace * OTF2Importer::importOTF2(const char* otf_file)
{

    entercount = 0;
    exitcount = 0;
    sendcount = 0;
    recvcount = 0;

    QElapsedTimer traceTimer;
    qint64 traceElapsed;

    traceTimer.start();

    // Setup
    otfReader = OTF2_Reader_Open(otf_file);
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


    functionGroups = new QMap<int, QString>();
    functions = new QMap<int, Function *>();
    communicators = new QMap<int, Communicator *>();
    collective_definitions = new QMap<int, OTFCollective *>();
    collectives = new QMap<unsigned long long, CollectiveRecord *>();
    counters = new QMap<unsigned int, Counter *>();

    processDefinitions();

    rawtrace = new RawTrace(num_processes);
    rawtrace->functions = functions;
    rawtrace->functionGroups = functionGroups;
    rawtrace->communicators = communicators;
    rawtrace->collective_definitions = collective_definitions;
    rawtrace->collectives = collectives;
    rawtrace->counters = counters;
    rawtrace->events = new QVector<QVector<EventRecord *> *>(num_processes);
    rawtrace->messages = new QVector<QVector<CommRecord *> *>(num_processes);
    rawtrace->messages_r = new QVector<QVector<CommRecord *> *>(num_processes);
    rawtrace->counter_records = new QVector<QVector<CounterRecord *> *>(num_processes);

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
    delete collectiveMap;
    collectiveMap = new QVector<QMap<unsigned long long, CollectiveRecord *> *>(num_processes);
    for (int i = 0; i < num_processes; i++) {
        (*unmatched_recvs)[i] = new QLinkedList<CommRecord *>();
        (*unmatched_sends)[i] = new QLinkedList<CommRecord *>();
        (*collectiveMap)[i] = new QMap<unsigned long long, CollectiveRecord *>();
        (*(rawtrace->events))[i] = new QVector<EventRecord *>();
        (*(rawtrace->messages))[i] = new QVector<CommRecord *>();
        (*(rawtrace->messages_r))[i] = new QVector<CommRecord *>();
        (*(rawtrace->counter_records))[i] = new QVector<CounterRecord *>();
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
        Communicator * c = new Communicator(index, stringMap->value((comm.value())->name));
        OTF2Group * g = groupMap->value((comm.value())->group);
        for (QList<uint64_t>::Iterator member = g->members.begin();
             member != g->members.end(); ++member)
        {
            c->processes->append(*member);
        }
        communicators->insert(index, c);
        index++;
    }

    // Grab only the MPI locations
    for (QMap<OTF2_LocationRef, OTF2Location *>::Iterator loc = locationMap->begin();
         loc != locationMap->end(); ++loc)
    {
        QString group = stringMap->value((locationGroupMap->value((loc.value())->group))->name);
        if (group.startsWith("MPI Rank "))
        {
            QString name = stringMap->value((loc.value())->name);
            if (name.startsWith("Master thread"))
            {
                int process = group.remove(0, 9).toInt();
                locationIndexMap->insert(loc.key(), process);
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
    OTF2_GlobalEvtReaderCallbacks_SetMpiIrecvRequestCallback(global_evt_callbacks,
                                                             &OTF2Importer::callbackMPIIrecvRequest);
    OTF2_GlobalEvtReaderCallbacks_SetMpiIrecvCallback(global_evt_callbacks,
                                                      &OTF2Importer::callbackMPIIrecv);
    OTF2_GlobalEvtReaderCallbacks_SetMpiRecvCallback(global_evt_callbacks,
                                                     &OTF2Importer::callbackMPIRecv);


    // Collective
    /*OTF2_GlobalEvtReaderCallbacks_SetMpiCollectiveBeginCallback(global_evt_callbacks,
                                                                  callbackMPICollectiveBegin);
    OTF2_GlobalEvtReaderCallbacks_SetMpiCollectiveEndCallback(global_evt_callbacks,
                                                               callbackMPICollectiveEnd);
                                                             */


}

// Find timescale
uint64_t OTF2Importer::convertTime(void* userData, OTF2_TimeStamp time)
{
    return (uint64_t) ((double) time)
            * ((OTF2Importer *) userData)->time_conversion_factor;
}


// May want to save globalOffset and traceLength for max and min
OTF2_CallbackCode OTF2Importer::callbackDefClockProperties(void * userData,
                                                           uint64_t timerResolution,
                                                           uint64_t globalOffset,
                                                           uint64_t traceLength)
{
    ((OTF2Importer*) userData)->ticks_per_second = timerResolution;

    double conversion_factor;
    conversion_factor = pow(10, (int) floor(log10(timerResolution)))
            / ((double) timerResolution);

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
        g->members.append(members[i]);
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
    Q_UNUSED(communicator);

    // Every time we find a send, check the unmatched recvs
    // to see if it has a match
    unsigned long long converted_time = convertTime(userData, time);
    int sender = ((OTF2Importer *) userData)->locationIndexMap->value(locationID);
    OTF2Comm * comm = ((OTF2Importer *) userData)->commMap->value(communicator);
    OTF2Group * group = ((OTF2Importer *) userData)->groupMap->value(comm->group);
    int world_receiver = group->members.at(receiver);
    CommRecord * cr = NULL;
    QLinkedList<CommRecord *> * unmatched = (*(((OTF2Importer *) userData)->unmatched_recvs))[sender];
    for (QLinkedList<CommRecord *>::Iterator itr = unmatched->begin();
         itr != unmatched->end(); ++itr)
    {
        if (OTF2Importer::compareComms((*itr), sender, world_receiver, msgTag, msgLength))
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
        cr = new CommRecord(sender, converted_time, world_receiver, 0, msgLength, msgTag);
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
    Q_UNUSED(communicator);
    Q_UNUSED(requestID);

    // Every time we find a send, check the unmatched recvs
    // to see if it has a match
    time = convertTime(userData, time);
    int sender = ((OTF2Importer *) userData)->locationIndexMap->value(locationID);
    CommRecord * cr = NULL;
    QLinkedList<CommRecord *> * unmatched = (*(((OTF2Importer *) userData)->unmatched_recvs))[sender];
    for (QLinkedList<CommRecord *>::Iterator itr = unmatched->begin();
         itr != unmatched->end(); ++itr)
    {
        if (OTF2Importer::compareComms((*itr), sender, receiver, msgTag, msgLength))
        {
            cr = *itr;
            cr->send_time = time;
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
        cr = new CommRecord(sender, time, receiver, 0, msgLength, msgTag);
        (*((((OTF2Importer*) userData)->rawtrace)->messages))[sender]->append(cr);
        (*(((OTF2Importer *) userData)->unmatched_sends))[sender]->append(cr);
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
    Q_UNUSED(locationID);
    Q_UNUSED(time);
    Q_UNUSED(userData);
    Q_UNUSED(attributeList);
    Q_UNUSED(requestID);

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
    Q_UNUSED(communicator);

    // Look for match in unmatched_sends
    unsigned long long converted_time = convertTime(userData, time);
    int receiver = ((OTF2Importer *) userData)->locationIndexMap->value(locationID);
    OTF2Comm * comm = ((OTF2Importer *) userData)->commMap->value(communicator);
    OTF2Group * group = ((OTF2Importer *) userData)->groupMap->value(comm->group);
    int world_sender = group->members.at(sender);
    CommRecord * cr = NULL;
    QLinkedList<CommRecord *> * unmatched = (*(((OTF2Importer*) userData)->unmatched_sends))[world_sender];
    for (QLinkedList<CommRecord *>::Iterator itr = unmatched->begin();
         itr != unmatched->end(); ++itr)
    {
        if (OTF2Importer::compareComms((*itr), world_sender, receiver,
                                       msgTag, msgLength))
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
        cr = new CommRecord(world_sender, 0, receiver, converted_time, msgLength, msgTag);
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
    Q_UNUSED(communicator);
    Q_UNUSED(requestID);

    // Look for match in unmatched_sends
    uint64_t converted_time = convertTime(userData, time);
    int receiver = ((OTF2Importer *) userData)->locationIndexMap->value(locationID);
    CommRecord * cr = NULL;
    QLinkedList<CommRecord *> * unmatched = (*(((OTF2Importer*) userData)->unmatched_sends))[sender];
    for (QLinkedList<CommRecord *>::Iterator itr = unmatched->begin();
         itr != unmatched->end(); ++itr)
    {
        if (OTF2Importer::compareComms((*itr), sender, receiver,
                                       msgTag, msgLength))
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
        cr = new CommRecord(sender, 0, receiver, converted_time, msgLength, msgTag);
        ((*(((OTF2Importer*) userData)->unmatched_recvs))[sender])->append(cr);
    }
    (*((((OTF2Importer*) userData)->rawtrace)->messages_r))[receiver]->append(cr);

    return OTF2_CALLBACK_SUCCESS;
}




OTF2_CallbackCode OTF2Importer::callbackMPICollectiveBegin(OTF2_LocationRef locationID,
                                                           OTF2_TimeStamp time,
                                                           void * userData,
                                                           OTF2_AttributeList * attributeList)
{
    Q_UNUSED(locationID);
    Q_UNUSED(time);
    Q_UNUSED(userData);
    Q_UNUSED(attributeList);
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

    // Convert rootProc to 0..p-1 space if it truly is a root and not unrooted value
    //if (root > 0)
    //    root--;
    //int process = ((OTF2Importer *) userData)->locationIndexMap->value(locationID);
    //int comm = ((OTF2Importer *) userData)->commIndexMap->value(communicator);

    // Create collective record if it doesn't yet exist
    /*if (!(*(((OTF2Importer *) userData)->collectives)).contains(matchingId))
        (*(((OTF2Importer *) userData)->collectives))[matchingId]
            = new CollectiveRecord(matchingId, root, collective, comm);

    // Get the matching collective record
    CollectiveRecord * cr = (*(((OTF2Importer *) userData)->collectives))[matchingId];

    // Map process/time to the collective record
    time = convertTime(userData, time);
    (*(*(((OTF2Importer *) userData)->collectiveMap))[process])[time] = cr;
    */

    return OTF2_CALLBACK_SUCCESS;
}

