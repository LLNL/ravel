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
      SRSRmap(QMap<int, int>()),
      SRSRpatterns(QSet<int>()),
      maxWAsize(0),
      cluster_leaves(NULL),
      cluster_root(NULL),
      top_processes(QList<int>()),
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
    if (type != SRSR)
    {
        SRSRmap.clear();
        SRSRpatterns.clear();
    }

    // Cluster vectors
    findClusters();
    //cluster_root->print();
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
        int srsr_pattern = 0;
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

                    if ((*evt)->messages->size() > maxWAsize)
                        maxWAsize = (*evt)->messages->size();
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

                    srsr_pattern += 1 << (((*evt)->step - partition->min_global_step) / 2);
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
        if (b_srsr) {
            types[SRSR]++;
            SRSRmap[event_list.key()] = srsr_pattern;
            SRSRpatterns.insert(srsr_pattern);
            std::cout << "Adding pattern " << srsr_pattern << " on process " << event_list.key() << std::endl;
        }
        if (b_ssrr)
            types[SSRR]++;
        if (b_sswa)
            types[SSWA]++;
        if (!(b_ssrr || b_sswa || b_srsr))
            types[UNKNOWN]++;
    }
    //std::cout << "Types are SRSR " << types[SRSR] << ", SSRR " << types[SSRR] << ", SSWA ";
   // std::cout << types[SSWA] << ", UNKNOWN " << types[UNKNOWN] << std::endl;

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
    std::cout << "Finding clusters..." << std::endl;
    QList<DistancePair> distances;
    QList<int> processes = partition->events->keys();
    top_processes.clear();
    if (cluster_root)
    {
        cluster_root->delete_tree();
        delete cluster_root;
        delete cluster_leaves;
    }
    cluster_leaves = new QMap<int, PartitionCluster *>();
    long long int max_metric = LLONG_MIN;
    int max_metric_process = -1;
    qSort(processes);
    int num_processes = processes.size();
    int p1, p2;
    long long int distance;
    for (int i = 0; i < num_processes; i++)
    {
        p1 = processes[i];
        //std::cout << "Calculating distances for process " << p1 << std::endl;
        cluster_leaves->insert(p1, new PartitionCluster(p1, partition->events->value(p1), "Lateness"));
        if (cluster_leaves->value(p1)->max_metric > max_metric)
        {
            max_metric = cluster_leaves->value(p1)->max_metric;
            max_metric_process = p1;
        }
        for (int j = i + 1; j < num_processes; j++)
        {
            p2 = processes[j];
            //std::cout << "     Calculating between " << p1 << " and " << p2 << std::endl;
            distance = calculateMetricDistance2(partition->events->value(p1),
                                               partition->events->value(p2));
            distances.append(DistancePair(distance, p1, p2));
            //std::cout << "    distance between " << p1 << " and " << p2 << " is " << distance << std::endl;
        }
    }
    qSort(distances);

    int lastp = distances[0].p1;
    QList<long long int> cluster_distances = QList<long long int>();
    PartitionCluster * pc = NULL;
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

    // From the max_metric_process and the type, get the rest of the processes we draw by default

    std::cout << "Top process is " << max_metric_process << std::endl;
    if (type == SRSR) // Add all partners and then search those partners for the missing strings
    {
        QList<Event *> * elist = partition->events->value(max_metric_process);
        QSet<int> add_processes = QSet<int>();
        QSet<int> level_processes = QSet<int>();
        QSet<int> patterns = QSet<int>();
        patterns.insert(SRSRmap[max_metric_process]);
        add_processes.insert(max_metric_process);
        for (QList<Event *>::Iterator evt = elist->begin(); evt != elist->end(); ++evt)
        {
            for (QVector<Message *>::Iterator msg = (*evt)->messages->begin(); msg != (*evt)->messages->end(); ++msg)
            {
                if (*evt == (*msg)->sender)
                {
                    if (!add_processes.contains((*msg)->receiver->process))
                    {
                        add_processes.insert((*msg)->receiver->process);
                        level_processes.insert((*msg)->receiver->process);
                        patterns.insert(SRSRmap[(*msg)->receiver->process]);
                        std::cout << "  Adding neighbor " << (*msg)->receiver->process << std::endl;
                    }
                }
                else
                {
                    if (!add_processes.contains((*msg)->sender->process))
                    {
                        add_processes.insert((*msg)->sender->process);
                        level_processes.insert((*msg)->sender->process);
                        patterns.insert(SRSRmap[(*msg)->sender->process]);
                        std::cout << "  Adding neighbor " << (*msg)->sender->process << std::endl;
                    }
                }
            }
        }

        // Now check, do we have all patterns?
        int neighbor_depth = 1;
        while (patterns.size() / 1.0 / SRSRpatterns.size() < 0.5)
        {
            QList<int> check_group = level_processes.toList();
            level_processes.clear();
            qSort(check_group); // Future, possibly sort by something else than process id, like metric

            // Now see about adding things from the check group
            for (QList<int>::Iterator proc = check_group.begin(); proc != check_group.end(); ++proc)
            {
                elist = partition->events->value(*proc);
                for (QList<Event *>::Iterator evt = elist->begin(); evt != elist->end(); ++evt)
                {
                    for (QVector<Message *>::Iterator msg = (*evt)->messages->begin(); msg != (*evt)->messages->end(); ++msg)
                    {
                        if (*evt == (*msg)->sender)
                        {
                            if (!add_processes.contains((*msg)->receiver->process))
                            {
                                add_processes.insert((*msg)->receiver->process);
                                level_processes.insert((*msg)->receiver->process);
                                patterns.insert(SRSRmap[(*msg)->receiver->process]);
                                std::cout << "  Adding " << (*msg)->receiver->process << std::endl;
                            }
                        }
                        else
                        {
                            if (!add_processes.contains((*msg)->sender->process))
                            {
                                add_processes.insert((*msg)->sender->process);
                                level_processes.insert((*msg)->sender->process);
                                patterns.insert(SRSRmap[(*msg)->sender->process]);
                                std::cout << "  Adding " << (*msg)->sender->process << std::endl;
                            }
                        }
                    }
                }
            } // checked everything in check_group

            check_group.clear();
            neighbor_depth++;
        }
        top_processes += add_processes.toList();
        std::cout << "Neighbor depth: " << neighbor_depth << std::endl;
    }
    else if (type == SSRR || type == SSWA) // Add all partners
    {
        top_processes.append(max_metric_process);
        QList<Event *> * elist = partition->events->value(max_metric_process);
        QSet<int> add_processes = QSet<int>();
        for (QList<Event *>::Iterator evt = elist->begin(); evt != elist->end(); ++evt)
        {
            for (QVector<Message *>::Iterator msg = (*evt)->messages->begin(); msg != (*evt)->messages->end(); ++msg)
            {
                if (*evt == (*msg)->sender)
                {
                    add_processes.insert((*msg)->receiver->process);
                }
                else
                {
                    add_processes.insert((*msg)->sender->process);
                }
            }
        }
        top_processes += add_processes.toList();
    }
    qSort(top_processes); // Let's keep the initial order for now when we draw
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

