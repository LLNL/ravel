#include "exchangegnome.h"
#include <QPainter>
#include <QRect>
#include <iostream>
#include <climits>
#include <cmath>

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
    cluster_root->print();
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
        cluster_leaves->value(i)->members->append(i);
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
                                      cluster_leaves->value(current.p1)->get_root(),
                                      cluster_leaves->value(current.p2)->get_root());
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
    std::cout << "Setting options in gnome" << std::endl;
    if (options->metric != metric)
    {
        metric = options->metric;
        // Re-cluster
        findClusters();
    }
}

void ExchangeGnome::drawGnomeQt(QPainter * painter, QRect extents, VisOptions *_options)
{
    options = _options;
    // debug
    drawGnomeQtCluster(painter, extents);
}

void ExchangeGnome::drawGnomeQtCluster(QPainter * painter, QRect extents)
{
    int treemargin = 5 * cluster_root->max_depth();

    int effectiveHeight = extents.height();
    int effectiveWidth = extents.width() - treemargin;

    int processSpan = partition->events->size();
    int stepSpan = partition->max_global_step - partition->min_global_step + 1;
    int spacingMinimum = 12;


    int process_spacing = 0;
    if (effectiveHeight / processSpan > spacingMinimum)
        process_spacing = 3;

    int step_spacing = 0;
    if (effectiveWidth / stepSpan + 1 > spacingMinimum)
        step_spacing = 3;


    float blockwidth;
    float blockheight = floor(effectiveHeight / processSpan);
    if (options->showAggregateSteps)
        blockwidth = floor(effectiveWidth / stepSpan);
    else
        blockwidth = floor(effectiveWidth / (ceil(stepSpan / 2.0)));
    float barheight = blockheight - process_spacing;
    float barwidth = blockwidth - step_spacing;
    painter->setPen(QPen(QColor(0, 0, 0)));

    // Generate inorder list of clusters

    // I need to go through clusters and keep track of how I'm drawing my branches from the root.
    // When I get to the leaf, I need to draw it.
    drawGnomeQtClusterBranch(painter, extents, cluster_root, extents.x() + treemargin, blockheight, blockwidth, barheight, barwidth);

}

void ExchangeGnome::drawGnomeQtClusterBranch(QPainter * painter, QRect current, PartitionCluster * pc, int leafx,
                                             int blockheight, int blockwidth, int barheight, int barwidth)
{
    std::cout << "Drawing for cluster " << pc->memberString().toStdString().c_str() << std::endl;
    int pc_size = pc->members->size();
    int my_x = current.x();
    int my_y = pc_size / 2.0 * blockheight;
    int child_x, child_y, used_y = 0;
    for (QList<PartitionCluster *>::Iterator child = pc->children->begin(); child != pc->children->end(); ++child)
    {
        // Draw line from wherever we start to correct height -- actually loop through children since info this side?
        // We are in the middle of these extents at current.x() and current.y() + current.h()
        // Though we may want to take the discreteness of the processes into account and figure it out by blockheight
        int child_size = (*child)->members->size();
        child_y = child_size / 2.0 * blockheight + used_y;
        painter->drawLine(my_x, my_y, my_x, child_y);

        // Draw forward correct amount of px
        child_x = my_x + 5;
        painter->drawLine(my_x, child_y, child_x, child_y);

        drawGnomeQtClusterBranch(painter, QRect(child_x, my_y + used_y, current.width(), current.height()), *child,
                                 leafx, blockheight, blockwidth, barheight, barwidth);
        used_y += child_size * blockheight;
    }

    // Possibly draw leaf
    if (pc->children->isEmpty())
    {
        int process = pc->members->at(0);
        std::cout << "Drawing leaf for member " << process << std::endl;
        drawGnomeQtClusterLeaf(painter, QRect(current.x(), current.y(), barwidth, barheight),
                               partition->events->value(process), blockwidth, partition->min_global_step);
    }
}

void ExchangeGnome::drawGnomeQtClusterLeaf(QPainter * painter, QRect startxy, QList<Event *> * elist, int blockwidth, int startStep)
{
    QString metric(options->metric);
    int y = startxy.y();
    int x, w, h, xa, wa;
    for (QList<Event *>::Iterator evt = elist->begin(); evt != elist->end(); ++evt)
    {
        if (options->showAggregateSteps)
            x = floor(((*evt)->step - startStep) * blockwidth) + 1 + startxy.x();
        else
            x = floor(((*evt)->step - startStep) / 2 * blockwidth) + 1 + startxy.x();
        w = startxy.width();
        h = startxy.height();

        // We know it will be complete in this view because we're not doing scrolling or anything here.

        // Draw the event
        if ((*evt)->hasMetric(metric))
            painter->fillRect(QRectF(x, y, w, h), QBrush(options->colormap->color((*evt)->getMetric(metric))));
        else
            painter->fillRect(QRectF(x, y, w, h), QBrush(QColor(180, 180, 180)));

        // Draw border but only if we're doing spacing, otherwise too messy
        if (blockwidth == w)
            painter->drawRect(QRectF(x,y,w,h));

        if (options->showAggregateSteps) {
            xa = floor(((*evt)->step - startStep - 1) * blockwidth) + 1 + startxy.x();
            wa = startxy.width();

            if ((*evt)->hasMetric(metric))
                painter->fillRect(QRectF(xa, y, wa, h), QBrush(options->colormap->color((*evt)->getMetric(metric, true))));
            else
                painter->fillRect(QRectF(xa, y, wa, h), QBrush(QColor(180, 180, 180)));
            if (blockwidth == w)
                painter->drawRect(QRectF(xa, y, wa, h));
        }

    }
}
