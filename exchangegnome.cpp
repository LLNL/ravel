#include "exchangegnome.h"

ExchangeGnome::ExchangeGnome()
    : Gnome(),
      type(UNKNOWN),
      metric(""),
      cluster_leaves(NULL),
      cluster_root(NULL)
{
}

ExchangeGnome::~ExchangeGnome()
{
    if (cluster_root) {
        cluster_root->delete_tree();
        delete cluster_root;
        delete cluster_leaves;
    }
}

bool ExchangeGnome::detectGnome(Partition * part)
{
    bool gnome = true;
    QSet<int> sends = QSet<int>();
    QSet<int> recvs = QSet<int>();
    for (QMap<int, QList<Event *> *>::Iterator event_list = part->events->begin();
         event_list != part->events->end(); ++event_list)
    {
        sends.clear();
        recvs.clear();
        for (QList<Event *>::Iterator evt = (event_list.value())->begin();
             evt != (event_list.value())->end(); ++evt)
        {
            for (QVector<Message *>::Iterator msg = (*evt)->messages->begin();
                 msg != (*evt)->messages->end(); ++msg)
            {
                if ((*msg)->sender == (*evt))
                {
                    recvs.insert((*msg)->receiver->process);
                }
                else
                {
                    sends.insert((*msg)->sender->process);
                }
            }
        }
        if (sends != recvs) {
            gnome = false;
            return gnome;
        }
    }
    return gnome;
}

Gnome * ExchangeGnome::create()
{
    return new ExchangeGnome();
}

void ExchangeGnome::preprocess(VisOptions * options)
{
    Gnome::preprocess(options);
    // Possibly determine type

    // Cluster vectors
}

ExchangeGnome::ExchangeType ExchangeGnome::findType()
{
    int ssrr = 0, srsr = 0, sswa = 0, unk = 0;
    for (QMap<int, QList<Event *> *>::Iterator event_list = partition->events->begin();
         event_list != partition->events->end(); ++event_list)
    {
        for (QList<Event *>::Iterator evt = (event_list.value())->begin();
             evt != (event_list.value())->end(); ++evt)
        {
            for (QVector<Message *>::Iterator msg = (*evt)->messages->begin();
                 msg != (*evt)->messages->end(); ++msg)
            {
                // ??
            }
        }
    }
}

void ExchangeGnome::findClusters()
{
    // Calculate initial distances
    QList<DistancePair> distances;
    QList<int> processes = partition->events->keys();
    if (cluster_root)
    {
        cluster_root->delete_tree();
        delete cluster_root;
        delete cluster_leaves;
    }
    cluster_leaves = new QMap<int, PartitionCluster *>();
    qSort(processes);
    int num_processes = processes.size();
    int p1, p2;
    long long int distance;
    for (int i = 0; i < num_processes; i++)
    {
        p1 = processes[i];
        cluster_leaves->insert(i, new PartitionCluster());
        cluster_leaves->value(i)->members->insert(i);
        for (int j = i + 1; j < num_processes; j++)
        {
            p2 = processes[j];
            distance = calculateMetricDistance(partition->events->value(p1),
                                               partition->events->value(p2));
            distances.append(DistancePair(distance, p1, p2));
        }
    }
    qSort(distances);

    int lastp = distances[0].p1;
    PartitionCluster * pc;
    QList<long long int> cluster_distances = QList<long long int>();
    for (int i = 0; i < distances.size(); i++)
    {
        DistancePair current = distances[i];
        if (cluster_leaves->value(current.p1)->get_root() != cluster_leaves->value(current.p2)->get_root())
        {
            pc = new PartitionCluster(current.distance,
                                      cluster_leaves->value(current.p1),
                                      cluster_leaves->value(current.p2));
            cluster_distances.append(current.distance);
            lastp = current.p1;
        }
    }
    cluster_root = cluster_leaves->value(lastp)->get_root();
}


// When calculating distance between two event lists, we only count where they both have
// an event at the same step. When one is missing at a step, that's not counted against
// the match
long long int ExchangeGnome::calculateMetricDistance(QList<Event *> * list1, QList<Event *> * list2)
{
    int index1 = 0, index2 = 0, total_matched_steps = 0;
    Event * evt1 = list1->at(0), * evt2 = list2->at(0);
    long long int total_difference = 0;

    while (evt1 && evt2)
    {
        if (evt1->step == evt2->step) // If they're equal, add their distance
        {
            total_difference += (evt1->getMetric(metric) - evt2->getMetric(metric)) * (evt1->getMetric(metric) - evt2->getMetric(metric));
            ++total_matched_steps;
        } else if (evt1->step > evt2->step) { // If not, increment steps until they match
            ++index2;
            if (index2 < list2->size())
                evt2 = list2->at(index2);
            else
                evt2 = NULL;
        } else {
            ++index1;
            if (index1 < list2->size())
                evt1 = list1->at(index1);
            else
                evt1 = NULL;
        }
    }
    return total_difference / total_matched_steps;
}

void ExchangeGnome::drawGnomeGL(QRect extents)
{
    if (options->metric != metric)
    {
        metric = options->metric;
        // Re-cluster
    }
}

void ExchangeGnome::drawGnomeQt(QPainter * painter, QRect extents)
{

}