// When calculating distance between two event lists. When one is missing a step, we
// estimate the lateness of that missing step via the average of the two adjacent steps
// No wait, for now lets estimate the lateness as the step that came before it if available
// and only if not we estimate as the one afterwards or perhaps skip?
long long int ExchangeGnome::calculateMetricDistance2(QList<Event *> * list1, QList<Event *> * list2)
{
    int index1 = 0, index2 = 0, total_calced_steps = 0;
    Event * evt1 = list1->at(0), * evt2 = list2->at(0);
    long long int last1 = 0, last2 = 0, total_difference = 0;

    while (evt1 && evt2)
    {
        if (evt1->step == evt2->step) // If they're equal, add their distance
        {
            last1 = evt1->getMetric(metric);
            last2 = evt2->getMetric(metric);
            total_difference += (last1 - last2) * (last1 - last2);
            ++total_calced_steps;
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
            // Estimate evt1 lateness
            last2 = evt2->getMetric(metric);
            if (evt1->comm_prev && evt1->comm_prev->partition == evt1->partition)
            {
                total_difference += (last1 - last2) * (last1 - last2);
                ++total_calced_steps;
            }

            // Move evt2 forward
            ++index2;
            if (index2 < list2->size())
                evt2 = list2->at(index2);
            else
                evt2 = NULL;
        } else {
            last1 = evt1->getMetric(metric);
            if (evt2->comm_prev && evt2->comm_prev->partition == evt2->partition)
            {
                total_difference += (last1 - last2) * (last1 - last2);
                ++total_calced_steps;
            }

            // Move evt1 forward
            ++index1;
            if (index1 < list1->size())
                evt1 = list1->at(index1);
            else
                evt1 = NULL;
        }
    }
    if (total_calced_steps == 0) {
        //std::cout << "No steps between " << evt1->process << " and " << evt2->process << std::endl;
        return LLONG_MAX; //0;
    }
    //std::cout << "     total_difference " << total_difference << " with steps " << total_calced_steps << std::endl;
    return total_difference / total_calced_steps;
}


