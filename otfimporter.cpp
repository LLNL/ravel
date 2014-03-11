#include "otfimporter.h"
#include <QString>
#include <QElapsedTimer>
#include <iostream>
#include <cmath>
#include "general_util.h"

OTFImporter::OTFImporter()
    : ticks_per_second(0),
      time_conversion_factor(0),
      num_processes(0),
      entercount(0),
      exitcount(0),
      sendcount(0),
      recvcount(0),
      fileManager(NULL),
      otfReader(NULL),
      handlerArray(NULL),
      unmatched_recvs(new QVector<QLinkedList<CommRecord *> *>()),
      unmatched_sends(new QVector<QLinkedList<CommRecord *> *>()),
      rawtrace(NULL),
      functionGroups(NULL),
      functions(NULL),
      collectiveMap(new QMap<std::pair<uint64_t, uint32_t>, CollectiveRecord *>())
{

}

OTFImporter::~OTFImporter()
{
    //delete rawtrace;

    for (QVector<QLinkedList<CommRecord *> *>::Iterator eitr = unmatched_recvs->begin(); eitr != unmatched_recvs->end(); ++eitr) {
        for (QLinkedList<CommRecord *>::Iterator itr = (*eitr)->begin(); itr != (*eitr)->end(); ++itr) {
            delete *itr;
            *itr = NULL;
        }
        delete *eitr;
        *eitr = NULL;
    }
    delete unmatched_recvs;

    // Don't delete the CommRecords as they are now part of messages in the raw trace
    for (QVector<QLinkedList<CommRecord *> *>::Iterator eitr = unmatched_sends->begin(); eitr != unmatched_sends->end(); ++eitr) {
        delete *eitr;
        *eitr = NULL;
    }
    delete unmatched_sends;

    delete collectiveMap;
}

RawTrace * OTFImporter::importOTF(const char* otf_file)
{

    entercount = 0;
    exitcount = 0;
    sendcount = 0;
    recvcount = 0;

    QElapsedTimer traceTimer;
    qint64 traceElapsed;

    traceTimer.start();

    fileManager = OTF_FileManager_open(1);
    otfReader = OTF_Reader_open(otf_file, fileManager);
    handlerArray = OTF_HandlerArray_open();

    setHandlers();

    functionGroups = new QMap<int, QString>();
    functions = new QMap<int, Function *>();

    std::cout << "Reading definitions" << std::endl;
    OTF_Reader_readDefinitions(otfReader, handlerArray);

    rawtrace = new RawTrace(num_processes);
    delete rawtrace->functions;
    rawtrace->functions = functions;
    delete rawtrace->functionGroups;
    rawtrace->functionGroups = functionGroups;

    delete unmatched_recvs;
    unmatched_recvs = new QVector<QLinkedList<CommRecord *> *>(num_processes);
    delete unmatched_sends;
    unmatched_sends = new QVector<QLinkedList<CommRecord *> *>(num_processes);
    for (int i = 0; i < num_processes; i++) {
        (*unmatched_recvs)[i] = new QLinkedList<CommRecord *>();
        (*unmatched_sends)[i] = new QLinkedList<CommRecord *>();
    }

    std::cout << "Reading events" << std::endl;
    OTF_Reader_readEvents(otfReader, handlerArray);

    OTF_HandlerArray_close(handlerArray);
    OTF_Reader_close(otfReader);
    OTF_FileManager_close(fileManager);
    std::cout << "Finish reading" << std::endl;

    int unmatched_recv_count = 0;
    for (QVector<QLinkedList<CommRecord *> *>::Iterator eitr = unmatched_recvs->begin(); eitr != unmatched_recvs->end(); ++eitr) {
        for (QLinkedList<CommRecord *>::Iterator itr = (*eitr)->begin(); itr != (*eitr)->end(); ++itr) {
            unmatched_recv_count++;
            std::cout << "Unmatched recv " << (*itr)->sender << "->" << (*itr)->receiver << " (" << (*itr)->send_time << ", " << (*itr)->recv_time << ")" << std::endl;
        }
    }
    int unmatched_send_count = 0;
    for (QVector<QLinkedList<CommRecord *> *>::Iterator eitr = unmatched_sends->begin(); eitr != unmatched_sends->end(); ++eitr) {
        for (QLinkedList<CommRecord *>::Iterator itr = (*eitr)->begin(); itr != (*eitr)->end(); ++itr) {
            unmatched_recv_count++;
            std::cout << "Unmatched SEND " << (*itr)->sender << "->" << (*itr)->receiver << " (" << (*itr)->send_time << ", " << (*itr)->recv_time << ")" << std::endl;
        }
    }
    std::cout << unmatched_send_count << " unmatched sends and " << unmatched_recv_count << " unmatched recvs." << std::endl;


    traceElapsed = traceTimer.nsecsElapsed();
    std::cout << "OTF Reading: ";
    gu_printTime(traceElapsed);
    std::cout << std::endl;

    return rawtrace;
}

