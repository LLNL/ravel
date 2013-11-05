#ifndef OTFIMPORTER_H
#define OTFIMPORTER_H

#include "rawtrace.h"
#include "commrecord.h"
#include "eventrecord.h"
#include "otf.h"

class OTFImporter
{
public:
    OTFImporter(const char* otf_file);
    ~OTFImporter();
    RawTrace * importOTF();

    static int handleDefTimerResolution(void * userData, uint32_t stream, uint64_t ticksPerSecond);
    static int handleDefFunction(void * userData, uint32_t stream, uint32_t func,
                                 const char* name, uint32_t funcGroup, uint32_t source);
    static int handleDefProcess(void * userData, uint32_t stream, uint32_t process,
                                const char* name, uint32_t parent);
    static int handleDefCounter(void * userData, uint32_t stream, uint32_t counter,
                                const char* name, uint32_t properties, uint32_t counterGroup,
                                const char* unit);
    static int handleEnter(void * userData, uint64_t time, uint32_t function,
                           uint32_t process, uint32_t source);
    static int handleLeave(void * userData, uint64_t time, uint32_t function,
                           uint32_t process, uint32_t source);
    static int handleSend(void * userData, uint64_t time, uint32_t sender, uint32_t receiver,
                          uint32_t group, uint32_t type, uint32_t length, uint32_t source);
    static int handleRecv(void * userData, uint64_t time, uint32_t receiver, uint32_t sender,
                          uint32_t group, uint32_t type, uint32_t length, uint32_t source);
    static int handleCounter(void * userData, uint64_t time, uint32_t process, uint32_t counter,
                             uint64_t value);

    static bool compareComms(CommRecord * comm, unsigned int sender, unsigned int receiver,
                             unsigned int tag, unsigned int size);
    static uint64_t convertTime(void* userData, uint64_t time);

    unsigned long long int ticks_per_second;
    double time_conversion_factor;
    int num_processes;

private:
    void readRawTrace();
    void setHandlers();

    const char * filename;
    OTF_FileManager * fileManager;
    OTF_Reader * otfReader;
    OTF_HandlerArray * handlerArray;

    QVector<CommRecord *> * unmatched_recvs;

    RawTrace * rawtrace;

};

#endif // OTFIMPORTER_H