void ExchangeGnome::drawGnomeGL(QRect extents, VisOptions *_options)
{
    options = _options;
    //std::cout << "Setting options in gnome" << std::endl;
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
    int treemargin = 5 * cluster_root->max_open_depth();
    int labelwidth = 0;
    if (cluster_root->leaf_open())
    {
        painter->setFont(QFont("Helvetica", 10));
        QFontMetrics font_metrics = painter->fontMetrics();
        QString text = QString::number((*(std::max_element(cluster_root->members->begin(), cluster_root->members->end()))));

        // Determine bounding box of FontMetrics
        labelwidth = font_metrics.boundingRect(text).width();
        treemargin += labelwidth;
    }

    int topHeight = 0;
    if (options->drawTop)
    {
        int fair_portion = top_processes.size() / 1.0 / cluster_root->members->size() * extents.height();
        int min_size = 12 * top_processes.size();
        if (min_size > extents.height())
            topHeight = fair_portion;
        else
            topHeight = std::max(fair_portion, min_size);
    }

    int effectiveHeight = extents.height() - topHeight;
    int effectiveWidth = extents.width() - treemargin;

    int processSpan = partition->events->size();
    int stepSpan = partition->max_global_step - partition->min_global_step + 2;
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


    if (options->drawTop)
    {
        QRect top_extents = QRect(extents.x() + treemargin, effectiveHeight, extents.width() - treemargin, topHeight);
        drawGnomeQtTopProcesses(painter, top_extents, blockwidth, barwidth, labelwidth);
    }

    // Generate inorder list of clusters

    // I need to go through clusters and keep track of how I'm drawing my branches from the root.
    // When I get to the leaf, I need to draw it.
    //drawGnomeQtClusterBranchPerfect(painter, extents, cluster_root, extents.x() + treemargin,
    //                                blockheight, blockwidth, barheight, barwidth);
    QRect cluster_extents = QRect(extents.x(), extents.y(), extents.width(), effectiveHeight);
    drawGnomeQtClusterBranch(painter, cluster_extents, cluster_root, extents.x() + treemargin,
                             blockheight, blockwidth, barheight, barwidth, treemargin,
                             labelwidth);

    // Now that we have drawn all the events, we need to draw the leaf-cluster messages or the leaf-leaf
    // messages which are saved in saved_messages.
    drawGnomeQtInterMessages(painter, extents.x() + treemargin, blockwidth, partition->min_global_step);

}


