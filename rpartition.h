#ifndef RPARTITION_H
#define RPARTITION_H

#include "event.h"
#include "commevent.h"
#include "general_util.h"
#include "clusterprocess.h"
#include <QList>
#include <QSet>
#include <QVector>
#include <QMap>
#include <climits>

class Gnome;

class Partition
{
public:
    Partition();
    ~Partition();
    void addEvent(Event * e);
    void deleteEvents();
    void sortEvents();
    void step();

    // Based on step
    bool operator<(const Partition &);
    bool operator>(const Partition &);
    bool operator<=(const Partition &);
    bool operator>=(const Partition &);
    bool operator==(const Partition &);

     // Time gap between partitions
    unsigned long long int distance(Partition * other);

     // For leap merge - which children can we merge to
    void calculate_dag_leap();
    QString generate_process_string(); // For debugging

     // When we're merging, find what this merged into
    Partition * newest_partition();
    int num_events();

    // Core partition information, events per process and step summary
    QMap<int, QList<CommEvent *> *> * events;
    int max_step;
    int max_global_step;
    int min_global_step;
    int dag_leap;

    // For message merge
    bool mark;

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

    // For graph drawing
    QString gvid;

    // For gnome and clustering
    // Cluster_vectors simplify the coding of distance calculations
    // between processes but don't aid performance so may be wasteful
    Gnome * gnome;
    int gnome_type;
    QVector<ClusterProcess *> * cluster_processes;
    void makeClusterVectors(QString metric);
    QMap<int, QVector<long long int> *> * cluster_vectors;
    QMap<int, int> * cluster_step_starts;

    bool debug_mark;

private:
    // Stepping logic -- probably want to rewrite
    int set_stride_dag(QList<CommEvent *> *stride_events);
    void find_stride_child(CommEvent * base, CommEvent * evt);

    QList<Event *> * free_recvs;

};

#endif // RPARTITION_H
