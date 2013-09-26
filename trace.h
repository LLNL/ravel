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

private:
    int num_processes;
    QVector<Event *> * events;
};

#endif // TRACE_H