void ExchangeGnome::drawGnomeQtTopProcesses(QPainter * painter, QRect extents,
                                            int blockwidth, int barwidth, int labelwidth)
{
    int effectiveHeight = extents.height();

    int process_spacing = blockwidth - barwidth;
    int step_spacing = process_spacing;

    float x, y, w, h, xa, wa;
    float blockheight = floor(effectiveHeight / top_processes.size());
    int startStep = partition->min_global_step;
    if (options->showAggregateSteps)
    {
        startStep -= 1;
    }
    float barheight = blockheight - process_spacing;

    QSet<Message *> drawMessages = QSet<Message *>();
    painter->setPen(QPen(QColor(0, 0, 0)));
    QMap<int, int> processYs = QMap<int, int>();

    for (int i = 0; i < top_processes.size(); ++i)
    {
        QList<Event *> * event_list = partition->events->value(top_processes[i]);
        y =  floor(extents.y() + i * blockheight) + 1;
        painter->drawText(extents.x() - labelwidth - 1, y + blockheight / 2, QString::number(top_processes[i]));
        processYs[top_processes[i]] = y;
        for (QList<Event *>::Iterator evt = event_list->begin(); evt != event_list->end(); ++evt)
        {
            if (options->showAggregateSteps)
                x = floor(((*evt)->step - startStep) * blockwidth) + 1 + extents.x();
            else
                x = floor(((*evt)->step - startStep) / 2 * blockwidth) + 1 + extents.x();
            w = barwidth;
            h = barheight;

            painter->fillRect(QRectF(x, y, w, h), QBrush(options->colormap->color((*evt)->getMetric(metric))));

            // Draw border but only if we're doing spacing, otherwise too messy
            if (step_spacing > 0 && process_spacing > 0)
                painter->drawRect(QRectF(x,y,w,h));

            // For selection
            // drawnEvents[*evt] = QRect(x, y, w, h);

            for (QVector<Message *>::Iterator msg = (*evt)->messages->begin(); msg != (*evt)->messages->end(); ++msg)
            {
                if (top_processes.contains((*msg)->sender->process) && top_processes.contains((*msg)->receiver->process))
                    drawMessages.insert((*msg));
                else if (*evt == (*msg)->sender)
                {
                    // send
                    if ((*msg)->sender->process > (*msg)->receiver->process)
                        painter->drawLine(x + w/2, y + h/2, x + w, y);
                    else
                        painter->drawLine(x + w/2, y + h/2, x + w, y + h);
                }
                else
                {
                    // recv
                    if ((*msg)->sender->process > (*msg)->receiver->process)
                        painter->drawLine(x + w/2, y + h/2, x, y + h);
                    else
                        painter->drawLine(x + w/2, y + h/2, x, y);
                }
            }

            if (options->showAggregateSteps) {
                xa = floor(((*evt)->step - startStep - 1) * blockwidth) + 1 + extents.x();
                wa = barwidth;

                painter->fillRect(QRectF(xa, y, wa, h), QBrush(options->colormap->color((*evt)->getMetric(metric, true))));

                if (step_spacing > 0 && process_spacing > 0)
                    painter->drawRect(QRectF(xa, y, wa, h));
            }

        }
    }

        // Messages
        // We need to do all of the message drawing after the event drawing
        // for overlap purposes
    if (options->showMessages)
    {
        if (top_processes.size() <= 32)
            painter->setPen(QPen(Qt::black, 2, Qt::SolidLine));
        else
            painter->setPen(QPen(Qt::black, 1, Qt::SolidLine));
        Event * send_event;
        Event * recv_event;
        QPointF p1, p2;
        w = barwidth;
        h = barheight;
        for (QSet<Message *>::Iterator msg = drawMessages.begin(); msg != drawMessages.end(); ++msg) {
            send_event = (*msg)->sender;
            recv_event = (*msg)->receiver;
            y = processYs[send_event->process];
            if (options->showAggregateSteps)
                x = floor((send_event->step - startStep) * blockwidth) + 1 + extents.x();
            else
                x = floor((send_event->step - startStep) / 2 * blockwidth) + 1 + extents.x();
            p1 = QPointF(x + w/2.0, y + h/2.0);
            y = processYs[recv_event->process];
            if (options->showAggregateSteps)
                x = floor((recv_event->step - startStep) * blockwidth) + 1 + extents.x();
            else
                x = floor((recv_event->step - startStep) / 2 * blockwidth) + 1 + extents.x();
            p2 = QPointF(x + w/2.0, y + h/2.0);
            painter->drawLine(p1, p2);
        }
    }
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
            //std::cout << "Drawing leaf for member " << process << std::endl;
            painter->drawLine(my_x, my_y, leafx, my_y);
            drawGnomeQtClusterLeaf(painter, QRect(leafx, current.y(), barwidth, barheight),
                                  partition->events->value(process), blockwidth, partition->min_global_step);
        }

}

