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
      cluster_root(NULL),
      saved_messages(QSet<Message *>()),
      drawnPCs(QMap<PartitionCluster *, QRect>()),
      drawnNodes(QMap<PartitionCluster *, QRect>()),
      alternation(true)
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
            /*Message * msg = (*evt)->messages->at(0); // For now only first matters, rest will be the same... NOT FOR RECVS
            if (msg->sender == (*evt))
                recvs.insert(msg->receiver->process);
            else
                sends.insert(msg->sender->process);
                */
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
        int recv_dist_2 = 0, recv_dist_not_2 = 0;
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
                    if (msg->receiver->step - msg->sender->step == 2)
                        recv_dist_2++;
                    else
                        recv_dist_not_2++;
                }
                odd = !odd;
            }
            if (!(b_ssrr || b_sswa || b_srsr)) { // Unknown
                types[UNKNOWN]++;
                break;
            }
        }
        if (b_srsr && recv_dist_not_2 > recv_dist_2)
            b_srsr = false;
        //std::cout << "Process " << event_list.key() << " with SRSR: " << b_srsr << ", SSRR ";
        //std::cout << b_ssrr << ", SSWA " << b_sswa << std::endl;
        if (b_srsr)
            types[SRSR]++;
        if (b_ssrr)
            types[SSRR]++;
        if (b_sswa)
            types[SSWA]++;
        if (!(b_ssrr || b_sswa || b_srsr))
            types[UNKNOWN]++;
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
        cluster_leaves->insert(i, new PartitionCluster(i, partition->events->value(i), "Lateness"));
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

    // From here we could now compress the ClusterEvent metrics (doing the four divides ahead of time)
    // but I'm going to retain the information for now and see how it goes
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
            if (index1 < list1->size())
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
            if (index1 < list1->size())
                evt1 = list1->at(index1);
            else
                evt1 = NULL;
        }
    }
    if (total_matched_steps == 0)
            return LLONG_MAX; //0;
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

    drawnPCs.clear();
}

void ExchangeGnome::drawGnomeQt(QPainter * painter, QRect extents, VisOptions *_options)
{
    options = _options;
    saved_messages.clear();
    drawnPCs.clear();
    drawnNodes.clear();
    // debug
    drawGnomeQtCluster(painter, extents);
}

void ExchangeGnome::drawGnomeQtCluster(QPainter * painter, QRect extents)
{
    alternation = true;
    int treemargin = 20 * cluster_root->max_depth();

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
    //drawGnomeQtClusterBranchPerfect(painter, extents, cluster_root, extents.x() + treemargin,
    //                                blockheight, blockwidth, barheight, barwidth);
    drawGnomeQtClusterBranch(painter, extents, cluster_root, extents.x() + treemargin,
                             blockheight, blockwidth, barheight, barwidth);

    // Now that we have drawn all the events, we need to draw the leaf-cluster messages or the leaf-leaf
    // messages which are saved in saved_messages.
    drawGnomeQtInterMessages(painter, extents.x() + treemargin, blockwidth, partition->min_global_step);

}

void ExchangeGnome::drawGnomeQtInterMessages(QPainter * painter, int leafx, int blockwidth, int startStep)
{
    if (options->showAggregateSteps)
        startStep -= 1;
    painter->setPen(QPen(Qt::black, 1.5, Qt::SolidLine));
    for (QSet<Message *>::Iterator msg = saved_messages.begin(); msg != saved_messages.end(); ++msg)
    {
        int x1, y1, x2, y2;
        PartitionCluster * sender_pc = cluster_leaves->value((*msg)->sender->process)->get_closed_root();
        PartitionCluster * receiver_pc = cluster_leaves->value((*msg)->receiver->process)->get_closed_root();

        x1 = leafx + blockwidth * ((*msg)->sender->step - startStep + 0.5);
        if (sender_pc->children->isEmpty())  // Sender is leaf
            y1 = sender_pc->extents.y() + sender_pc->extents.height() / 2;
        else if (sender_pc->extents.y() > receiver_pc->extents.y()) // Sender is lower cluster
            y1 = sender_pc->extents.y();
        else
            y1 = sender_pc->extents.y() + sender_pc->extents.height();

        x2 = leafx + blockwidth * ((*msg)->receiver->step - startStep + 0.5);
        if (receiver_pc->children->isEmpty()) // Sender is leaf
            y2 = receiver_pc->extents.y() + receiver_pc->extents.height() / 2;
        else if (receiver_pc->extents.y() > sender_pc->extents.y()) // Sender is lower cluster
            y2 = receiver_pc->extents.y();
        else
            y2 = receiver_pc->extents.y() + receiver_pc->extents.height();

        painter->drawLine(x1, y1, x2, y2);
    }
}

