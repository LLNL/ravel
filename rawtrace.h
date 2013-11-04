#ifndef RAWTRACE_H
#define RAWTRACE_H

#include "eventrecord.h"
#include "commrecord.h"
#include <QMap>
#include <QVector>

// Trace from OTF

class RawTrace
{
public:
    RawTrace();
    ~RawTrace();

    QMap<int, QString> * functions;
    QVector<EventRecord *> * events;
    QVector<CommRecord *> * messages;
    int num_processes;
};

#endif // RAWTRACE_H
