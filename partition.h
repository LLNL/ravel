#ifndef PARTITION_H
#define PARTITION_H

#include "event.h"
#include "general_util.h"
#include <QList>
#include <QSet>
#include <QVector>
#include <QMap>
#include <climits>

class Partition
{
public:
    Partition();
    ~Partition();
    void addEvent(Event * e);
    void deleteEvents();
    void sortEvents();
    void step();

    unsigned long long int distance(Partition * other);
    void calculate_dag_leap();

    QMap<int, QList<Event *> *> * events;
    int max_step;
    int max_global_step;
    int min_global_step;
    int dag_leap;
    //QMap<int, QList<Event *> *> * step_dict;

    // For dag / Tarjan / merging
    QSet<Partition *> * parents;
    QSet<Partition *> * children;
    QSet<Partition *> * old_parents;
    QSet<Partition *> * old_children;
    Partition * new_partition;
    int tindex;
    int lowlink;

    // For leap merge
    bool leapmark;
    QSet<Partition *> * group;

private:
    void step_receive(Message * msg);
    void finalize_steps();
    void restep();

    QList<Event *> * free_recvs;

};

#endif // PARTITION_H
