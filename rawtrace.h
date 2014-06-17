#ifndef RAWTRACE_H
#define RAWTRACE_H

#include "eventrecord.h"
#include "commrecord.h"
#include "communicator.h"
#include "otfcollective.h"
#include "collectiverecord.h"
#include "function.h"
#include <QMap>
#include <QVector>

// Trace from OTF without processing
class RawTrace
{
public:
    RawTrace(int np);
    ~RawTrace();

    QMap<int, QString> * functionGroups;
    QMap<int, Function *> * functions;
    QVector<QVector<EventRecord *> *> * events;
    QVector<QVector<CommRecord *> *> * messages;
    QVector<QVector<CommRecord *> *> * messages_r; // by receiver instead of sender
    QMap<int, Communicator *> * communicators;
    QMap<int, OTFCollective *> * collective_definitions;

    QMap<unsigned long long, CollectiveRecord *> * collectives;
    QVector<QMap<unsigned long long, CollectiveRecord *> *> * collectiveMap;
    int num_processes;
};

#endif // RAWTRACE_H