void ExchangeGnome::drawGnomeQtClusterBranchPerfect(QPainter * painter, QRect current, PartitionCluster * pc, int leafx,
                                                    int blockheight, int blockwidth, int barheight, int barwidth)
{
    int pc_size = pc->members->size();
    int my_x = current.x();
    int top_y = current.y();
    int my_y = top_y + pc_size / 2.0 * blockheight;
    int child_x, child_y, used_y = 0;
    painter->setPen(QPen(Qt::black, 2.0, Qt::SolidLine));
        for (QList<PartitionCluster *>::Iterator child = pc->children->begin(); child != pc->children->end(); ++child)
        {
            // Draw line from wherever we start to correct height -- actually loop through children since info this side?
            // We are in the middle of these extents at current.x() and current.y() + current.h()
            // Though we may want to take the discreteness of the processes into account and figure it out by blockheight
            int child_size = (*child)->members->size();
            //std::cout << "  Child size " << child_size << std::endl;
            child_y = top_y + child_size / 2.0 * blockheight + used_y;
            painter->drawLine(my_x, my_y, my_x, child_y);
            //std::cout << "Drawing line from " << my_x << ", " << my_y << "  to   " << my_x << ", " << child_y << std::endl;

            // Draw forward correct amount of px
            child_x = my_x + 20;
            painter->drawLine(my_x, child_y, child_x, child_y);

            drawGnomeQtClusterBranchPerfect(painter, QRect(child_x, top_y + used_y, current.width(), current.height()), *child,
                                     leafx, blockheight, blockwidth, barheight, barwidth);
            used_y += child_size * blockheight;
        }

        // Possibly draw leaf
        if (pc->children->isEmpty())
        {
            int process = pc->members->at(0);
            std::cout << "Drawing leaf for member " << process << std::endl;
            painter->drawLine(my_x, my_y, leafx, my_y);
            drawGnomeQtClusterLeaf(painter, QRect(leafx, current.y(), barwidth, barheight),
                                  partition->events->value(process), blockwidth, partition->min_global_step);
        }

}

