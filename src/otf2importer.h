#ifndef OTF2IMPORTER_H
#define OTF2IMPORTER_H

#include <otf2/otf2.h>
#include <QLinkedList>
#include <QString>
#include <QMap>
#include <QVector>

class CommRecord;
class RawTrace;
class Function;
class Task;
class TaskGroup;
class OTFCollective;
class Counter;
class CollectiveRecord;
class OTFImportOptions;

class OTF2Importer
{
public:
    OTF2Importer();
    ~OTF2Importer();
    RawTrace * importOTF2(const char* otf_file, bool _enforceMessageSize);

    class OTF2Attribute {
    public:
        OTF2Attribute(OTF2_AttributeRef _self,
                      OTF2_StringRef _name,
                      OTF2_StringRef _desc,
                      OTF2_Type _type)
            : self(_self), name(_name), description(_desc), type(_type) {}

        OTF2_AttributeRef self;
        OTF2_StringRef name;
        OTF2_StringRef description;
        OTF2_Type type;
    };

    class OTF2IsendComplete {
    public:
        OTF2IsendComplete(uint64_t _time, uint64_t _request)
            : time(_time), request(_request) {}

        uint64_t time;
        uint64_t request;
    };

    class OTF2CollectiveFragment {
    public:
        OTF2CollectiveFragment(uint64_t _time, OTF2_CollectiveOp _op,
                               OTF2_CommRef _comm, uint32_t _root)
            : time(_time), op(_op), comm(_comm), root(_root) {}

        uint64_t time;
        OTF2_CollectiveOp op;
        OTF2_CommRef comm;
        uint32_t root;
    };

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
                     OTF2_LocationType _type,
                     uint64_t _num,
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

    class OTF2Group {
    public:
        OTF2Group(OTF2_GroupRef _self,
                  OTF2_StringRef _name,
                  OTF2_GroupType _type,
                  OTF2_Paradigm _paradigm,
                  OTF2_GroupFlag _flags)
            : self(_self), name(_name), type(_type),
              paradigm(_paradigm), flags(_flags),
              members(new QList<uint32_t>()) {}

        OTF2_GroupRef self;
        OTF2_StringRef name;
        OTF2_GroupType type;
        OTF2_Paradigm paradigm;
        OTF2_GroupFlag flags;
        QList<uint32_t> * members;
    };


    // Callbacks per OTF2

    static OTF2_CallbackCode callbackDefClockProperties(void * userData,
                                                        uint64_t timerResolution,
                                                        uint64_t globalOffset,
                                                        uint64_t traceLength);
    static OTF2_CallbackCode callbackDefString(void * userData,
                                               OTF2_StringRef self,
                                               const char * string);
    static OTF2_CallbackCode callbackDefAttribute(void * userData,
                                                  OTF2_AttributeRef self,
                                                  OTF2_StringRef name,
                                                  OTF2_StringRef description,
                                                  OTF2_Type type);
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
    static OTF2_CallbackCode callbackDefGroup(void* userData,
                                              OTF2_GroupRef self,
                                              OTF2_StringRef name,
                                              OTF2_GroupType groupType,
                                              OTF2_Paradigm paradigm,
                                              OTF2_GroupFlag groupFlags,
                                              uint32_t numberOfMembers,
                                              const uint64_t* members );

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
                                              uint64_t msgLength,
                                              uint64_t requestID);
    static OTF2_CallbackCode callbackMPIIsendComplete(OTF2_LocationRef locationID,
                                                      OTF2_TimeStamp time,
                                                      void * userData,
                                                      OTF2_AttributeList * attributeList,
                                                      uint64_t requestID);
    static OTF2_CallbackCode callbackMPIIrecvRequest(OTF2_LocationRef locationID,
                                                     OTF2_TimeStamp time,
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
                                              uint64_t msgLength,
                                              uint64_t requestID);
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
    static bool compareComms(CommRecord * comm, unsigned int sender,
                             unsigned int receiver, unsigned int tag);


    static uint64_t convertTime(void* userData, OTF2_TimeStamp time);

    QString from_saved_version;
    unsigned long long int ticks_per_second;
    unsigned long long int time_offset;
    double time_conversion_factor;
    int num_processes;
    int second_magnitude;

    int entercount;
    int exitcount;
    int sendcount;
    int recvcount;


private:
    void processDefinitions();
    void setDefCallbacks();
    void setEvtCallbacks();
    void processCollectives();

    bool enforceMessageSize;

    OTFImportOptions * options;
    OTF2_Reader * otfReader;
    OTF2_GlobalDefReaderCallbacks * global_def_callbacks;
    OTF2_GlobalEvtReaderCallbacks * global_evt_callbacks;

    QMap<OTF2_StringRef, QString> * stringMap;
    QMap<OTF2_AttributeRef, OTF2Attribute *> * attributeMap;
    QMap<OTF2_LocationRef, OTF2Location *> * locationMap;
    QMap<OTF2_LocationGroupRef, OTF2LocationGroup *> * locationGroupMap;
    QMap<OTF2_RegionRef, OTF2Region *> * regionMap;
    QMap<OTF2_CommRef, OTF2Comm *> * commMap;
    QMap<OTF2_GroupRef, OTF2Group *> * groupMap;

    QMap<OTF2_CommRef, int> * commIndexMap;
    QMap<OTF2_RegionRef, int> * regionIndexMap;
    QMap<OTF2_LocationRef, int> * locationIndexMap;

    QVector<QLinkedList<CommRecord *> *> * unmatched_recvs;
    QVector<QLinkedList<CommRecord *> *> * unmatched_sends;
    QVector<QLinkedList<CommRecord *> *> * unmatched_send_requests;
    QVector<QLinkedList<OTF2IsendComplete *> *> * unmatched_send_completes;

    RawTrace * rawtrace;

    QMap<int, Task *> * tasks;
    QMap<int, QString> * functionGroups;
    QMap<int, Function *> * functions;
    QMap<int, TaskGroup *> * taskgroups;
    QMap<int, OTFCollective *> * collective_definitions;
    QMap<unsigned int, Counter *> * counters;

    QMap<unsigned long long, CollectiveRecord *> * collectives; // matchingId to CR <-- REMOVE ME
    QVector<QMap<unsigned long long, CollectiveRecord *> *> * collectiveMap; // process/time to CR

    QVector<QLinkedList<uint64_t> *> * collective_begins;
    QVector<QLinkedList<OTF2CollectiveFragment *> *> * collective_fragments;

    QList<OTF2_AttributeRef> metrics;
    QList<QString> * metric_names;
    OTF2_AttributeRef stepRef;
    OTF2_AttributeRef phaseRef;
};

#endif // OTF2IMPORTER_H
