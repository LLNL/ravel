#include "exchangegnome.h"
#include <QPainter>
#include <QRect>
#include <iostream>
#include <climits>
#include <cmath>

ExchangeGnome::ExchangeGnome()
    : Gnome(),
      type(UNKNOWN),
      SRSRmap(QMap<int, int>()),
      SRSRpatterns(QSet<int>()),
      maxWAsize(0)
{
}

ExchangeGnome::~ExchangeGnome()
{

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

    Gnome::preprocess();
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

void ExchangeGnome::generateTopProcesses()
{
    top_processes.clear();
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
        qSort(top_processes);
    }
    else if (type == SSRR || type == SSWA) // Add all partners
    {
        Gnome::generateTopProcesses();
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

void ExchangeGnome::drawGnomeQtClusterEnd(QPainter * painter, QRect clusterRect, PartitionCluster * pc,
                                          int barwidth, int barheight, int blockwidth, int blockheight,
                                          int startStep)
{
    if (type == SRSR)
        drawGnomeQtClusterSRSR(painter, clusterRect, pc, barwidth, barheight, blockwidth, blockheight, startStep);
    else if (type == SSRR)
        drawGnomeQtClusterSSRR(painter, clusterRect, pc, barwidth, barheight, blockwidth, blockheight, startStep);
    else if (type == SSWA)
        drawGnomeQtClusterSSWA(painter, clusterRect, pc, barwidth, barheight, blockwidth, blockheight, startStep);
    else
        Gnome::drawGnomeQtClusterEnd(painter, clusterRect, pc, barwidth, barheight, blockwidth, blockheight, startStep);
}
