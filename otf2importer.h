#ifndef OTF2IMPORTER_H
#define OTF2IMPORTER_H

#include "rawtrace.h"
#include "commrecord.h"
#include "eventrecord.h"
#include "collectiverecord.h"
#include "communicator.h"
#include "otfcollective.h"
#include <otf2/otf2.h>
#include <QLinkedList>

class OTF2LocationGroup {
public:
    OTF2LocationGroup(OTF2_LocationGroupRef _self,
                      OTF2_StringRef _name,
                      OTF2_LocationGroupType _type,
                      OTF2_SystemTreeNodeRef _parent)
        : self(_self), name(_name), type(_type), parent(_parent) {}

    OTF2_LocationGroupRef self;
    OTF2_StringRef name;
    OTF2_LocationGroupType type;
    OTF2_SystemTreeNodeRef parent;
};

class OTF2Location {
public:
    OTF2Location(OTF2_LocationRef _self,
                 OTF2_StringRef _name,
                 uint64_t _num,
                 OTF2_LocationType _type,
                 OTF2_LocationGroupRef _group)
        : self(_self), name(_name), num_events(_num),
          type(_type), group(_group) {}

    OTF2_LocationRef self;
    OTF2_StringRef name;
    uint64_t num_events;
    OTF2_LocationType type;
    OTF2_LocationGroupRef group;
};

class OTF2Comm {
public:
    OTF2Comm(OTF2_CommRef _self,
             OTF2_StringRef _name,
             OTF2_GroupRef _group,
             OTF2_CommRef _parent)
        : self(_self), name(_name), group(_group), parent(_parent) {}

    OTF2_CommRef self;
    OTF2_StringRef name;
    OTF2_GroupRef group;
    OTF2_CommRef parent;
};

class OTF2Comm {
public:
    OTF2Comm(OTF2_CommRef _self,
             OTF2_StringRef _name,
             OTF2_GroupRef _group,
             OTF2_CommRef _parent)
        : self(_self), name(_name), group(_group), parent(_parent) {}

    OTF2_CommRef self;
    OTF2_StringRef name;
    OTF2_GroupRef group;
    OTF2_CommRef parent;
};

class OTF2Region {
public:
    OTF2Region(OTF2_RegionRef _self,
               OTF2_StringRef _name,
               OTF2_StringRef _canon,
               OTF2_RegionRole _role,
               OTF2_Paradigm _paradigm,
               OTF2_RegionFlag _flag,
               OTF2_StringRef _source,
               uint32_t _line,
               uint32_t _end)
        : self(_self), name(_name), canon(_canon), role(_role),
          paradigm(_paradigm), flag(_flag), source(_source),
          line(_line), line_end(_end) {}

    OTF2_RegionRef self;
    OTF2_StringRef name;
    OTF2_StringRef canon;
    OTF2_RegionRole role;
    OTF2_Paradigm paradigm;
    OTF2_RegionFlag flag;
    OTF2_StringRef source;
    uint32_t line;
    uint32_t line_end;
};


class OTF2Importer
{
public:
    OTF2Importer();
    ~OTF2Importer();
    RawTrace * importOTF2(const char* otf_file);

    // Callbacks per OTF2

    static OTF2_CallbackCode callbackDefClockProperties(void * userData,
                                                        unit64_t timerResolution,
                                                        uint64_t globalOffset,
                                                        uint64_t traceLength);
    static OTF2_CallbackCode callbackString(void * userData,
                                            OTF2_StringRef self,
                                            const char * string);
    static OTF2_CallbackCode callbackDefLocationGroup(void * userData,
                                                      OTF2_LocationGroupRef self,
                                                      OTF2_StringRef name,
                                                      OTF2_LocationGroupType locationGroupType,
                                                      OTF2_SystemTreeNodeRef systemTreeParent);
    static OTF2_CallbackCode callbackDefLocation(void * userData,
                                                 OTF2_LocationRef self,
                                                 OTF2_StringRef name,
                                                 OTF2_LocationType locationType,
                                                 uint64_t numberOfEvents,
                                                 OTF2_LocationGroupRef locationGroup);
    static OTF2_CallbackCode callbackDefComm(void * userData,
                                             OTF2_CommRef self,
                                             OTF2_StringRef name,
                                             OTF2_GroupRef group,
                                             OTF2_CommRef parent);
    static OTF2_CallbackCode callbackDefRegion(void * userData,
                                               OTF2_RegionRef self,
                                               OTF2_StringRef name,
                                               OTF2_StringRef canonicalName,
                                               OTF2_StringRef description,
                                               OTF2_RegionRole regionRole,
                                               OTF2_Paradigm paradigm,
                                               OTF2_RegionFlag regionFlag,
                                               OTF2_StringRef sourceFile,
                                               uint32_t beginLineNumber,
                                               uint32_t endLineNumber);

    // TODO: Metrics might be akin to counters