void OTFImporter::setHandlers()
{
    // Timer
    OTF_HandlerArray_setHandler(handlerArray,
                                (OTF_FunctionPointer*) &OTFImporter::handleDefTimerResolution,
                                OTF_DEFTIMERRESOLUTION_RECORD);
    OTF_HandlerArray_setFirstHandlerArg(handlerArray, this, OTF_DEFTIMERRESOLUTION_RECORD);

    // Function Groups
    OTF_HandlerArray_setHandler(handlerArray,
                                (OTF_FunctionPointer*) &OTFImporter::handleDefFunctionGroup,
                                OTF_DEFFUNCTIONGROUP_RECORD);
    OTF_HandlerArray_setFirstHandlerArg(handlerArray, this, OTF_DEFFUNCTIONGROUP_RECORD);

    // Function Names
    OTF_HandlerArray_setHandler(handlerArray,
                                (OTF_FunctionPointer*) &OTFImporter::handleDefFunction,
                                OTF_DEFFUNCTION_RECORD);
    OTF_HandlerArray_setFirstHandlerArg(handlerArray, this, OTF_DEFFUNCTION_RECORD);

    // Process Info
    OTF_HandlerArray_setHandler(handlerArray,
                                (OTF_FunctionPointer*) &OTFImporter::handleDefProcess,
                                OTF_DEFPROCESS_RECORD);
    OTF_HandlerArray_setFirstHandlerArg(handlerArray, this, OTF_DEFPROCESS_RECORD);

    // Counter Names
    OTF_HandlerArray_setHandler(handlerArray,
                                (OTF_FunctionPointer*) &OTFImporter::handleDefCounter,
                                OTF_DEFCOUNTER_RECORD);
    OTF_HandlerArray_setFirstHandlerArg(handlerArray, this, OTF_DEFCOUNTER_RECORD);

    // Enter & Leave
    OTF_HandlerArray_setHandler(handlerArray,
                                (OTF_FunctionPointer*) &OTFImporter::handleEnter,
                                OTF_ENTER_RECORD);
    OTF_HandlerArray_setFirstHandlerArg(handlerArray, this, OTF_ENTER_RECORD);

    OTF_HandlerArray_setHandler(handlerArray,
                                (OTF_FunctionPointer*) &OTFImporter::handleLeave,
                                OTF_LEAVE_RECORD);
    OTF_HandlerArray_setFirstHandlerArg(handlerArray, this, OTF_LEAVE_RECORD);

    // Send & Receive
    OTF_HandlerArray_setHandler(handlerArray,
                                (OTF_FunctionPointer*) &OTFImporter::handleSend,
                                OTF_SEND_RECORD);
    OTF_HandlerArray_setFirstHandlerArg(handlerArray, this, OTF_SEND_RECORD);

    OTF_HandlerArray_setHandler(handlerArray,
                                (OTF_FunctionPointer*) &OTFImporter::handleRecv,
                                OTF_RECEIVE_RECORD);
    OTF_HandlerArray_setFirstHandlerArg(handlerArray, this, OTF_RECEIVE_RECORD);

    // Counter Value
    OTF_HandlerArray_setHandler(handlerArray,
                                (OTF_FunctionPointer*) &OTFImporter::handleCounter,
                                OTF_COUNTER_RECORD);
    OTF_HandlerArray_setFirstHandlerArg(handlerArray, this, OTF_COUNTER_RECORD);

    // Collectives
    /*
    OTF_HandlerArray_setHandler(handlerArray,
                                (OTF_FunctionPointer*) &OTFImporter::handleDefProcessGroup,
                                OTF_DEFPROCESSGROUP_RECORD);
    OTF_HandlerArray_setFirstHandlerArg(handlerArray, this, OTF_DEFPROCESSGROUP_RECORD);

    OTF_HandlerArray_setHandler(handlerArray,
                                (OTF_FunctionPointer*) &OTFImporter::handleDefCollectiveOperation,
                                OTF_DEFCOLLOP_RECORD);
    OTF_HandlerArray_setFirstHandlerArg(handlerArray, this, OTF_DEFCOLLOP_RECORD);
    */

    /* Doesn't seem to be used anymore
    OTF_HandlerArray_setHandler(handlerArray,
                                (OTF_FunctionPointer*) &OTFImporter::handleCollectiveOperation,
                                OTF_COLLOP_RECORD);
    OTF_HandlerArray_setFirstHandlerArg(handlerArray, this, OTF_COLLOP_RECORD);
    */

    /*
    OTF_HandlerArray_setHandler(handlerArray,
                                (OTF_FunctionPointer*) &OTFImporter::handleBeginCollectiveOperation,
                                OTF_BEGINCOLLOP_RECORD);
    OTF_HandlerArray_setFirstHandlerArg(handlerArray, this, OTF_BEGINCOLLOP_RECORD);


    OTF_HandlerArray_setHandler(handlerArray,
                                (OTF_FunctionPointer*) &OTFImporter::handleEndCollectiveOperation,
                                OTF_ENDCOLLOP_RECORD);
    OTF_HandlerArray_setFirstHandlerArg(handlerArray, this, OTF_ENDCOLLOP_RECORD);
    */
}

