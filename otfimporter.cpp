#include "otfimporter.h"
#include <QString>
#include <iostream>

OTFImporter::OTFImporter(const char* otf_file) : filename(otf_file)
{
    rawtrace = new RawTrace();
}

OTFImporter::~OTFImporter()
{
    //delete rawtrace;
}

RawTrace * OTFImporter::importOTF()
{
    readRawTrace();

    // convertRawTrace() ? <-- should be different class to convert

    // For now
    return rawtrace;
}

void OTFImporter::readRawTrace()
{
    fileManager = OTF_FileManager_open(1);
    otfReader = OTF_Reader_open(filename, fileManager);
    handlerArray = OTF_HandlerArray_open();

    setHandlers();
    OTF_Reader_readDefinitions(otfReader, handlerArray);

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


int OTFImporter::handleDefTimerResolution(void* userData, uint32_t stream, uint64_t ticksPerSecond)
{
    Q_UNUSED(stream);
    ((OTFImporter*) userData)->ticks_per_second = ticksPerSecond;
    std::cout << ((OTFImporter*) userData)->ticks_per_second << " is ticks_per_second" << std::endl;
    return 0;
}

int OTFImporter::handleDefFunction(void * userData, uint32_t stream, uint32_t func,
                             const char* name, uint32_t funcGroup, uint32_t source)
{
    Q_UNUSED(stream);
    Q_UNUSED(funcGroup);
    Q_UNUSED(source);

    std::cout << func << " : " << name << " - " << funcGroup << std::endl;
    (*((((OTFImporter*) userData)->rawtrace)->functions))[func] = QString(name);
    return 0;
}

int OTFImporter::handleDefProcess(void * userData, uint32_t stream, uint32_t process,
                            const char* name, uint32_t parent)
{
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
    return 0;
}

int OTFImporter::handleLeave(void * userData, uint64_t time, uint32_t function,
                       uint32_t process, uint32_t source)
{
    return 0;
}

int OTFImporter::handleSend(void * userData, uint64_t time, uint32_t sender, uint32_t receiver,
                      uint32_t group, uint32_t type, uint32_t length, uint32_t source)
{
    return 0;
}
int OTFImporter::handleRecv(void * userData, uint64_t time, uint32_t receiver, uint32_t sender,
                      uint32_t group, uint32_t type, uint32_t length, uint32_t source)
{
    return 0;
}

int OTFImporter::handleCounter(void * userData, uint64_t time, uint32_t process, uint32_t counter,
                         uint64_t value)
{
    return 0;
}
