#ifndef TRACE_H
#define TRACE_H

#include "event.h"
#include "function.h"
#include "partition.h"
#include "otfimportoptions.h"
#include <QMap>
#include <QVector>
#include <QStack>
#include <QSet>

class Trace
{
public:
    Trace(int np);
    Trace(int np, bool legacy = false);
    ~Trace();

    void preprocess(OTFImportOptions * _options);
    void partition();
    void assignSteps();
    void printStats();

    int num_processes;
    int units;

    QList<Partition *> * partitions;

    // Processing options
    bool isLegacy; // from Python-build JSON
    OTFImportOptions options;

    // Below set by OTFConverter
    QMap<int, QString> * functionGroups;
    QMap<int, Function *> * functions;
    QVector<QVector<Event *> *> * events;
    QVector<QVector<Event *> *> * roots; // Roots of call trees per process

    QVector<QList<Event *> *> * mpi_events;
    QList<Event *> * send_events;

    int mpi_group;

    int global_max_step; // largest global step
    QList<Partition * > * dag_entries; // Leap 0 in the dag
    QMap<int, QSet<Partition *> *> * dag_step_dict; // Map from leap to partition

private:
    // Link the comm events together by order
    void chainCommEvents();

    // Partitioning process
    void partitionByPhase();
    void initializePartitions();
    void initializePartitionsWaitall();
    void mergeForMessages();
    void mergeCycles();
    void mergeByLeap();
    class RecurseInfo {  // For Tarjan
    public:
        RecurseInfo(Partition * p, Partition * c, QList<Partition *> * cc, int i)
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
                            QList<Partition *> * children, int cIndex,
                            QStack<RecurseInfo *> * recurse, QList<QList<Partition *> *> * components);
    int strong_connect_iter(Partition * partition, QStack<Partition *> * stack,
                            QList<QList<Partition *> *> * components, int index);
    QList<QList<Partition *> *> * tarjan();
    void set_global_steps();
    void calculate_lateness();
    void output_graph(QString filename);


    bool isProcessed; // Partitions exist

    // TODO: Replace this terrible stuff with QSharedPointer
    QSet<RecurseInfo *> * riTracker;
    QSet<QList<Partition *> *> * riChildrenTracker;
};

#endif // TRACE_H
