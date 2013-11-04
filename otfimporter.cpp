#include "otfimporter.h"
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
    OTF_HandlerArray_setHandler(handlerArray,
                                (OTF_FunctionPointer*) &OTFImporter::handleDefTimerResolution,
                                OTF_DEFTIMERRESOLUTION_RECORD);
    OTF_HandlerArray_setFirstHandlerArg(handlerArray, this, OTF_DEFTIMERRESOLUTION_RECORD);
}


int OTFImporter::handleDefTimerResolution(void* userData, uint32_t stream, uint64_t ticksPerSecond)
{
    Q_UNUSED(stream);
    ((OTFImporter*) userData)->ticks_per_second = ticksPerSecond;
    std::cout << ((OTFImporter*) userData)->ticks_per_second << " is ticks_per_second" << std::endl;
    return 0;
}