uint64_t OTFImporter::convertTime(void* userData, uint64_t time) {
    return (uint64_t) ((double) time) * ((OTFImporter *) userData)->time_conversion_factor;
}

int OTFImporter::handleDefTimerResolution(void* userData, uint32_t stream, uint64_t ticksPerSecond)
{
    Q_UNUSED(stream);
    ((OTFImporter*) userData)->ticks_per_second = ticksPerSecond;

    double conversion_factor;
    conversion_factor = pow(10, (int) floor(log10(ticksPerSecond)))
            / ((double) ticksPerSecond);

    ((OTFImporter*) userData)->time_conversion_factor = conversion_factor;
    return 0;
}

int OTFImporter::handleDefFunctionGroup(void * userData, uint32_t stream, uint32_t funcGroup,
                                        const char * name)
{
    Q_UNUSED(stream);

    (*(((OTFImporter *) userData)->functionGroups))[funcGroup] = QString(name);
    return 0;
}

int OTFImporter::handleDefFunction(void * userData, uint32_t stream, uint32_t func,
                             const char* name, uint32_t funcGroup, uint32_t source)
{
    Q_UNUSED(stream);
    Q_UNUSED(source);

    (*(((OTFImporter*) userData)->functions))[func] = new Function(QString(name), funcGroup);
    return 0;
}

// Here the name seems to correspond to the MPI rank. We might want to do some
// processing to get the actual name of the process or have another map.
// For now we can go with the default numbering I guess.
int OTFImporter::handleDefProcess(void * userData, uint32_t stream, uint32_t process,
                            const char* name, uint32_t parent)
{
    Q_UNUSED(stream);
    Q_UNUSED(parent);
    Q_UNUSED(name);
    Q_UNUSED(process);

    //std::cout << process << " : " << name << std::endl;
    ((OTFImporter*) userData)->num_processes++;
    return 0;
}

int OTFImporter::handleDefCounter(void * userData, uint32_t stream, uint32_t counter,
                            const char* name, uint32_t properties, uint32_t counterGroup,
                            const char* unit)
{
    return 0;
}

int OTFImporter::handleEnter(void * userData, uint64_t time, uint32_t function,
                       uint32_t process, uint32_t source)
{
    Q_UNUSED(source);
    ((*((((OTFImporter*) userData)->rawtrace)->events))[process - 1])->append(new EventRecord(process - 1, convertTime(userData, time), function));
    //((OTFImporter*) userData)->entercount++;
    //std::cout << "Enter " << ((OTFImporter*) userData)->entercount << std::endl;
    return 0;
}

int OTFImporter::handleLeave(void * userData, uint64_t time, uint32_t function,
                       uint32_t process, uint32_t source)
{
    Q_UNUSED(source);
    ((*((((OTFImporter*) userData)->rawtrace)->events))[process - 1])->append(new EventRecord(process - 1, convertTime(userData, time), function));
    //((OTFImporter*) userData)->exitcount++;
    //std::cout << "Exit " << ((OTFImporter*) userData)->exitcount << " with time " <<  convertTime(userData, time) << std::endl;
    return 0;
}

bool OTFImporter::compareComms(CommRecord * comm, unsigned int sender, unsigned int receiver,
                         unsigned int tag, unsigned int size)
{
    if ((comm->sender != sender) || (comm->receiver != receiver)
            || (comm->tag != tag) || (comm->size != size))
        return false;
    return true;
}