    static OTF2_CallbackCode callbackEnter(OTF2_LocationRef locationID,
                                           OTF2_TimeStamp time,
                                           void * userData,
                                           OTF2_AttributeList * attributeList,
                                           OTF2_RegionRef region);
    static OTF2_CallbackCode callbackLeave(OTF2_LocationRef locationID,
                                           OTF2_TimeStamp time,
                                           void * userData,
                                           OTF2_AttributeList * attributeList,
                                           OTF2_RegionRef region);
    static OTF2_CallbackCode callbackMPISend(OTF2_LocationRef locationID,
                                             OTF2_TimeStamp time,
                                             void * userData,
                                             OTF2_AttributeList * attributeList,
                                             uint32_t receiver,
                                             OTF2_CommRef communicator,
                                             uint32_t msgTag,
                                             uint64_t msgLength);
    static OTF2_CallbackCode callbackMPIIsend(OTF2_LocationRef locationID,
                                              OTF2_TimeStamp time,
                                              void * userData,
                                              OTF2_AttributeList * attributeList,
                                              uint32_t receiver,
                                              OTF2_CommRef communicator,
                                              uint32_t msgTag,
                                              uint64_t msgLength);
    static OTF2_CallbackCode callbackMPIIsendComplete(OTF2_LocationRef locationID,
                                                      OTF2_TimeStamp time,
                                                      void * userData,
                                                      OTF2_AttributeList * attributeList,
                                                      uint64_t requestID);
    static OTF2_CallbackCode callbackMPIIrecvRequest(OTF2_LocationRef locationID,
                                                     OTF2_Timestamp time,
                                                     void * userData,
                                                     OTF2_AttributeList * attributeList,
                                                     uint64_t requestID);
    static OTF2_CallbackCode callbackMPIRecv(OTF2_LocationRef locationID,
                                             OTF2_TimeStamp time,
                                             void * userData,
                                             OTF2_AttributeList * attributeList,
                                             uint32_t sender,
                                             OTF2_CommRef communicator,
                                             uint32_t msgTag,
                                             uint64_t msgLength);
    static OTF2_CallbackCode callbackMPIIrecv(OTF2_LocationRef locationID,
                                              OTF2_TimeStamp time,
                                              void * userData,
                                              OTF2_AttributeList * attributeList,
                                              uint32_t sender,
                                              OTF2_CommRef communicator,
                                              uint32_t msgTag,
                                              uint64_t msgLength);
    /*static OTF2_CallbackCode callbackMPIRequestTest(OTF2_LocationRef locationID,
                                                    OTF2_TimeStamp time,
                                                    void * userData,
                                                    OTF2_AttributeList * attributeList,
                                                    uint64_t requestID);*/
    static OTF2_CallbackCode callbackMPICollectiveBegin(OTF2_LocationRef locationID,
                                                        OTF2_TimeStamp time,
                                                        void * userData,
                                                        OTF2_AttributeList * attributeList);
    static OTF2_CallbackCode callbackMPICollectiveEnd(OTF2_LocationRef locationID,
                                                      OTF2_TimeStamp time,
                                                      void * userData,
                                                      OTF2_AttributeList * attributeList,
                                                      OTF2_CollectiveOp collectiveOp,
                                                      OTF2_CommRef communicator,
                                                      uint32_t root,
                                                      uint64_t sizeSent,
                                                      uint64_t sizeReceived);



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
    void setDefCallbacks();
    void setEvtCallbacks();

    OTF2_Reader * otfReader;
    OTF2_GlobalDefReaderCallbacks * global_def_callbacks;
    OTF2_GlobalEvtReaderCallbacks * global_evt_callbacks;

    QMap<OTF2_StringRef, QString> * stringMap;
    QMap<OTF2_LocationRef, OTF2Location *> * locationMap;
    QMap<OTF2_LocationGroupRef, OTF2LocationGroup *> * locationGroupMap;
    QMap<OTF2_RegionRef, OTF2Region *> * regionMap;
    QMap<OTF2_CommRef, OTF2Comm *> * commMap;

    QMap<OTF2_CommRef, int> * commIndexMap;
    QMap<OTF2_RegionRef, int> * regionIndexMap;
    QMap<OTF2_LocationRef, int> * locationIndexMap;

    QVector<QLinkedList<CommRecord *> *> * unmatched_recvs;
    QVector<QLinkedList<CommRecord *> *> * unmatched_sends;

    RawTrace * rawtrace;
    QMap<int, QString> * functionGroups;
    QMap<int, Function *> * functions;
    QMap<int, Communicator *> * communicators;
    QMap<int, OTFCollective *> * collective_definitions;
    QMap<unsigned int, Counter *> * counters;

    QMap<unsigned long long, CollectiveRecord *> * collectives;
    QVector<QMap<unsigned long long, CollectiveRecord *> *> * collectiveMap;
};

#endif // OTF2IMPORTER_H
