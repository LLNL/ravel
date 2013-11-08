#ifndef TRACE_H
#define TRACE_H

#include "event.h"
#include "function.h"
#include "partition.h"
#include <QMap>
#include <QVector>

class Trace
{
public:
    Trace(int np);
    ~Trace();

    QMap<int, QString> * functionGroups;
    QMap<int, Function *> * functions;
    QVector<QVector<Event *> *> * events;
    QVector<QVector<Event *> *> * roots;
    QVector<QVector<Event *> *> * comm_events;
    QVector<Partition *> * partitions;
    int num_processes;
};

#endif // TRACE_H