void ExchangeGnome::drawGnomeQtClusterBranch(QPainter * painter, QRect current, PartitionCluster * pc, int leafx,
                                             int blockheight, int blockwidth, int barheight, int barwidth,
                                             int treemargin, int labelwidth)
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
        // Draw line to myself
        //painter->drawLine(my_x - 20, my_y, my_x, my_y);
        for (QList<PartitionCluster *>::Iterator child = pc->children->begin(); child != pc->children->end(); ++child)
        {
            // Draw line from wherever we start to correct height -- actually loop through children since info this side?
            // We are in the middle of these extents at current.x() and current.y() + current.h()
            // Though we may want to take the discreteness of the processes into account and figure it out by blockheight
            painter->setPen(QPen(Qt::black, 2.0, Qt::SolidLine));
            int child_size = (*child)->members->size();
            //std::cout << "  Child size " << child_size << std::endl;
            child_y = top_y + child_size / 2.0 * blockheight + used_y;
            painter->drawLine(my_x, my_y, my_x, child_y);
            //std::cout << "Drawing line from " << my_x << ", " << my_y << "  to   " << my_x << ", " << child_y << std::endl;

            // Draw forward correct amount of px
            child_x = my_x + 5;
            painter->drawLine(my_x, child_y, child_x, child_y);

            QRect node = QRect(my_x - 3, my_y - 3, 6, 6);
            painter->fillRect(node, QBrush(Qt::black));
            drawnNodes[pc] = node;

            drawGnomeQtClusterBranch(painter, QRect(child_x, top_y + used_y, current.width(), current.height()), *child,
                                     leafx, blockheight, blockwidth, barheight, barwidth, treemargin, labelwidth);
            used_y += child_size * blockheight;
        }
    }
    else if (pc->children->isEmpty())
    {
        painter->setPen(QPen(Qt::black, 2.0, Qt::SolidLine));
            //std::cout << "Drawing for cluster " << pc->memberString().toStdString().c_str() << std::endl;
            int process = pc->members->at(0);
            //std::cout << "Drawing leaf for member " << process << std::endl;
            painter->drawLine(my_x, my_y, leafx - labelwidth, my_y);
            painter->setPen(QPen(Qt::white, 2.0, Qt::SolidLine));
            painter->drawLine(leafx - labelwidth, my_y, leafx, my_y);
            painter->setPen(QPen(Qt::black, 2.0, Qt::SolidLine));
            painter->drawText(leafx - labelwidth, my_y + 3, QString::number(process));
            drawGnomeQtClusterLeaf(painter, QRect(leafx, current.y(), barwidth, barheight),
                                  partition->events->value(process), blockwidth, partition->min_global_step);
            // TODO: Figure out message lines for this view -- may need to look up things in cluster to leaves to
            // see if we draw and if so where.
            // So we really need to save the locations of sends and receives with messages... probably need a new
            // ClusterMessage container for this. Lots of allocating and deleting will be required
            drawnPCs[pc] = QRect(leafx, current.y(), current.width() - treemargin, blockheight);
            pc->extents = drawnPCs[pc];
    }
    else // This is open
    {
        painter->setPen(QPen(Qt::black, 2.0, Qt::SolidLine));
        //std::cout << "Drawing for cluster " << pc->memberString().toStdString().c_str() << std::endl;
        painter->drawLine(my_x, my_y, leafx, my_y);
        QRect clusterRect = QRect(leafx, current.y(), current.width() - treemargin, blockheight * pc->members->size());
        if (type == SRSR)
            drawGnomeQtClusterSRSR(painter, clusterRect, pc,
                                   barwidth, barheight, blockwidth, blockheight,
                                   partition->min_global_step);
        else if (type == SSRR)
            drawGnomeQtClusterSSRR(painter, clusterRect, pc,
                                   barwidth, barheight, blockwidth, blockheight,
                                   partition->min_global_step);
        else if (type == SSWA)
            drawGnomeQtClusterSSWA(painter, clusterRect, pc,
                                   barwidth, barheight, blockwidth, blockheight,
                                   partition->min_global_step);
        drawnPCs[pc] = clusterRect;
        pc->extents = clusterRect;
    }


}

