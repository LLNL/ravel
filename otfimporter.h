#ifndef OTFIMPORTER_H
#define OTFIMPORTER_H

#include "rawtrace.h"
#include "commrecord.h"
#include "eventrecord.h"
#include "collectiverecord.h"
#include "otf.h"
#include <QLinkedList>

// Use OTF API to get records
class OTFImporter
{
public:
    OTFImporter();
    ~OTFImporter();
    RawTrace * importOTF(const char* otf_file);

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


    static uint64_t convertTime(void* userData, uint64_t time);

    unsigned long long int ticks_per_second;
    double time_conversion_factor;
    int num_processes;

    int entercount;
    int exitcount;
    int sendcount;
    int recvcount;

private:
    void setHandlers();

    OTF_FileManager * fileManager;
    OTF_Reader * otfReader;
    OTF_HandlerArray * handlerArray;

    QVector<QLinkedList<CommRecord *> *> * unmatched_recvs;
    QVector<QLinkedList<CommRecord *> *> * unmatched_sends;

    RawTrace * rawtrace;
    QMap<int, QString> * functionGroups;
    QMap<int, Function *> * functions;
    QMap<std::pair<uint64_t, uint32_t>, CollectiveRecord *> * collectiveMap;

};

#endif // OTFIMPORTER_H