// Note the send matching doesn't guarantee any particular order of the sends/receives in time. We will need to look into this.
int OTFImporter::handleSend(void * userData, uint64_t time, uint32_t sender, uint32_t receiver,
                      uint32_t group, uint32_t type, uint32_t length, uint32_t source)
{
    Q_UNUSED(source);
    Q_UNUSED(group);


    time = convertTime(userData, time);
    CommRecord * cr = NULL;
    QLinkedList<CommRecord *> * unmatched = (*(((OTFImporter *) userData)->unmatched_recvs))[sender - 1];
    for (QLinkedList<CommRecord *>::Iterator itr = unmatched->begin(); itr != unmatched->end(); ++itr)
    {
        if (OTFImporter::compareComms((*itr), sender, receiver, type, length))
        {
            cr = *itr;
            cr->send_time = time;
            ((*((((OTFImporter*) userData)->rawtrace)->messages))[sender - 1])->append((cr));
            break;
        }
    }

    //((OTFImporter*) userData)->sendcount++;
    //std::cout << "Send " << ((OTFImporter*) userData)->sendcount << std::endl;

    if (cr)
    {
        (*(((OTFImporter *) userData)->unmatched_recvs))[sender - 1]->removeOne(cr);
    }
    else
    {
        cr = new CommRecord(sender - 1, time, receiver - 1, 0, length, type);
        (*((((OTFImporter*) userData)->rawtrace)->messages))[sender - 1]->append(cr);
        (*(((OTFImporter *) userData)->unmatched_sends))[sender - 1]->append(cr);
    }
    return 0;
}


int OTFImporter::handleRecv(void * userData, uint64_t time, uint32_t receiver, uint32_t sender,
                      uint32_t group, uint32_t type, uint32_t length, uint32_t source)
{
    Q_UNUSED(source);
    Q_UNUSED(group);

    time = convertTime(userData, time);
    CommRecord * cr = NULL;
    QLinkedList<CommRecord *> * unmatched = (*(((OTFImporter*) userData)->unmatched_sends))[sender - 1];
    for (QLinkedList<CommRecord *>::Iterator itr = unmatched->begin(); itr != unmatched->end(); ++itr) {
        if (OTFImporter::compareComms((*itr), sender - 1, receiver - 1, type, length)) {
            cr = *itr;
            cr->recv_time = time;
            break;
        }
    }

    if (cr)
    {
        (*(((OTFImporter *) userData)->unmatched_sends))[sender - 1]->removeOne(cr);
    }
    else
    {
        ((*(((OTFImporter*) userData)->unmatched_recvs))[sender - 1])->append(new CommRecord(sender - 1, 0, receiver - 1, time, length, type));
    }

    //((OTFImporter*) userData)->recvcount++;
    //std::cout << "Recv " << ((OTFImporter*) userData)->recvcount << std::endl;

    return 0;
}

int OTFImporter::handleCounter(void * userData, uint64_t time, uint32_t process, uint32_t counter,
                         uint64_t value)
{
    return 0;
}

int OTFImporter::handleDefProcessGroup(void * userData, uint32_t stream, uint32_t procGroup,
                                       const char * name, uint32_t numberOfProcs, const uint32_t * procs)
{
    std::cout << "ProcessGroup: " << name << " : " << numberOfProcs << std::endl;
    return 0;
}

int OTFImporter::handleDefCollectiveOperation(void * userData, uint32_t stream, uint32_t collOp,
                                              const char * name, uint32_t type)
{
    std::cout << "DefCollective: " << collOp << " --> " << name << " : " << type << std::endl;
    return 0;
}

int OTFImporter::handleCollectiveOperation(void * userData, uint64_t time, uint32_t process,
                                           uint32_t collective, uint32_t procGroup, uint32_t rootProc,
                                           uint32_t sent, uint32_t received, uint64_t duration,
                                           uint32_t source)
{
    Q_UNUSED(source); // Location in source code

    std::cout << "Collective: " << time << ", proc: " << process << ", coll: " << collective;
    std::cout << ", root: " << rootProc << ", group: " << procGroup << ", sent: " << sent;
    std::cout << ", recv: " << received << ", dur: " << duration << std::endl;

    return 0;
}

int OTFImporter::handleBeginCollectiveOperation(void * userData, uint64_t time, uint32_t process,
                                           uint32_t collective, uint64_t matchingId, uint32_t procGroup,
                                           uint32_t rootProc, uint32_t sent, uint32_t received,
                                           uint32_t scltoken, OTF_KeyValueList * list)
{
    Q_UNUSED(list);
    Q_UNUSED(scltoken);
    std::cout << "Begin Collective: " << time << ", proc: " << process << ", coll: " << collective;
    std::cout << ", root: " << rootProc << ", group: " << procGroup << ", sent: " << sent;
    std::cout << ", recv: " << received << ", matching: " << matchingId << std::endl;
}

int OTFImporter::handleEndCollectiveOperation(void * userData, uint64_t time, uint32_t process,
                                        uint64_t matchingId, OTF_KeyValueList * list)
{
    Q_UNUSED(list);
    std::cout << "End Collective: " << time << ", proc: " << process;
    std::cout << ", matching: " << matchingId << std::endl;
    return 0;
}