void ExchangeGnome::drawGnomeQtClusterSSRR(QPainter * painter, QRect startxy, PartitionCluster * pc,
                                           int barwidth, int barheight, int blockwidth, int blockheight,
                                           int startStep)
{
    // Here blockheight is the maximum height afforded to the block. We actually scale it
    // based on how many processes are sending or receiving at that point
    // The first row is send, the second row is recv
    bool drawMessages = true;
    if (startxy.height() > 2 * clusterMaxHeight)
    {
        blockheight = clusterMaxHeight - 25;
        barheight = blockheight - 3;
    }
    else
    {
        blockheight = startxy.height() / 2;
        if (blockheight < 40)
            drawMessages = false;
        else
            blockheight -= 20; // For message drawing
        if (barheight > blockheight - 3)
            barheight = blockheight;
    }
    int base_y = startxy.y() + startxy.height() / 2 - blockheight;
    int x, ys, yr, w, hs, hr, xa, wa, ya, ha, nsends, nrecvs;
    yr = base_y + blockheight;
    int base_ya = base_y + blockheight / 2 + (blockheight - barheight) / 2;
    if (options->showAggregateSteps) {
        startStep -= 1;
    }
    //std::cout << "Drawing background " << startxy.x() << ", " << startxy.y();
    //std::cout << ", " << startxy.width() << ", " << startxy.height() << std::endl;
    if (alternation) {
        painter->fillRect(startxy, QBrush(QColor(217, 217, 217)));
    } else {
        painter->fillRect(startxy, QBrush(QColor(189, 189, 189)));
    }
    alternation = !alternation;
    painter->setPen(QPen(Qt::black, 2.0, Qt::SolidLine));
    for (QList<ClusterEvent *>::Iterator evt = pc->events->begin(); evt != pc->events->end(); ++evt)
    {
        if (options->showAggregateSteps)
            x = floor(((*evt)->step - startStep) * blockwidth) + 1 + startxy.x();
        else
            x = floor(((*evt)->step - startStep) / 2 * blockwidth) + 1 + startxy.x();
        w = barwidth;
        nsends = (*evt)->getCount(ClusterEvent::COMM, ClusterEvent::SEND);
        nrecvs = (*evt)->getCount(ClusterEvent::COMM, ClusterEvent::RECV);
        hs = barheight * nsends / 1.0 / pc->members->size();
        ys = base_y + barheight - hs;
        hr = barheight * nrecvs / 1.0 / pc->members->size();

        // Draw the event
        if ((*evt)->getCount(ClusterEvent::COMM, ClusterEvent::SEND, ClusterEvent::ALL))
            painter->fillRect(QRectF(x, ys, w, hs), QBrush(options->colormap->color(
                                                         (*evt)->getMetric(ClusterEvent::COMM, ClusterEvent::SEND,
                                                                           ClusterEvent::ALL)
                                                         / (*evt)->getCount(ClusterEvent::COMM, ClusterEvent::SEND,
                                                                            ClusterEvent::ALL)
                                                         )));
        if ((*evt)->getCount(ClusterEvent::COMM, ClusterEvent::RECV, ClusterEvent::ALL))
            painter->fillRect(QRectF(x, yr, w, hr), QBrush(options->colormap->color(
                                                         (*evt)->getMetric(ClusterEvent::COMM, ClusterEvent::RECV,
                                                                           ClusterEvent::ALL)
                                                         / (*evt)->getCount(ClusterEvent::COMM, ClusterEvent::RECV,
                                                                            ClusterEvent::ALL)
                                                         )));

        // Draw border but only if we're doing spacing, otherwise too messy
        if (blockwidth != w) {
            painter->drawRect(QRectF(x,ys,w,hs));
            painter->drawRect(QRectF(x,yr,w,hr));
        }

        if (drawMessages) {
            if (nsends)
            {
                painter->setPen(QPen(Qt::black, nsends * 2.0 / pc->members->size(), Qt::SolidLine));
                painter->drawLine(x + blockwidth / 2, ys, x + barwidth, ys - 20);
            }
            if (nrecvs)
            {
                painter->setPen(QPen(Qt::black, nrecvs * 2.0 / pc->members->size(), Qt::SolidLine));
                painter->drawLine(x + blockwidth / 2, yr + barheight, x + 1, yr + barheight + 20);
            }
        }

        if (options->showAggregateSteps) {
            xa = floor(((*evt)->step - startStep - 1) * blockwidth) + 1 + startxy.x();
            wa = barwidth;
            ha = barheight * (nsends + nrecvs) / 1.0 / pc->members->size();
            ya = base_ya + (barheight - ha) / 2;

            painter->fillRect(QRectF(xa, ya, wa, ha),
                              QBrush(options->colormap->color(
                                                              (*evt)->getMetric(ClusterEvent::AGG, ClusterEvent::BOTH,
                                                                                ClusterEvent::ALL)
                                                              / (*evt)->getCount(ClusterEvent::AGG, ClusterEvent::BOTH,
                                                                                 ClusterEvent::ALL)
                                                              )));
            if (blockwidth != w)
                painter->drawRect(QRectF(xa, ya, wa, ha));
        }


    }
}