void ExchangeGnome::drawGnomeQtClusterBranch(QPainter * painter, QRect current, PartitionCluster * pc, int leafx,
                                             int blockheight, int blockwidth, int barheight, int barwidth)
{
    //std::cout << "Drawing for cluster " << pc->memberString().toStdString().c_str() << std::endl;
    int pc_size = pc->members->size();
    //std::cout << "PC size is " << pc_size << " and blockheight is " << blockheight << std::endl;
    int my_x = current.x();
    int top_y = current.y();
    int my_y = top_y + pc_size / 2.0 * blockheight;
    int child_x, child_y, used_y = 0;
    painter->setPen(QPen(Qt::black, 2.0, Qt::SolidLine));
    if (pc->open && !pc->children->isEmpty())
    {
        for (QList<PartitionCluster *>::Iterator child = pc->children->begin(); child != pc->children->end(); ++child)
        {
            // Draw line from wherever we start to correct height -- actually loop through children since info this side?
            // We are in the middle of these extents at current.x() and current.y() + current.h()
            // Though we may want to take the discreteness of the processes into account and figure it out by blockheight
            int child_size = (*child)->members->size();
            //std::cout << "  Child size " << child_size << std::endl;
            child_y = top_y + child_size / 2.0 * blockheight + used_y;
            painter->drawLine(my_x, my_y, my_x, child_y);
            //std::cout << "Drawing line from " << my_x << ", " << my_y << "  to   " << my_x << ", " << child_y << std::endl;

            // Draw forward correct amount of px
            child_x = my_x + 20;
            painter->drawLine(my_x, child_y, child_x, child_y);

            QRect node = QRect(my_x - 3, my_y - 3, 6, 6);
            painter->fillRect(node, QBrush(Qt::black));
            drawnNodes[pc] = node;

            drawGnomeQtClusterBranch(painter, QRect(child_x, top_y + used_y, current.width(), current.height()), *child,
                                     leafx, blockheight, blockwidth, barheight, barwidth);
            used_y += child_size * blockheight;
        }
    }
    else if (pc->children->isEmpty())
    {
            //std::cout << "Drawing for cluster " << pc->memberString().toStdString().c_str() << std::endl;
            int process = pc->members->at(0);
            //std::cout << "Drawing leaf for member " << process << std::endl;
            painter->drawLine(my_x, my_y, leafx, my_y);
            drawGnomeQtClusterLeaf(painter, QRect(leafx, current.y(), barwidth, barheight),
                                  partition->events->value(process), blockwidth, partition->min_global_step);
            // TODO: Figure out message lines for this view -- may need to look up things in cluster to leaves to
            // see if we draw and if so where.
            // So we really need to save the locations of sends and receives with messages... probably need a new
            // ClusterMessage container for this. Lots of allocating and deleting will be required
            drawnPCs[pc] = QRect(leafx, current.y(), current.width() - leafx, blockheight);
            pc->extents = drawnPCs[pc];
    }
    else // This is open
    {
        //std::cout << "Drawing for cluster " << pc->memberString().toStdString().c_str() << std::endl;
        if (type == SRSR)
            drawGnomeQtClusterSRSR(painter, QRect(leafx, current.y(), current.width() - leafx, blockheight * pc->members->size()),
                                   pc, barwidth, barheight, blockwidth, blockheight,
                                   partition->min_global_step);
        drawnPCs[pc] = QRect(leafx, current.y(), current.width() - leafx, blockheight * pc->members->size());
        pc->extents = drawnPCs[pc];
    }


}

void ExchangeGnome::drawGnomeQtClusterSRSR(QPainter * painter, QRect startxy, PartitionCluster * pc,
                                           int barwidth, int barheight, int blockwidth, int blockheight,
                                           int startStep)
{
    int total_height = pc->members->size() * blockheight;
    int base_y = startxy.y() + total_height / 2 - blockheight;
    int x, y, w, h, xa, wa, xr, yr;
    xr = blockwidth;
    if (options->showAggregateSteps) {
        startStep -= 1;
        xr *= 2;
    }
    if (alternation) {
        painter->fillRect(startxy, QBrush(QColor(217, 217, 217)));
    } else {
        painter->fillRect(startxy, QBrush(QColor(189, 189, 189)));
    }
    alternation = !alternation;
    painter->setPen(QPen(Qt::black, 2.0, Qt::SolidLine));
    QList<DrawMessage *> msgs = QList<DrawMessage *>();
    for (QList<ClusterEvent *>::Iterator evt = pc->events->begin(); evt != pc->events->end(); ++evt)
    {
        if (options->showAggregateSteps)
            x = floor(((*evt)->step - startStep) * blockwidth) + 1 + startxy.x();
        else
            x = floor(((*evt)->step - startStep) / 2 * blockwidth) + 1 + startxy.x();
        w = barwidth;
        h = barheight;
        y = base_y;
        if ((((*evt)->step - startStep + 2) / 4) % 2)
            y += blockheight;

        // We know it will be complete in this view because we're not doing scrolling or anything here.

        // Draw the event
        painter->fillRect(QRectF(x, y, w, h), QBrush(options->colormap->color(
                                                         ((*evt)->low_metric + (*evt)->high_metric)
                                                         / ((*evt)->num_low + (*evt)->num_high)
                                                         )));

        // Draw border but only if we're doing spacing, otherwise too messy
        if (blockwidth != w)
            painter->drawRect(QRectF(x,y,w,h));

        if (options->showAggregateSteps) {
            xa = floor(((*evt)->step - startStep - 1) * blockwidth) + 1 + startxy.x();
            wa = barwidth;

                painter->fillRect(QRectF(xa, y, wa, h), QBrush(options->colormap->color(
                                                                   ((*evt)->agg_low + (*evt)->agg_high)
                                                                   / ((*evt)->num_agg_low + (*evt)->num_agg_high)
                                                                   )));
            if (blockwidth != w)
                painter->drawRect(QRectF(xa, y, wa, h));
        }

        // Need to draw messages after this, or at least keep track of messages...
        // Maybe check num_sends here?
        if ((*evt)->num_send > 0)
        {
            if (y == base_y)
                yr = base_y + blockheight;
            else
                yr = base_y;
            msgs.append(new DrawMessage(QPoint(x + w/2, y + h/2), QPoint(x + w/2 + xr, yr + h/2), (*evt)->num_send));
        }
        if ((*evt)->num_recv > 0)
        {
            DrawMessage * dm = msgs.last();
            dm->nrecvs = (*evt)->num_recv;
        }
    }

    // Now draw the messages....
    for (QList<DrawMessage *>::Iterator dm = msgs.begin(); dm != msgs.end(); ++dm)
    {
        painter->setPen(QPen(Qt::black, (*dm)->nsends * 2.0 / pc->members->size(), Qt::SolidLine));
        painter->drawLine((*dm)->send, (*dm)->recv);
    }

    // Clean up the draw messages...
    for (QList<DrawMessage *>::Iterator dm = msgs.begin(); dm != msgs.end(); ++dm)
        delete *dm;
}

