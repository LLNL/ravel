#ifndef PARTITION_H
#define PARTITION_H

#include "event.h"
#include "general_util.h"
#include <QVector>
#include <QMap>

class Partition
{
public:
    Partition();
    void addEvent(Event * e);
    void deleteEvents();
    void sortEvents();

    QMap<QVector<Event *> *> * events;
    int min_step;
    int max_step;
    int dag_level;

    QMap<int, Partition * > * next;
    QMap<int, Partition * > *  prev;

    // For dag / Tarjan
    int tindex;
    int lowlink;

};

#endif // PARTITION_H
