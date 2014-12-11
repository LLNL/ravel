#include "exchangegnome.h"
#include <QPainter>
#include <QRect>
#include <iostream>
#include <climits>
#include <cmath>
#include "gnome.h"
#include "partitioncluster.h"
#include "clusterevent.h"
#include "message.h"
#include "colormap.h"
#include "commevent.h"
#include "p2pevent.h"
#include "collectiveevent.h"

ExchangeGnome::ExchangeGnome()
    : Gnome(),
      type(EXCH_UNKNOWN),
      SRSRmap(QMap<int, int>()),
      SRSRpatterns(QSet<int>()),
      maxWAsize(0)
{
}

// Check that the send-to tasks and receive-from tasks are the same.
// Note right now this doesn't check number, only identity, so it could false
// positive if you send-to a task twice but only receive from it once.
bool ExchangeGnome::detectGnome(Partition * part)
{
    bool gnome = true;
    QSet<int> sends = QSet<int>();
    QSet<int> recvs = QSet<int>();
    for (QMap<int, QList<CommEvent *> *>::Iterator event_list
         = part->events->begin();
         event_list != part->events->end(); ++event_list)
    {
        sends.clear();
        recvs.clear();
        for (QList<CommEvent *>::Iterator evt = (event_list.value())->begin();
             evt != (event_list.value())->end(); ++evt)
        {
            QVector<Message *> * msgs = (*evt)->getMessages();
            if (!msgs)
                return false;
            for (QVector<Message *>::Iterator msg = msgs->begin();
                 msg != msgs->end(); ++msg)
            {
                if ((*msg)->sender == (*evt))
                {
                    recvs.insert((*msg)->receiver->task);
                }
                else
                {
                    sends.insert((*msg)->sender->task);
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
    if (type != EXCH_SRSR)
    {
        SRSRmap.clear();
        SRSRpatterns.clear();
    }

    Gnome::preprocess();
}

ExchangeGnome::ExchangeType ExchangeGnome::findType()
{
    int types[EXCH_UNKNOWN + 1];
    for (int i = 0; i <= EXCH_UNKNOWN; i++)
        types[i] = 0;
    for (QMap<int, QList<CommEvent *> *>::Iterator event_list
         = partition->events->begin();
         event_list != partition->events->end(); ++event_list)
    {
        // Odd keeps track of whether the step in the SRSR order is even or
        // odd. We need to keep track of pairs which can be SR or RS, so we use
        // odd to keep track of where we were in the pair.
        // Note this could be kind of off in pF3D because some tasks only
        // send or only recv in a pair... but when they do that, in that pair of
        // pairs they tend to form an SR or an RS, so it works for now.
        bool b_ssrr = true, b_srsr = true, b_sswa = true, first = true,
             sentlast = true, odd = true;
        int recv_dist_2 = 0, recv_dist_not_2 = 0;

        // srsr_pattern compresses the send/recv steps into a single number to
        // be looked at in focus task generation. It puts a 1 where each
        // send is. It doesn't however place where the receives are so again,
        // this might do  something odd for the pair of pairs, which is why
        // this isn't in use yet or maybe ever.
        int srsr_pattern = 0;
        for (QList<CommEvent *>::Iterator evt = (event_list.value())->begin();
             evt != (event_list.value())->end(); ++evt)
        {
            Message * msg = (*evt)->getMessages()->at(0);
            if (first) // For the first message, we don't have a sentlast
            {
                first = false;
                if (msg->receiver == *evt) // Receive
                {
                    sentlast = false;
                    // Can't be these patterns since a task starts on a recv
                    // & we know it must have a send since this is an exchange
                    b_ssrr = false;
                    b_sswa = false;
                }
                odd = false;
            }
            else // For all other messages, check the pattern
            {
                if (msg->receiver == *evt) // Receive
                {
                     // Not alternating
                    if (!sentlast && !odd)
                        b_srsr = false;

                    // Waitall or simmilar
                    else if (sentlast && (*evt)->getMessages()->size() > 1)
                        b_ssrr = false;

                    sentlast = false;

                    // Keep track of largest Waitall
                    if ((*evt)->getMessages()->size() > maxWAsize)
                        maxWAsize = (*evt)->getMessages()->size();
                }
                else // Send
                {
                     // send hapepening after receive
                    if (!sentlast)
                    {
                        b_ssrr = false;
                        b_sswa = false;
                    }
                    // multiple sends in a row without switching
                    else if (sentlast && !odd)
                    {
                        b_srsr = false;
                    }

                    srsr_pattern += 1 << (((*evt)->step
                                           - partition->min_global_step) / 2);
                    sentlast = true;
                    if (msg->receiver->step - msg->sender->step == 2)
                        recv_dist_2++;
                    else
                        recv_dist_not_2++;
                }
                odd = !odd;
            }
            if (!(b_ssrr || b_sswa || b_srsr)) { // Unknown, we can stop now
                types[EXCH_UNKNOWN]++;
                break;
            }
        }

        // Only count as  SRSR if really tight back and forth
        if (b_srsr && recv_dist_not_2 > recv_dist_2)
            b_srsr = false;

        if (b_srsr) {
            types[EXCH_SRSR]++;
            SRSRmap[event_list.key()] = srsr_pattern;
            SRSRpatterns.insert(srsr_pattern);
        }
        if (b_ssrr)
            types[EXCH_SSRR]++;
        if (b_sswa)
            types[EXCH_SSWA]++;
        if (!(b_ssrr || b_sswa || b_srsr))
            types[EXCH_UNKNOWN]++;
    }

    // If we made it down here, we do the final checks to figure out what
    // patterns we have based on all tasks
    if (types[EXCH_SRSR] > 0 && (types[EXCH_SSRR] > 0 || types[EXCH_SSWA] > 0))
        // If we have this going on, too confusing to pick one
        return EXCH_UNKNOWN;
    else if (types[EXCH_UNKNOWN] > types[EXCH_SRSR]
             && types[EXCH_UNKNOWN] > types[EXCH_SSRR]
             && types[EXCH_UNKNOWN] > types[EXCH_SSWA])
        // Mostly unknown
        return EXCH_UNKNOWN;
    else if (types[EXCH_SRSR] > 0)
        // If we make it to here and have SRSR, then the others are 0
        // (based on first if) so we pick it
        return EXCH_SRSR;
    else if (types[EXCH_SSWA] > types[EXCH_SSRR])
        // Some SSWA might look like SSRR and thus be double counted,
        // so we give precedence to SSWA
        return EXCH_SSWA;
    else
        return EXCH_SSRR;
}


// We have a special generator for top tasks for SRSR based on trying to
// find which number of neighbors best represents the different patterns.
// We add all neighbors and then search those partners for the missing strings
// We only do this when the neighbor number has not been otherwise set
// (e.g. by the slider)
void ExchangeGnome::generateTopTasks()
{

    if (type == EXCH_SRSR && neighbors < 0)
    {
        top_tasks.clear();
        QList<CommEvent *> * elist = partition->events->value(max_metric_task);
        QSet<int> add_tasks = QSet<int>();
        QSet<int> level_tasks = QSet<int>();
        QSet<int> patterns = QSet<int>();
        patterns.insert(SRSRmap[max_metric_task]);
        add_tasks.insert(max_metric_task);
        // Always add the 1-neighborhood:
        for (QList<CommEvent *>::Iterator evt = elist->begin();
             evt != elist->end(); ++evt)
        {
            QVector<Message *> * msgs = (*evt)->getMessages();
            for (QVector<Message *>::Iterator msg = msgs->begin();
                 msg != msgs->end(); ++msg)
            {
                if (*evt == (*msg)->sender)
                {
                    if (!add_tasks.contains((*msg)->receiver->task))
                    {
                        add_tasks.insert((*msg)->receiver->task);
                        level_tasks.insert((*msg)->receiver->task);
                        patterns.insert(SRSRmap[(*msg)->receiver->task]);
                    }
                }
                else
                {
                    if (!add_tasks.contains((*msg)->sender->task))
                    {
                        add_tasks.insert((*msg)->sender->task);
                        level_tasks.insert((*msg)->sender->task);
                        patterns.insert(SRSRmap[(*msg)->sender->task]);
                    }
                }
            }
        }

        // Now check, do we have most of the patterns?
        int neighbor_depth = 1;
        while (patterns.size() / 1.0 / SRSRpatterns.size() < 0.5)
        {
            QList<int> check_group = level_tasks.toList();
            level_tasks.clear();

            // Future, possibly sort by something else than task id, like metric
            qSort(check_group);

            // Now see about adding things from the check group, as long as they
            // haven't already been added
            for (QList<int>::Iterator proc = check_group.begin();
                 proc != check_group.end(); ++proc)
            {
                elist = partition->events->value(*proc);
                for (QList<CommEvent *>::Iterator evt = elist->begin();
                     evt != elist->end(); ++evt)
                {
                    QVector<Message *> * msgs = (*evt)->getMessages();
                    for (QVector<Message *>::Iterator msg
                         = msgs->begin();
                         msg != msgs->end(); ++msg)
                    {
                        if (*evt == (*msg)->sender)
                        {
                            if (!add_tasks.contains((*msg)->receiver->task))
                            {
                                add_tasks.insert((*msg)->receiver->task);
                                level_tasks.insert((*msg)->receiver->task);
                                patterns.insert(SRSRmap[(*msg)->receiver->task]);
                            }
                        }
                        else
                        {
                            if (!add_tasks.contains((*msg)->sender->task))
                            {
                                add_tasks.insert((*msg)->sender->task);
                                level_tasks.insert((*msg)->sender->task);
                                patterns.insert(SRSRmap[(*msg)->sender->task]);
                            }
                        }
                    }
                }
            } // checked everything in check_group

            check_group.clear();
            neighbor_depth++;
        }
        top_tasks += add_tasks.toList();
        qSort(top_tasks);
        // We need to somehow get the neighbor_depth (neighbor_radius) set on
        // this gnome and known by the tree vis
        neighbors = neighbor_depth;
    }
    else
    {
        Gnome::generateTopTasks();
    }
}


// Probably need to do this for the isend -> waitall pattern...
// Replace the receive line with a pie indicating the size of the waitall
void ExchangeGnome::drawGnomeQtClusterSSWA(QPainter * painter, QRect startxy,
                                           PartitionCluster * pc,
                                           int barwidth, int barheight,
                                           int blockwidth, int blockheight,
                                           int startStep)
{
    // For this we only need one row of events but we probably want to have
    // some extra room for all of the messages that happen
    bool drawMessages = true;
    if (startxy.height() > 2 * clusterMaxHeight)
    {
        blockheight = 2*(clusterMaxHeight - 20);
        barheight = blockheight; // - 3;
    }
    else
    {
        blockheight = startxy.height() / 2;
        if (blockheight < 40)
            drawMessages = false;
        else
            blockheight -= 20; // Room for message drawing
        if (barheight > blockheight - 3)
            barheight = blockheight;
    }
    int x, ys, yw, w, hs, hw, xa, wa, nsends, nwaits;
    int base_y = startxy.y() + startxy.height() / 2 - blockheight / 2;
    if (options->showAggregateSteps) {
        startStep -= 1;
    }
    painter->setPen(QPen(Qt::black, 1.0, Qt::SolidLine));
    for (QList<ClusterEvent *>::Iterator evt = pc->events->begin();
         evt != pc->events->end(); ++evt)
    {
        if (options->showAggregateSteps)
            x = floor(((*evt)->step - startStep) * blockwidth) + 1
                + startxy.x();
        else
            x = floor(((*evt)->step - startStep) / 2 * blockwidth) + 1
                + startxy.x();
        w = barwidth;
        nsends = (*evt)->getCount(ClusterEvent::CE_EVENT_COMM, ClusterEvent::CE_COMM_SEND);
        nwaits = (*evt)->getCount(ClusterEvent::CE_EVENT_COMM, ClusterEvent::CE_COMM_WAITALL);
        int divisor = pc->members->size();
        if (!options->showInactiveSteps)
            divisor = nsends + nwaits;

        hs = blockheight * nsends / 1.0 / divisor;
        ys = base_y;
        hw = blockheight * nwaits / 1.0 / divisor;
        yw = base_y + blockheight - hw;

        // Draw the event
        if (nsends) {
            QColor sendColor = options->colormap->color((*evt)->getMetric(ClusterEvent::CE_EVENT_COMM,
                                                                          ClusterEvent::CE_COMM_SEND,
                                                                          ClusterEvent::CE_THRESH_BOTH)
                                                        / nsends);
            painter->fillRect(QRectF(x, ys, w, hs), QBrush(sendColor));
        }
         if (nwaits)
         {
            QColor waitColor = options->colormap->color((*evt)->getMetric(ClusterEvent::CE_EVENT_COMM,
                                                                          ClusterEvent::CE_COMM_WAITALL,
                                                                          ClusterEvent::CE_THRESH_BOTH)
                                                        / nwaits );
            painter->fillRect(QRectF(x, yw, w, hw), QBrush(waitColor));
         }

        // Draw border but only if we're doing spacing, otherwise too messy
        if (blockwidth != w) {
            painter->setPen(QPen(Qt::black, 1.0, Qt::SolidLine));
            painter->drawRect(QRect(x, ys, w, blockheight));
        }

        // Draw the sends as normal, draw the waitalls as a partially filled
        // pie and a number label
        // Maybe we want to move the number label inside some day?
        if (drawMessages) {
            if (nsends)
            {
                painter->setPen(QPen(Qt::black, nsends * 2.0 / divisor,
                                     Qt::SolidLine));
                painter->drawLine(x + blockwidth / 2, ys, x + barwidth,
                                  ys - 20);
            }
            if (nwaits)
            {
                float avg_recvs = (*evt)->waitallrecvs / 1.0 / nwaits;
                int angle = 90 * 16 * avg_recvs  / maxWAsize;
                int start = 180 * 16;
                painter->setPen(QPen(Qt::black, 1.0, Qt::SolidLine));
                painter->setBrush(QBrush(Qt::black));
                painter->drawPie(x + blockwidth - 16,
                                 base_y + blockheight - 12,
                                 25, 25, start, angle);
                painter->drawText(x + blockwidth / 4 - 12,
                                  base_y + blockheight + 15,
                                  QString::number(avg_recvs, 'g', 2));
                painter->setBrush(QBrush());
                painter->drawPie(x + blockwidth - 16,
                                 base_y + blockheight - 12,
                                 25, 25, start, 90 * 16);
                painter->setPen(QPen(Qt::black, 1.0, Qt::SolidLine));
            }
        }

        // Repeat for aggregate step which has no messages of course
        if (options->showAggregateSteps) {
            xa = floor(((*evt)->step - startStep - 1) * blockwidth) + 1
                 + startxy.x();
            wa = barwidth;

            if (nsends)
                painter->fillRect(QRectF(xa, ys, wa, hs),
                              QBrush(options->colormap->color((*evt)->getMetric(ClusterEvent::CE_EVENT_AGG,
                                                                                ClusterEvent::CE_COMM_SEND,
                                                                                ClusterEvent::CE_THRESH_BOTH)
                                                              / nsends)));

            if (nwaits)
                painter->fillRect(QRectF(xa, yw, wa, hw),
                              QBrush(options->colormap->color((*evt)->getMetric(ClusterEvent::CE_EVENT_AGG,
                                                                                ClusterEvent::CE_COMM_WAITALL,
                                                                                ClusterEvent::CE_THRESH_BOTH)
                                                              / nwaits)));

            if (blockwidth != w)
            {
                painter->setPen(QPen(Qt::black, 1.0, Qt::SolidLine));
                painter->drawRect(QRect(xa, ys, wa, blockheight));
            }
        }


    }
}

// For SRSR instead of doing a single combined box for each step, we divide in
// half and draw each half as either the active or waiting task. All active
// tasks are aggregated onto the active step in the drawing, regardless of
// what they are doing. This is the most distoring because it is showing a
// simplified diagram of what is not going on to give the general idea.
void ExchangeGnome::drawGnomeQtClusterSRSR(QPainter * painter, QRect startxy,
                                           PartitionCluster * pc,
                                           int barwidth, int barheight,
                                           int blockwidth, int blockheight,
                                           int startStep)
{
    // Unlike others, this doesn't need to leave room for outer messages
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


    int base_y = startxy.y() + startxy.height() / 2 - blockheight;
    int x, y, w, h, xa, wa, xr, yr;
    xr = blockwidth;
    int starti = startStep;
    if (options->showAggregateSteps) {
        startStep -= 1;
        xr *= 2;
    }
    painter->setPen(QPen(Qt::black, 2.0, Qt::SolidLine));
    QList<DrawMessage *> msgs = QList<DrawMessage *>();
    ClusterEvent * evt = pc->events->first();
    int events_index = 0;

    // Unlike our other drawing methods, we walk through based on steps and
    // advance the ClusterEvent separately. This isn't particularly
    // functionally different but puts the emphasis on the step over the event.
    // At least that's what I assume I was thinking.
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

        // Every other set is in the second lane
        if (((i - startStep + 2) / 4) % 2)
            y += blockheight;

        // If we have an event at this space, otherwise draw a dummy
        if (evt->step == i && evt->getCount())
        {

            // Draw the event
            painter->fillRect(QRectF(x, y, w, h),
                              QBrush(options->colormap->color(evt->getMetric(ClusterEvent::CE_EVENT_COMM,
                                                                             ClusterEvent::CE_COMM_ALL,
                                                                             ClusterEvent::CE_THRESH_BOTH)
                                                             / evt->getCount(ClusterEvent::CE_EVENT_COMM,
                                                                             ClusterEvent::CE_COMM_ALL,
                                                                             ClusterEvent::CE_THRESH_BOTH)
                                                             )));

            // Draw border but only if we're doing spacing, otherwise too messy
            if (blockwidth != w)
                painter->drawRect(QRectF(x,y,w,h));

            // Draw the aggregate & border too if necessary
            if (options->showAggregateSteps) {
                painter->fillRect(QRectF(xa, y, wa, h),
                                  QBrush(options->colormap->color(evt->getMetric(ClusterEvent::CE_EVENT_AGG,
                                                                                 ClusterEvent::CE_COMM_ALL,
                                                                                 ClusterEvent::CE_THRESH_BOTH)
                                                                  / evt->getCount(ClusterEvent::CE_EVENT_AGG,
                                                                                  ClusterEvent::CE_COMM_ALL,
                                                                                  ClusterEvent::CE_THRESH_BOTH)
                                                                       )));
                if (blockwidth != w)
                    painter->drawRect(QRectF(xa, y, wa, h));
            }

            // Save message info here to be drawn later. We get its weight as
            // well as its start and end positions
            if (evt->getCount(ClusterEvent::CE_EVENT_COMM, ClusterEvent::CE_COMM_SEND) > 0)
            {
                if (y == base_y)
                    yr = base_y + blockheight;
                else
                    yr = base_y;
                msgs.append(new DrawMessage(QPoint(x + w/2, y + h/2),
                                            QPoint(x + w/2 + xr, yr + h/2),
                                            evt->getCount(ClusterEvent::CE_EVENT_COMM,
                                                          ClusterEvent::CE_COMM_SEND)));
            }
            if (evt->getCount(ClusterEvent::CE_EVENT_COMM, ClusterEvent::CE_COMM_RECV) > 0
                    && !msgs.isEmpty())
            {
                DrawMessage * dm = msgs.last();
                dm->nrecvs = evt->getCount(ClusterEvent::CE_EVENT_COMM,
                                           ClusterEvent::CE_COMM_RECV);
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
    for (QList<DrawMessage *>::Iterator dm = msgs.begin();
         dm != msgs.end(); ++dm)
    {
        painter->setPen(QPen(Qt::black,
                             (*dm)->nsends * 2.0 / pc->members->size(),
                             Qt::SolidLine));
        painter->drawLine((*dm)->send, (*dm)->recv);
    }

    // Clean up the draw messages...
    for (QList<DrawMessage *>::Iterator dm = msgs.begin();
         dm != msgs.end(); ++dm)
    {
        delete *dm;
    }
}


// Special drawing styles for exchange types
// SSRR does not need a special drawing type
void ExchangeGnome::drawGnomeQtClusterEnd(QPainter * painter, QRect clusterRect,
                                          PartitionCluster * pc,
                                          int barwidth, int barheight,
                                          int blockwidth, int blockheight,
                                          int startStep)
{
    if (type == EXCH_SRSR)
        drawGnomeQtClusterSRSR(painter, clusterRect, pc, barwidth, barheight,
                               blockwidth, blockheight, startStep);
    else if (type == EXCH_SSWA)
        drawGnomeQtClusterSSWA(painter, clusterRect, pc, barwidth, barheight,
                               blockwidth, blockheight, startStep);
    else
        Gnome::drawGnomeQtClusterEnd(painter, clusterRect, pc, barwidth,
                                     barheight, blockwidth, blockheight,
                                     startStep);
}