void ExchangeGnome::drawGnomeQtClusterSSWA(QPainter * painter, QRect startxy, PartitionCluster * pc,
                                           int barwidth, int barheight, int blockwidth, int blockheight,
                                           int startStep)
{
    // For this we only need one row of events but we probably want to have some extra room
    // for all of the messages that happen
    bool drawMessages = true;
    if (startxy.height() > 2 * clusterMaxHeight)
    {
        blockheight = clusterMaxHeight - 25;
        barheight = blockheight - 3;
    }
    else
    {
        blockheight = startxy.height() / 2;
        if (blockheight < 40)
            drawMessages = false;
        else
            blockheight -= 20; // For message drawing
        if (barheight > blockheight - 3)
            barheight = blockheight;
    }
    int x, ys, yw, w, hs, hw, xa, wa, ya, ha, nsends, nwaits;
    int base_y = startxy.y() + startxy.height() / 2 - blockheight;
    int base_ya = base_y + blockheight / 2;
    yw = base_y + blockheight;
    if (options->showAggregateSteps) {
        startStep -= 1;
    }
    std::cout << "Drawing background " << startxy.x() << ", " << startxy.y();
    std::cout << ", " << startxy.width() << ", " << startxy.height() << std::endl;
    if (alternation) {
        painter->fillRect(startxy, QBrush(QColor(217, 217, 217)));
    } else {
        painter->fillRect(startxy, QBrush(QColor(189, 189, 189)));
    }
    alternation = !alternation;
    painter->setPen(QPen(Qt::black, 2.0, Qt::SolidLine));
    for (QList<ClusterEvent *>::Iterator evt = pc->events->begin(); evt != pc->events->end(); ++evt)
    {
        if (options->showAggregateSteps)
            x = floor(((*evt)->step - startStep) * blockwidth) + 1 + startxy.x();
        else
            x = floor(((*evt)->step - startStep) / 2 * blockwidth) + 1 + startxy.x();
        w = barwidth;
        nsends = (*evt)->getCount(ClusterEvent::COMM, ClusterEvent::SEND);
        nwaits = (*evt)->getCount(ClusterEvent::COMM, ClusterEvent::WAITALL);
        std::cout << "Num waits is " << nwaits << std::endl;

        hs = barheight * nsends / 1.0 / pc->members->size();
        ys = base_y + barheight - hs;
        hw = barheight * nwaits / 1.0 / pc->members->size();

        // Draw the event
        if (nsends) {
            QColor sendColor = options->colormap->color((*evt)->getMetric(ClusterEvent::COMM, ClusterEvent::SEND,
                                          ClusterEvent::ALL) / nsends);
            //painter->setBrush(sendColor);
            painter->fillRect(QRectF(x, ys, w, hs), QBrush(sendColor));
        }
         if (nwaits)
         {
            QColor waitColor = options->colormap->color((*evt)->getMetric(ClusterEvent::COMM, ClusterEvent::WAITALL, ClusterEvent::ALL)
                        / nwaits );
            //painter->setBrush(waitColor);
            painter->fillRect(QRectF(x, yw, w, hw), QBrush(waitColor));
         }

        // Draw border but only if we're doing spacing, otherwise too messy
        if (blockwidth != w) {
            if (nsends)
                painter->drawRect(QRectF(x,ys,w,hs));
            if (nwaits)
                painter->drawRect(QRectF(x,yw,w,hw));
        }

        if (drawMessages) {
            if (nsends)
            {
                painter->setPen(QPen(Qt::black, nsends * 2.0 / pc->members->size(), Qt::SolidLine));
                painter->drawLine(x + blockwidth / 2, ys, x + barwidth, ys - 20);
            }
            if (nwaits)
            {
                float avg_recvs = (*evt)->waitallrecvs / 1.0 / nwaits;
                int angle = 90 * 16 * avg_recvs  / maxWAsize;
                int start = 180 * 16;
                painter->setPen(QPen(Qt::black, 1.0, Qt::SolidLine));
                painter->setBrush(QBrush(Qt::black));
                //painter->drawPie(x + blockwidth * 3 / 4, yw + blockwidth / 4,
                //                 blockwidth / 4, blockwidth / 2,
                //                 start, angle);
                painter->drawPie(x + blockwidth - 16, yw + hw - 12, 25, 25, start, angle);
                painter->drawText(x + blockwidth / 4 - 12, yw + hw + 15, QString::number(avg_recvs, 'g', 2));
                painter->setBrush(QBrush());
                painter->drawPie(x + blockwidth - 16, yw + hw - 12, 25, 25, start, 90 * 16);
                painter->setPen(QPen(Qt::black, 2.0, Qt::SolidLine));
            }
        }

        if (options->showAggregateSteps) {
            xa = floor(((*evt)->step - startStep - 1) * blockwidth) + 1 + startxy.x();
            wa = barwidth;
            ha = barheight * (nsends + nwaits) / 1.0 / pc->members->size();
            ya = base_ya + (barheight - ha) / 2;

            painter->fillRect(QRectF(xa, ya, wa, ha),
                              QBrush(options->colormap->color(
                                                              (*evt)->getMetric(ClusterEvent::AGG, ClusterEvent::BOTH,
                                                                                ClusterEvent::ALL)
                                                              / (*evt)->getCount(ClusterEvent::AGG, ClusterEvent::BOTH,
                                                                                 ClusterEvent::ALL)
                                                              )));
            if (blockwidth != w)
                painter->drawRect(QRectF(xa, ya, wa, ha));
        }


    }
}

