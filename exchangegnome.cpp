#include "exchangegnome.h"
#include <iostream>
#include <climits>

ExchangeGnome::ExchangeGnome()
    : Gnome(),
      type(UNKNOWN),
      metric("Lateness"),
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
            Message * msg = (*evt)->messages->at(0); // For now only first matters, rest will be the same
            if (msg->sender == (*evt))
                recvs.insert(msg->receiver->process);
            else
                sends.insert(msg->sender->process);
            /*for (QVector<Message *>::Iterator msg = (*evt)->messages->begin();
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
            }*/
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

void ExchangeGnome::preprocess()
{
    // Possibly determine type
    type = findType();
    std::cout << "Type is " << type << std::endl;

    // Cluster vectors
    findClusters();
}

// Need to add even/odd check so we can tell if we have somethign essentially srsr
ExchangeGnome::ExchangeType ExchangeGnome::findType()
{
    int types[UNKNOWN + 1];
    for (int i = 0; i <= UNKNOWN; i++)
        types[i] = 0;
    for (QMap<int, QList<Event *> *>::Iterator event_list = partition->events->begin();
         event_list != partition->events->end(); ++event_list)
    {
        bool b_ssrr = true, b_srsr = true, b_sswa = true, first = true, sentlast = true, odd = true;
        for (QList<Event *>::Iterator evt = (event_list.value())->begin();
             evt != (event_list.value())->end(); ++evt)
        {
            Message * msg = (*evt)->messages->at(0);
            if (first) // For the first message, we don't have a sentlast
            {
                first = false;
                if (msg->receiver == *evt) // Receive
                {
                    sentlast = false;
                    // Can't be these patterns
                    b_ssrr = false;
                    b_sswa = false;
                }
                odd = false;
            }
            else // For all other messages, check the pattern
            {
                if (msg->receiver == *evt) // Receive
                {
                    if (!sentlast && !odd) // Not alternating
                        b_srsr = false;
                    else if (sentlast && (*evt)->messages->size() > 1) // WaitAll or similar
                        b_ssrr = false;
                    sentlast = false;
                }
                else // Send
                {
                    if (!sentlast)
                    {
                        b_ssrr = false;
                        b_sswa = false;
                    }
                    else if (sentlast && !odd)
                    {
                        b_srsr = false;
                    }
                    sentlast = true;
                }
                odd = !odd;
            }
            if (!(b_ssrr || b_sswa || b_srsr)) { // Unknown
                types[UNKNOWN]++;
                break;
            }
        }
        std::cout << "Process " << event_list.key() << " with SRSR: " << b_srsr << ", SSRR ";
        std::cout << b_ssrr << ", SSWA " << b_sswa << std::endl;
        if (b_srsr)
            types[SRSR]++;
        if (b_ssrr)
            types[SSRR]++;
        if (b_sswa)
            types[SSWA]++;
    }
    std::cout << "Types are SRSR " << types[SRSR] << ", SSRR " << types[SSRR] << ", SSWA ";
    std::cout << types[SSWA] << ", UNKNOWN " << types[UNKNOWN] << std::endl;

    if (types[SRSR] > 0 && (types[SSRR] > 0 || types[SSWA] > 0)) // If we have this going on, really confusing
        return UNKNOWN;
    else if (types[UNKNOWN] > types[SRSR] && types[UNKNOWN] > types[SSRR] && types[UNKNOWN] > types[SSWA])
        return UNKNOWN;
    else if (types[SRSR] > 0)
        return SRSR;
    else if (types[SSWA] > types[SSRR]) // Some SSWA might look like SSRR and thus be double counted
        return SSWA;
    else
        return SSRR;
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
        //std::cout << "Calculating distances for process " << p1 << std::endl;
        cluster_leaves->insert(i, new PartitionCluster());
        cluster_leaves->value(i)->members->insert(i);
        for (int j = i + 1; j < num_processes; j++)
        {
            p2 = processes[j];
            //std::cout << "     Calculating between " << p1 << " and " << p2 << std::endl;
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
        //std::cout << "Handling distance #" << i << std::endl;
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
            // Increment both event lists now
            ++index2;
            if (index2 < list2->size())
                evt2 = list2->at(index2);
            else
                evt2 = NULL;
            ++index1;
            if (index1 < list2->size())
                evt1 = list1->at(index1);
            else
                evt1 = NULL;
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
    if (total_matched_steps == 0)
            return 0; //LLONG_MAX;
    return total_difference / total_matched_steps;
}

void ExchangeGnome::drawGnomeGL(QRect extents, VisOptions *_options)
{
    options = _options;
    if (options->metric != metric)
    {
        metric = options->metric;
        // Re-cluster
        findClusters();
    }
}

void ExchangeGnome::drawGnomeQt(QPainter * painter, QRect extents)
{

}
