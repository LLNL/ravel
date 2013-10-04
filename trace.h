#ifndef TRACE_H
#define TRACE_H

#include "event.h"
#include <QMap>
#include <QVector>

class Trace
{
public:
    Trace(int np);
    ~Trace();
    int addEvent(Event * e);

    QMap<int, QString> * functions;
    QVector<Event *> * events;
    int num_processes;
};

#endif // TRACE_H