void ExchangeGnome::drawGnomeQtClusterSRSR(QPainter * painter, QRect startxy, PartitionCluster * pc,
                                           int barwidth, int barheight, int blockwidth, int blockheight,
                                           int startStep)
{
    if (startxy.height() > 2 * clusterMaxHeight)
    {
        blockheight = clusterMaxHeight;
        barheight = clusterMaxHeight - 3;
    }
    else
    {
        blockheight = startxy.height() / 2;
        if (barheight > blockheight - 3)
            barheight = blockheight;
        else
            barheight = blockheight - 3;
    }
    // Rest of these should not be blockheight now that we've changed blockheight
    int base_y = startxy.y() + startxy.height() / 2 - blockheight;
    int x, y, w, h, xa, wa, xr, yr;
    xr = blockwidth;
    int starti = startStep;
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
    ClusterEvent * evt = pc->events->first();
    int events_index = 0;
    //for (QList<ClusterEvent *>::Iterator evt = pc->events->begin(); evt != pc->events->end(); ++evt)
    for (int i = starti; i < partition->max_global_step; i += 2)
    {
        if (options->showAggregateSteps)
        {
            x = floor((i - startStep) * blockwidth) + 1 + startxy.x();
            xa = floor((i - startStep - 1) * blockwidth) + 1 + startxy.x();
            wa = barwidth;
        }
        else
            x = floor((i - startStep) / 2 * blockwidth) + 1 + startxy.x();
        w = barwidth;
        h = barheight;
        y = base_y;
        if (((i - startStep + 2) / 4) % 2)
            y += blockheight;

        if (evt->step == i)
        {
            // We know it will be complete in this view because we're not doing scrolling or anything here.

            // Draw the event
            painter->fillRect(QRectF(x, y, w, h), QBrush(options->colormap->color(
                                                             evt->getMetric(ClusterEvent::COMM, ClusterEvent::BOTH,
                                                                               ClusterEvent::ALL)
                                                             / evt->getCount(ClusterEvent::COMM, ClusterEvent::BOTH,
                                                                                ClusterEvent::ALL)
                                                             )));

            // Draw border but only if we're doing spacing, otherwise too messy
            if (blockwidth != w)
                painter->drawRect(QRectF(x,y,w,h));

            if (options->showAggregateSteps) {
                painter->fillRect(QRectF(xa, y, wa, h), QBrush(options->colormap->color(
                                                                       evt->getMetric(ClusterEvent::AGG, ClusterEvent::BOTH,
                                                                                         ClusterEvent::ALL)
                                                                       / evt->getCount(ClusterEvent::AGG, ClusterEvent::BOTH,
                                                                                          ClusterEvent::ALL)
                                                                       )));
                if (blockwidth != w)
                    painter->drawRect(QRectF(xa, y, wa, h));
            }

            // Need to draw messages after this, or at least keep track of messages...
            // Maybe check num_sends here?
            if (evt->getCount(ClusterEvent::COMM, ClusterEvent::SEND) > 0)
            {
                if (y == base_y)
                    yr = base_y + blockheight;
                else
                    yr = base_y;
                msgs.append(new DrawMessage(QPoint(x + w/2, y + h/2), QPoint(x + w/2 + xr, yr + h/2),
                                            evt->getCount(ClusterEvent::COMM, ClusterEvent::SEND)));
            }
            if (evt->getCount(ClusterEvent::COMM, ClusterEvent::RECV) > 0 && !msgs.isEmpty())
            {
                DrawMessage * dm = msgs.last();
                dm->nrecvs = evt->getCount(ClusterEvent::COMM, ClusterEvent::RECV);
            }
            events_index++;
            if (events_index < pc->events->size())
                evt = pc->events->at(events_index);
        }
        else // Nothing in the cluster here, draw a dummy
        {
            painter->setPen(QPen(Qt::black, 1.5, Qt::DashLine));
            if (blockwidth != w)
                painter->drawRect(QRectF(x,y,w,h));
            else
                painter->drawRect(QRectF(x+2,y+2,w-4,h-4));
            if (options->showAggregateSteps)
                if (blockwidth != w)
                    painter->drawRect(QRectF(xa, y, wa, h));
                else
                    painter->drawRect(QRectF(xa+2, y+2, wa-4, h-4));
            painter->setPen(QPen(Qt::black, 2.0, Qt::SolidLine));
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
    //std::cout << "I am handling double click for gnome" << std::endl;
    int x = event->x();
    int y = event->y();
    // Figure out which branch this occurs in, open that branch
    for (QMap<PartitionCluster *, QRect>::Iterator p = drawnNodes.begin(); p != drawnNodes.end(); ++p)
        if (p.value().contains(x,y))
        {
            PartitionCluster * pc = p.key();
            //std::cout << "Closing " << pc->memberString().toStdString().c_str() << std::endl;
            pc->close();
            return; // Return so we don't look elsewhere.
        }
    for (QMap<PartitionCluster *, QRect>::Iterator p = drawnPCs.begin(); p != drawnPCs.end(); ++p)
        if (p.value().contains(x,y))
        {
            PartitionCluster * pc = p.key();
            //std::cout << "Opening " << pc->memberString().toStdString().c_str() << std::endl;
            pc->open = true;
            return;
        }
}
