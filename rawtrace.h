#ifndef RAWTRACE_H
#define RAWTRACE_H

#include "task.h"
#include "eventrecord.h"
#include "commrecord.h"
#include "taskgroup.h"
#include "otfcollective.h"
#include "collectiverecord.h"
#include "function.h"
#include "counter.h"
#include "counterrecord.h"
#include <QMap>
#include <QVector>

// Trace from OTF without processing
class RawTrace
{
public:
    RawTrace(int nt);
    ~RawTrace();

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
    int num_tasks;
    int second_magnitude; // seconds are 10^this over the smallest smaple unit
};

#endif // RAWTRACE_H
