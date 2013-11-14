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

    void preprocess();
    void partition();
    void assignSteps();

    QMap<int, QString> * functionGroups;
    QMap<int, Function *> * functions;
    QVector<QVector<Event *> *> * events;
    QVector<QVector<Event *> *> * roots;
    QVector<QVector<Event *> *> * comm_events;
    QList<Partition *> * partitions;
    int num_processes;

    QVector<QList<Event *> *> * mpi_events;
    QList<Event *> * send_events;

    int mpi_group;

private:
    void chainCommEvents();

    void initializePartitions();
    void initializePartitionsWaitall();
    void mergeByMessages();
    void mergeCycles();
    void mergeByLeap();
    class RecurseInfo {
    public:
        RecurseInfo(Partition * p, Partition * c, QList<QList<Partition *> *> * cc, int i)
            : part(p), child(c), children(cc), cIndex(i) {}
        Partition * part;
        Partition * child;
        QList<Partition *> * children;
        int cIndex;
    };
    void set_partition_dag();
    void set_dag_steps();
    Partition * mergePartitions(Partition * p1, Partition * p2);
    void strong_connect_loop(Partition * part, QStack<Partition *> * stack,
                            QList<QList<Partition *> *> * children, int cIndex,
                            QStack<RecurseInfo *> * recurse, QList<Partition *> * components);
    int strong_connect_iter(Partition * partition, QStack<Partition *> * stack,
                            QList<QList<Partition *> *> * components, int index);
    QList<QList<Partition *> *> * tarjan();

    bool isProcessed;
    QList<Partition * > * dag_entries;
    QMap<int, QSet<Partition *> *> * dag_step_dict;
};

#endif // TRACE_H
