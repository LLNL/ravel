#ifndef RAWTRACE_H
#define RAWTRACE_H

#include <QString>
#include <QMap>
#include <QVector>
#include <stdint.h>

class CollectiveRecord;
class Task;
class Function;
class EventRecord;
class CommRecord;
class TaskGroup;
class OTFCollective;
class Counter;
class CounterRecord;
class CollectiveRecord;
class OTFImportOptions;

// Trace from OTF without processing
class RawTrace
{
public:
    RawTrace(int nt);
    ~RawTrace();

    class CollectiveBit {
    public:
        CollectiveBit(uint64_t _time, CollectiveRecord * _cr)
            : time(_time), cr(_cr) {}

        uint64_t time;
        CollectiveRecord * cr;
    };

    OTFImportOptions * options;
    QMap<int, Task *> * tasks;
    QMap<int, QString> * functionGroups;
    QMap<int, Function *> * functions;
    QVector<QVector<EventRecord *> *> * events;
    QVector<QVector<CommRecord *> *> * messages;
    QVector<QVector<CommRecord *> *> * messages_r; // by receiver instead of sender
    QMap<int, TaskGroup *> * taskgroups;
    QMap<int, OTFCollective *> * collective_definitions;
    QMap<unsigned int, Counter *> * counters;
    QVector<QVector<CounterRecord * > *> * counter_records;

    QMap<unsigned long long, CollectiveRecord *> * collectives;
    QVector<QMap<unsigned long long, CollectiveRecord *> *> * collectiveMap;
    QVector<QVector<CollectiveBit *> *> * collectiveBits;
    int num_tasks;
    int second_magnitude; // seconds are 10^this over the smallest smaple unit
    QString from_saved_version;
    QList<QString> * metric_names;
};

#endif // RAWTRACE_H
