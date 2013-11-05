#include "otfimporter.h"
#include <QString>
#include <iostream>
#include <cmath>

OTFImporter::OTFImporter(const char* otf_file) : filename(otf_file)
{
    num_processes = 0;
    unmatched_recvs = new QVector<CommRecord *>();
}

OTFImporter::~OTFImporter()
{
    //delete rawtrace;

    for (QVector<CommRecord *>::Iterator itr = unmatched_recvs->begin(); itr != unmatched_recvs->end(); ++itr) {
        delete *itr;
        *itr = NULL;
    }
    delete unmatched_recvs;
}

RawTrace * OTFImporter::importOTF()
{
    rawtrace = new RawTrace();
    readRawTrace();

    return rawtrace;
}

void OTFImporter::readRawTrace()
{
    fileManager = OTF_FileManager_open(1);
    otfReader = OTF_Reader_open(filename, fileManager);
    handlerArray = OTF_HandlerArray_open();

    setHandlers();
    OTF_Reader_readDefinitions(otfReader, handlerArray);
    OTF_Reader_readEvents(otfReader, handlerArray);

    OTF_HandlerArray_close(handlerArray);
    OTF_Reader_close(otfReader);
    OTF_FileManager_close(fileManager);
}

void OTFImporter::setHandlers()
{
    // Timer
    OTF_HandlerArray_setHandler(handlerArray,
                                (OTF_FunctionPointer*) &OTFImporter::handleDefTimerResolution,
                                OTF_DEFTIMERRESOLUTION_RECORD);
    OTF_HandlerArray_setFirstHandlerArg(handlerArray, this, OTF_DEFTIMERRESOLUTION_RECORD);

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

int OTFImporter::handleDefFunction(void * userData, uint32_t stream, uint32_t func,
                             const char* name, uint32_t funcGroup, uint32_t source)
{
    Q_UNUSED(stream);
    Q_UNUSED(funcGroup);
    Q_UNUSED(source);

    (*((((OTFImporter*) userData)->rawtrace)->functions))[func] = QString(name);
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
    ((((OTFImporter*) userData)->rawtrace)->events)->append(new EventRecord(process, convertTime(userData, time), function));
    return 0;
}

int OTFImporter::handleLeave(void * userData, uint64_t time, uint32_t function,
                       uint32_t process, uint32_t source)
{
    Q_UNUSED(source);
    ((((OTFImporter*) userData)->rawtrace)->events)->append(new EventRecord(process, convertTime(userData, time), function));
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
    bool matchFound = false;
    QVector<CommRecord *> * unmatched = ((OTFImporter *) userData)->unmatched_recvs;
    for (QVector<CommRecord *>::Iterator itr = unmatched->begin(); itr != unmatched->end(); ++itr) {
        if (!(*itr)->matched && OTFImporter::compareComms((*itr), sender, receiver, type, length)) {
            (*itr)->matched = true;
            (*itr)->send_time = time;
            matchFound = true;
            ((((OTFImporter*) userData)->rawtrace)->messages)->append((new CommRecord(sender, time, receiver, (*itr)->recv_time, length, type)));
            break;
        }
    }

    if (!matchFound)
        ((((OTFImporter*) userData)->rawtrace)->messages)->append(new CommRecord(sender, time, receiver, 0, length, type));
    return 0;
}
int OTFImporter::handleRecv(void * userData, uint64_t time, uint32_t receiver, uint32_t sender,
                      uint32_t group, uint32_t type, uint32_t length, uint32_t source)
{
    Q_UNUSED(source);
    Q_UNUSED(group);

    time = convertTime(userData, time);
    bool matchFound = false;
    QVector<CommRecord *> * unmatched = (((OTFImporter*) userData)->rawtrace)->messages;
    for (QVector<CommRecord *>::Iterator itr = unmatched->begin(); itr != unmatched->end(); ++itr) {
        if (!(*itr)->matched && OTFImporter::compareComms((*itr), sender, receiver, type, length)) {
            (*itr)->recv_time = time;
            (*itr)->matched = true;
            matchFound = true;
            break;
        }
    }

    if (!matchFound)
        (((OTFImporter*) userData)->unmatched_recvs)->append(new CommRecord(sender, 0, receiver, time, length, type));

    return 0;
}

int OTFImporter::handleCounter(void * userData, uint64_t time, uint32_t process, uint32_t counter,
                         uint64_t value)
{
    return 0;
}