void ExchangeGnome::drawGnomeQtClusterLeaf(QPainter * painter, QRect startxy, QList<Event *> * elist, int blockwidth, int startStep)
{
    QString metric(options->metric);
    int y = startxy.y();
    int x, w, h, xa, wa;
    if (options->showAggregateSteps)
        startStep -= 1;
    painter->setPen(QPen(Qt::black, 2.0, Qt::SolidLine));
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
        if (blockwidth != w)
            painter->drawRect(QRectF(x,y,w,h));

        if (options->showAggregateSteps) {
            xa = floor(((*evt)->step - startStep - 1) * blockwidth) + 1 + startxy.x();
            wa = startxy.width();

            if ((*evt)->hasMetric(metric))
                painter->fillRect(QRectF(xa, y, wa, h), QBrush(options->colormap->color((*evt)->getMetric(metric, true))));
            else
                painter->fillRect(QRectF(xa, y, wa, h), QBrush(QColor(180, 180, 180)));
            if (blockwidth != w)
                painter->drawRect(QRectF(xa, y, wa, h));
        }

        for (QVector<Message *>::Iterator msg = (*evt)->messages->begin(); msg != (*evt)->messages->end(); ++msg)
            saved_messages.insert(*msg);

    }
}

void ExchangeGnome::handleDoubleClick(QMouseEvent * event)
{
    std::cout << "I am handling double click for gnome" << std::endl;
    int x = event->x();
    int y = event->y();
    // Figure out which branch this occurs in, open that branch
    for (QMap<PartitionCluster *, QRect>::Iterator p = drawnNodes.begin(); p != drawnNodes.end(); ++p)
        if (p.value().contains(x,y))
        {
            PartitionCluster * pc = p.key();
            std::cout << "Closing " << pc->memberString().toStdString().c_str() << std::endl;
            pc->close();
            return; // Return so we don't look elsewhere.
        }
    for (QMap<PartitionCluster *, QRect>::Iterator p = drawnPCs.begin(); p != drawnPCs.end(); ++p)
        if (p.value().contains(x,y))
        {
            PartitionCluster * pc = p.key();
            std::cout << "Opening " << pc->memberString().toStdString().c_str() << std::endl;
            pc->open = true;
            return;
        }
}
