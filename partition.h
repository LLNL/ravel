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
    int dag_leap;

    // For dag / Tarjan
    QSet<Partition *> * parents;
    QSet<Partition *> * children;
    QSet<Partition *> * old_parents;
    QSet<Partition *> * old_children;
    Partition * new_partition;
    int tindex;
    int lowlink;

};

#endif // PARTITION_H
