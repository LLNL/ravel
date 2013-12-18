#ifndef RAWTRACE_H
#define RAWTRACE_H

#include "eventrecord.h"
#include "commrecord.h"
#include "function.h"
#include <QMap>
#include <QVector>

// Trace from OTF

class RawTrace
{
public:
    RawTrace(int np);
    ~RawTrace();

    QMap<int, QString> * functionGroups;
    QMap<int, Function *> * functions;
    QVector<QVector<EventRecord *> *> * events;
    QVector<QVector<CommRecord *> *> * messages;
    int num_processes;
};

#endif // RAWTRACE_H
