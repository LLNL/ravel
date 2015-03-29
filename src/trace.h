#ifndef TRACE_H
#define TRACE_H

#include <QObject>
#include <QString>
#include <QList>
#include <QMap>
#include <QVector>
#include <QQueue>
#include <QStack>
#include <QSharedPointer>

#include "otfimportoptions.h"

class Partition;
class Gnome;
class Event;
class CommEvent;
class Function;
class Task;
class TaskGroup;
class PrimaryTaskGroup;
class OTFCollective;
class CollectiveRecord;

class Trace : public QObject
{
    Q_OBJECT
public:
    Trace(int nt, int np);
    ~Trace();

    void preprocess(OTFImportOptions * _options);
    void preprocessFromSaved();
    void partition();
    void assignSteps();
    void gnomify();
    void mergePartitions(QList<QList<Partition *> *> * components);
    Event * findEvent(int task, unsigned long long time);

    QString name;
    QString fullpath;
    int num_tasks;
    int num_application_tasks; // debug
    int num_pes;
    int units;
    bool use_aggregates;

    QList<Partition *> * partitions;
    QList<QString> * metrics;
    QMap<QString, QString> * metric_units;
    QList<Gnome *> * gnomes;

    // Processing options
    OTFImportOptions options;

    // Below set by OTFConverter
    QMap<int, QString> * functionGroups;
    QMap<int, Function *> * functions;

    QMap<int, PrimaryTaskGroup *> * primaries;
    QMap<int, TaskGroup *> * taskgroups;
    QMap<int, OTFCollective *> * collective_definitions;

    QMap<unsigned long long, CollectiveRecord *> * collectives;
    QVector<QMap<unsigned long long, CollectiveRecord *> *> * collectiveMap;

    QVector<QVector<Event *> *> * events; // This is going to be by tasks
    QVector<QVector<Event *> *> * pe_events; // This is by pes
    QVector<QVector<Event *> *> * roots; // Roots of call trees per pe

    int mpi_group; // functionGroup index of "MPI" functions

    int global_max_step; // largest global step
    QList<Partition * > * dag_entries; // Leap 0 in the dag
    QMap<int, QSet<Partition *> *> * dag_step_dict; // Map leap to partition

    // This is for aggregate event reporting... lists all functions
    // and how much time was spent in each
    class FunctionPair {
    public:
        FunctionPair(int _f, long long int _t)
            : fxn(_f), time(_t) {}
        FunctionPair()
            : fxn(0), time(0) {}
        FunctionPair(const FunctionPair &fp)
        {
            fxn = fp.fxn;
            time = fp.time;
        }

        // Backwards for greatest to lease
        bool operator<(const FunctionPair &fp) const { return time > fp.time; }

        int fxn;
        long long int time;
    };
    QList<FunctionPair> getAggregateFunctions(CommEvent *evt);

signals:
    // This is for progress bars
    void updatePreprocess(int, QString);
    void updateClustering(int);
    void startClustering();

private:
    // Link the comm events together by order
    void chainCommEvents();

    // Partition Dag
    void set_partition_dag();
    void set_dag_steps();
    void set_dag_entries();

    // Partitioning process
    void mergeForMessages();
    void mergeForMessagesHelper(Partition * part, QSet<Partition *> * to_merge,
                                QQueue<Partition *> * to_process);
    void mergeCycles();
    void mergeByCommonCaller();
    void mergeByLeap();
    void mergeGlobalSteps(); // Use after global steps are set, needs fixing
    class RecurseInfo {  // For Tarjan
    public:
        RecurseInfo(Partition * p, Partition * c, QList<Partition *> * cc, int i)
            : part(p), child(c), children(cc), cIndex(i) {}
        Partition * part;
        Partition * child;
        QList<Partition *> * children;
        int cIndex;
    };

    // Tarjan
    void strong_connect_loop(Partition * part, QStack<Partition *> * stack,
                            QList<Partition *> * children, int cIndex,
                            QStack<QSharedPointer<RecurseInfo> > * recurse,
                             QList<QList<Partition *> *> * components);
    int strong_connect_iter(Partition * partition, QStack<Partition *> * stack,
                            QList<QList<Partition *> *> * components, int index);
    QList<QList<Partition *> *> * tarjan();

    // Steps and metrics
    void mergeForEntryRepair(bool entries = true);
    void mergeForCharmLeaps();
    void forcePartitionDag();
    void finalizeTaskEventOrder();
    void set_global_steps();
    void calculate_lateness();
    void calculate_differential_lateness(QString metric_name, QString base_name);
    void calculate_partition_lateness();
    void calculate_partition_duration();

    // For debugging
    void output_graph(QString filename, bool byparent = false);
    void verify_partitions();

    // Extra metrics somewhat for debugging
    void setGnomeMetric(Partition * part, int gnome_index);
    void addPartitionMetric();

    // Find functions inside aggregate function
    long long int getAggregateFunctionRecurse(Event * evt,
                                              QMap<int, FunctionPair> * fpMap,
                                              unsigned long long start,
                                              unsigned long long stop);

    bool isProcessed; // Partitions exist

    // TODO: Replace this terrible stuff with QSharedPointer
    //QSet<RecurseInfo *> * riTracker;
    //QSet<QList<Partition *> *> * riChildrenTracker;

    static const bool debug = false;
    static const int partition_portion = 25;
    static const int lateness_portion = 45;
    static const int steps_portion = 30;
    static const QString collectives_string;
};

#endif // TRACE_H
