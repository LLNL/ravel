//////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2014, Lawrence Livermore National Security, LLC.
// Produced at the Lawrence Livermore National Laboratory.
//
// This file is part of Ravel.
// Written by Kate Isaacs, kisaacs@acm.org, All rights reserved.
// LLNL-CODE-663885
//
// For details, see https://github.com/scalability-llnl/ravel
// Please also see the LICENSE file for our notice and the LGPL.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License (as published by
// the Free Software Foundation) version 2.1 dated February 1999.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the IMPLIED WARRANTY OF
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the terms and
// conditions of the GNU General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program; if not, write to the Free Software Foundation,
// Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
//////////////////////////////////////////////////////////////////////////////
#include "gnome.h"
#include <QElapsedTimer>
#include <QLocale>
#include <QMouseEvent>
#include <iostream>
#include <climits>
#include <cmath>
#include "kmedoids.h"

#include "p2pevent.h"
#include "clusterevent.h"
#include "message.h"
#include "colormap.h"
#include "ravelutils.h"

using namespace cluster;

Gnome::Gnome()
    : partition(NULL),
      options(NULL),
      seed(0),
      mousex(-1),
      mousey(-1),
      metric("Lateness"),
      cluster_leaves(NULL),
      cluster_map(NULL),
      cluster_root(NULL),
      max_metric_entity(-1),
      top_entities(QList<int>()),
      alternation(true),
      neighbors(-1),
      saved_messages(QSet<Message *>()),
      drawnPCs(QMap<PartitionCluster *, QRect>()),
      drawnNodes(QMap<PartitionCluster *, QRect>()),
      drawnEvents(QMap<Event *, QRect>()),
      selected_pc(NULL),
      is_selected(false),
      hover_event(NULL),
      hover_aggregate(false),
      stepwidth(0)
{
}

Gnome::~Gnome()
{
    if (cluster_root) {
        cluster_root->delete_tree();
        delete cluster_root;
        delete cluster_leaves;
        delete cluster_map;
    }
}

bool Gnome::detectGnome(Partition * part)
{
    Q_UNUSED(part);
    return false;
}

Gnome * Gnome::create()
{
    return new Gnome();
}


// Should be called initially and whenever the metric changes so it can
// recluster, generate top entities, etc
void Gnome::preprocess()
{
    if (partition && partition->events->size() > 20)
    {
        findMusters();
        for (QMap<int, PartitionCluster *>::Iterator pc
             = cluster_leaves->begin(); pc != cluster_leaves->end(); ++pc)
        {
            (pc.value())->makeClusterVectors();
            for (QList<int>::Iterator member = (pc.value())->members->begin();
                 member != (pc.value())->members->end(); ++member)
            {
                (*cluster_map)[*member] = pc.value();
            }
        }
        hierarchicalMusters();
    }
    else
    {
        findClusters();
    }
    generateTopEntities();
}

// Clustering using Muster
void Gnome::findMusters()
{
    QElapsedTimer traceTimer;
    qint64 traceElapsed;

    traceTimer.start();
    top_entities.clear();
    if (cluster_root)
    {
        cluster_root->delete_tree();
        delete cluster_root;
        delete cluster_leaves;
        delete cluster_map;
    }
    long long int metric, max_metric = LLONG_MIN;
    max_metric_entity = -1;
    QString cmetric = "Lateness";
    if (options)
        cmetric = options->metric;

    int num_clusters = std::min(20, partition->events->size());

    kmedoids clara;
    clara.set_seed(seed);
    clara.clara(partition->cluster_entities->toStdVector(),
                entity_distance(), num_clusters);

    /* // (Fail to) generate optimal cluster number
    int dim = (partition->max_global_step - partition->min_global_step)/2 + 1;
    std::vector<ClusterEntity> * xvector = new std::vector<ClusterEntity>();
    for (int i = 0; i < partition->cluster_entities->size(); i++)
        xvector->push_back(*(partition->cluster_entities->at(i)));
    clara.xclara(*xvector, entity_distance_np(), num_clusters, dim);
    std::cout << "XClara found " << clara.medoid_ids.size()
              << " clusters" << std::endl;
    */

    // Set up clusters structures
    cluster_leaves = new QMap<int, PartitionCluster *>();
    cluster_map = new QMap<int, PartitionCluster *>();
    for (int i = 0; i < num_clusters; i++) // TODO: Make cluster_leaves a list
        cluster_leaves->insert(i,
                               new PartitionCluster(partition->max_global_step
                                                    - partition->min_global_step
                                                    + 2,
                                                    partition->min_global_step));
    for (int i = 0; i < clara.cluster_ids.size(); i++)
    {
        int entity = partition->cluster_entities->at(i)->entity;
        metric = cluster_leaves->value(clara.cluster_ids[i])->addMember(partition->cluster_entities->at(i),
                                                                        partition->events->value(entity),
                                                                        cmetric);
        if (metric > max_metric)
        {
            max_metric = metric;
            max_metric_entity = entity;
        }
    }
    traceElapsed = traceTimer.nsecsElapsed();
    RavelUtils::gu_printTime(traceElapsed, "Musterizing: ");
}

// Once clusters have been determined by muster, do the remaining as hierarchy
// We do single linkage so we don't calculate much
void Gnome::hierarchicalMusters()
{
    QElapsedTimer traceTimer;
    qint64 traceElapsed;

    traceTimer.start();
    // Calculate initial distances
    QList<DistancePair> distances;

    // muster-distances
    long long int distance;
    for (int i = 0; i < cluster_leaves->size(); i++)
    {
        for (int j = i + 1; j < cluster_leaves->size(); j++)
        {
            distance = cluster_leaves->value(i)->distance(cluster_leaves->value(j));
            distances.append(DistancePair(distance, i, j));
        }
    }
    qSort(distances); // so we do smallest distance first

    // create hierarchy
    int lastp = distances[0].p1;
    PartitionCluster * pc = NULL;
    for (int i = 0; i < distances.size(); i++)
    {
        DistancePair current = distances[i];
        if (cluster_leaves->value(current.p1)->get_root()
            != cluster_leaves->value(current.p2)->get_root())
        {
            pc = new PartitionCluster(current.distance,
                                      cluster_leaves->value(current.p1)->get_root(),
                                      cluster_leaves->value(current.p2)->get_root());
            lastp = current.p1;
        }
    }
    cluster_root = cluster_leaves->value(lastp)->get_root();

    traceElapsed = traceTimer.nsecsElapsed();
    RavelUtils::gu_printTime(traceElapsed, "Hierarchical mustering: ");

    // From here we could now compress the ClusterEvent metrics (doing the four
    // divides ahead of time)but I'm going to retain the information for now
    // and see how it goes
}

// Straigth SLINK hierarchy, can take a long time for large #entities or #steps
void Gnome::findClusters()
{
    // Calculate initial distances
    QList<DistancePair> distances;
    QList<unsigned long> entities = partition->events->keys();
    top_entities.clear();
    if (cluster_root)
    {
        cluster_root->delete_tree();
        delete cluster_root;
        delete cluster_leaves;
        delete cluster_map;
    }

    // Create PartitionClusters for leaves and create distance list
    cluster_leaves = new QMap<int, PartitionCluster *>();
    cluster_map = new QMap<int, PartitionCluster *>();
    long long int max_metric = LLONG_MIN;
    max_metric_entity = -1;
    qSort(entities);
    int num_entities = entities.size();
    int p1, p2;
    long long int distance;
    for (int i = 0; i < num_entities; i++)
    {
        p1 = entities[i];
        cluster_leaves->insert(p1, new PartitionCluster(p1,
                                                        partition->events->value(p1),
                                                        "Lateness"));
        cluster_map->insert(p1, cluster_leaves->value(p1));
        if (cluster_leaves->value(p1)->max_metric > max_metric)
        {
            max_metric = cluster_leaves->value(p1)->max_metric;
            max_metric_entity = p1;
        }
        for (int j = i + 1; j < num_entities; j++)
        {
            p2 = entities[j];
            distance = calculateMetricDistance(p1,
                                               p2);
            distances.append(DistancePair(distance, p1, p2));
        }
    }
    qSort(distances); // so we do shortest distance first

    // build hierarchy
    int lastp = distances[0].p1;
    QList<long long int> cluster_distances = QList<long long int>();
    PartitionCluster * pc = NULL;
    for (int i = 0; i < distances.size(); i++)
    {
        DistancePair current = distances[i];
        if (cluster_leaves->value(current.p1)->get_root()
                != cluster_leaves->value(current.p2)->get_root())
        {
            pc = new PartitionCluster(current.distance,
                                      cluster_leaves->value(current.p1)->get_root(),
                                      cluster_leaves->value(current.p2)->get_root());
            cluster_distances.append(current.distance);
            lastp = current.p1;
        }
    }
    cluster_root = cluster_leaves->value(lastp)->get_root();

    // From here we could now compress the ClusterEvent metrics (doing the four
    // divides ahead of time) but I'm going to retain the information for now
    // and see how it goes
}

// When calculating distance between two event lists. When one is missing a step,
// webbestimate the lateness as the step that came before it if available
// and only if not we skip
long long int Gnome::calculateMetricDistance(int p1, int p2)
{
    int start1 = partition->cluster_step_starts->value(p1);
    int start2 = partition->cluster_step_starts->value(p2);
    QVector<long long int> * events1 = partition->cluster_vectors->value(p1);
    QVector<long long int> * events2 = partition->cluster_vectors->value(p2);
    int num_matches = events1->size();
    long long int total_difference = 0;
    int offset = 0;
    if (start1 < start2)
    {
        num_matches = events2->size();
        offset = events1->size() - events2->size();
        for (int i = 0; i < events2->size(); i++)
            total_difference += (events1->at(offset + i) - events2->at(i))
                                * (events1->at(offset + i) - events2->at(i));
    }
    else
    {
        offset = events2->size() - events1->size();
        for (int i = 0; i < events1->size(); i++)
            total_difference += (events2->at(offset + i) - events1->at(i))
                                * (events2->at(offset + i) - events1->at(i));
    }
    if (num_matches <= 0)
        return LLONG_MAX;
    return total_difference / num_matches;
}

// Old distance metric where we skip any non-matching steps. Also this was done
// before cluster_vectors were written in so the indexing is a little more
// complicated
long long int Gnome::calculateMetricDistance2(QList<CommEvent *> * list1,
                                              QList<CommEvent *> * list2)
{
    int index1 = 0, index2 = 0, total_calced_steps = 0;
    CommEvent * evt1 = list1->at(0), * evt2 = list2->at(0);
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
        return LLONG_MAX; //0;
    }
    return total_difference / total_calced_steps;
}

// Neighbor radius
void Gnome::setNeighbors(int _neighbors)
{
    if (neighbors == _neighbors)
        return;

    neighbors = _neighbors;
    generateTopEntities();
}

// Find the focus entities
void Gnome::generateTopEntities(PartitionCluster *pc)
{
    top_entities.clear();
    if (neighbors < 0)
        neighbors = 1;

    if (pc)
    {
        if (options->topByCentroid)
            generateTopEntitiesWorker(findCentroidEntity(pc));
        else
            generateTopEntitiesWorker(findMaxMetricEntity(pc));
    }
    else
        generateTopEntitiesWorker(max_metric_entity);
    qSort(top_entities);
}

// Sets top_entities to a list w/entity and its neighbors-hop neighborhood
void Gnome::generateTopEntitiesWorker(int entity)
{
    QList<CommEvent *> * elist = NULL;
    QSet<int> add_entities = QSet<int>();
    add_entities.insert(entity);
    QSet<int> new_entities = QSet<int>();
    QSet<int> current_entities = QSet<int>();
    current_entities.insert(entity);
    for (int i = 0; i < neighbors; i++)
    {
        for (QSet<int>::Iterator proc = current_entities.begin();
             proc != current_entities.end(); ++proc)
        {
            elist = partition->events->value(*proc);
            for (QList<CommEvent *>::Iterator evt = elist->begin();
                 evt != elist->end(); ++evt)
            {
                // Should happen at the evt level
                QVector<Message *> * msgs = (*evt)->getMessages();
                if (!msgs)
                    continue;
                for (QVector<Message *>::Iterator msg
                     = msgs->begin();
                     msg != msgs->end(); ++msg)
                {
                    if (*evt == (*msg)->sender)
                    {
                        add_entities.insert((*msg)->receiver->entity);
                        new_entities.insert((*msg)->receiver->entity);
                    }
                    else
                    {
                        add_entities.insert((*msg)->sender->entity);
                        new_entities.insert((*msg)->receiver->entity);
                    }
                }
            }
        }
        current_entities.clear();
        current_entities += new_entities;
        new_entities.clear();
    }
    top_entities = add_entities.toList();
}

// TODO: Make this a per-PC thing.
int Gnome::findMaxMetricEntity(PartitionCluster * pc)
{
    return pc->max_entity;
}

// Calculates the centroid of a partition cluster by first finding the average
// and then finding the closest member to the average
int Gnome::findCentroidEntity(PartitionCluster * pc)
{
    // Init
    QList<CentroidDistance> distances = QList<CentroidDistance>();
    for (int i = 0; i < pc->members->size(); i++)
        distances.append(CentroidDistance(0, pc->members->at(i)));

    // Take the average from the top
    QList<AverageMetric> events = QList<AverageMetric>();
    for (QList<ClusterEvent *>::Iterator evt = pc->events->begin();
         evt != pc->events->end(); ++evt)
    {
        if ((*evt)->getCount())
            events.append(AverageMetric((*evt)->getMetric()
                                        / (*evt)->getCount(), (*evt)->step));
    }

    // Find the distances, note there may be a cleaner way to write this now
    // that more structures have been added
    int num_events = events.size();
    for (int i = 0; i < distances.size(); i++)
    {
        int index1 = 0, index2 = 0, total_calced_steps = 0;
        long long int last1 = 0, last2 = 0, total_difference = 0;
        int entity = distances[i].entity;
        CommEvent * evt = partition->events->value(entity)->first();

        while (evt && index2 < num_events)
        {
            AverageMetric am = events[index2];
            if (evt->step == am.step) // If they're equal, add their distance
            {
                last1 = evt->getMetric(metric);
                last2 = am.metric;
                total_difference += (last1 - last2) * (last1 - last2);
                ++total_calced_steps;
                // Increment both event lists now
                ++index2;
                ++index1;
                if (index1 < partition->events->value(entity)->size())
                    evt = partition->events->value(entity)->at(index1);
                else
                    evt = NULL;
            } else if (evt->step > am.step) { // If not, increment steps until they match
                // Estimate evt1 lateness
                last2 = am.metric;
                if (evt->comm_prev && evt->comm_prev->partition == evt->partition)
                {
                    total_difference += (last1 - last2) * (last1 - last2);
                    ++total_calced_steps;
                }

                // Move evt2 forward
                ++index2;
            } else {
                last1 = evt->getMetric(metric);
                if (index2 > 0)
                {
                    total_difference += (last1 - last2) * (last1 - last2);
                    ++total_calced_steps;
                }

                // Move evt1 forward
                ++index1;
                if (index1 < partition->events->value(entity)->size())
                    evt = partition->events->value(entity)->at(index1);
                else
                    evt = NULL;
            }
        }
        if (total_calced_steps == 0)
        {
            distances[i].distance = LLONG_MAX;
        }
        else
        {
            distances[i].distance = total_difference / total_calced_steps;
        }
    }
    qSort(distances);
    return distances[0].entity;
}


void Gnome::drawGnomeQt(QPainter * painter, QRect extents,
                        VisOptions *_options, int blockwidth)
{
    options = _options;
    if (options->metric != metric)
    {
        metric = options->metric;
        preprocess();
    }
    saved_messages.clear();
    drawnPCs.clear();
    drawnNodes.clear();
    drawnEvents.clear();

    drawGnomeQtCluster(painter, extents, blockwidth);
}

// The height allowed to the top entities (of the total Gnome drawing height).
// This is not the y but the height.
// This could be improved.
int Gnome::getTopHeight(QRect extents)
{
    int topHeight = 0;
    int fair_portion = top_entities.size() / 1.0
                       / cluster_root->members->size() * extents.height();
    int min_size = 12 * top_entities.size();

    // If we don't have enough for 12 pixels each entity,
    // go with the fair portion
    if (min_size > extents.height())
        topHeight = fair_portion;
    else // but if we do, go with whatever is bigger
        topHeight = std::max(fair_portion, min_size);

    // Max cluster leftover tries the other way - seeing how much room we need
    // for our clusters and then choosing the size based on them. Note however
    // that this is not how the clusters will actually be allotted in terms of
    // size (since they're shown relative to how many they contain), so
    // this may not make sense. Perhaps we need a non-linear cluster scale.
    int max_cluster_leftover = extents.height()
                               - cluster_root->visible_clusters()
                               * (2 * clusterMaxHeight);
    if (topHeight < max_cluster_leftover)
        topHeight = max_cluster_leftover;

    return topHeight;
}

void Gnome::drawGnomeQtCluster(QPainter * painter,
                               QRect extents,
                               int blockwidth)
{
    alternation = true;


    int topHeight = getTopHeight(extents);

    int effectiveHeight = extents.height() - topHeight;
    int effectiveWidth = extents.width();

    int entitiespan = partition->events->size();
    int stepSpan = partition->max_global_step - partition->min_global_step + 2;
    int spacingMinimum = 12;


    int entity_spacing = 0;
    if (effectiveHeight / entitiespan > spacingMinimum)
        entity_spacing = 3;

    int step_spacing = 0;
    if (effectiveWidth / stepSpan + 1 > spacingMinimum)
        step_spacing = 3;


    float blockheight = effectiveHeight / 1.0 / entitiespan;
    if (blockheight >= 1.0)
        blockheight = floor(blockheight);
    int barheight = blockheight - entity_spacing;
    int barwidth = blockwidth - step_spacing;
    painter->setPen(QPen(QColor(0, 0, 0)));

    // Draw the Focus entities
    QRect top_extents = QRect(extents.x(), extents.y(),
                              extents.width(), topHeight);
    drawGnomeQtTopEntities(painter, top_extents, blockwidth, barwidth);

    // Draw the clusters
    QRect cluster_extents = QRect(extents.x(), extents.y() + topHeight,
                                  extents.width(), effectiveHeight);
    drawGnomeQtClusterBranch(painter, cluster_extents, cluster_root,
                             blockheight, blockwidth, barheight, barwidth);

    // Now that we have drawn all the events, we need to draw the leaf-cluster
    // messages or the leaf-leaf messages which are saved in saved_messages.
    drawGnomeQtInterMessages(painter, blockwidth,
                             partition->min_global_step, extents.x());

    drawHover(painter);

}

// Entity labels for the focus entities
void Gnome::drawTopLabels(QPainter * painter, QRect extents)
{
    int topHeight = getTopHeight(extents);
    int entitiespan = top_entities.size();

    float blockheight = floor(topHeight / entitiespan);

    QLocale systemlocale = QLocale::system();
    QFontMetrics font_metrics = painter->fontMetrics();
    QString testString = systemlocale.toString(top_entities.last());
    int labelWidth = font_metrics.width(testString);
    int labelHeight = font_metrics.height();

    int x = extents.width() - labelWidth - 2;

    painter->setPen(Qt::black);
    painter->setFont(QFont("Helvetica", 10));
    int total_labels = floor(topHeight / labelHeight);
    int y;
    int skip = 1;
    if (total_labels < entitiespan && total_labels > 0)
    {
        skip = ceil(float(entitiespan) / total_labels);
    }

    if (total_labels > 0)
    {
        for (int i = 0; i < top_entities.size(); i+= skip)
        {
            y = floor(i * blockheight) + (blockheight + labelHeight) / 2 + 1;
            if (y < topHeight)
                painter->drawText(x, y, QString::number(top_entities[i]));
        }
    }
}


// Draw focus entities much like StepVis
void Gnome::drawGnomeQtTopEntities(QPainter * painter, QRect extents,
                                            int blockwidth, int barwidth)
{
    int effectiveHeight = extents.height();

    int entity_spacing = blockwidth - barwidth;
    int step_spacing = entity_spacing;

    float x, y, w, h, xa, wa;
    float blockheight = floor(effectiveHeight / top_entities.size());
    int startStep = partition->min_global_step;
    if (options->showAggregateSteps)
    {
        startStep -= 1;
    }
    float barheight = blockheight - entity_spacing;
    stepwidth = blockwidth;

    QSet<Message *> drawMessages = QSet<Message *>();
    painter->setPen(QPen(QColor(0, 0, 0)));
    QMap<int, int> entityYs = QMap<int, int>();
    float myopacity, opacity = 1.0;
    if (is_selected && selected_pc)
        opacity = 0.5;
    for (int i = 0; i < top_entities.size(); ++i)
    {
        QList<CommEvent *> * event_list = partition->events->value(top_entities[i]);
        bool selected = false;
        if (is_selected && selected_pc
            && selected_pc->members->contains(top_entities[i]))
        {
            selected = true;
        }
        y =  floor(extents.y() + i * blockheight) + 1;

        entityYs[top_entities[i]] = y;
        for (QList<CommEvent *>::Iterator evt = event_list->begin();
             evt != event_list->end(); ++evt)
        {
            if (options->showAggregateSteps)
                x = floor(((*evt)->step - startStep) * blockwidth) + 1
                    + extents.x();
            else
                x = floor(((*evt)->step - startStep) / 2 * blockwidth) + 1
                    + extents.x();
            w = barwidth;
            h = barheight;

            myopacity = opacity;
            if (selected)
                myopacity = 1.0;
            painter->fillRect(QRectF(x, y, w, h),
                              QBrush(options->colormap->color((*evt)->getMetric(metric),
                                                              myopacity)));

            // Draw border but only if we're doing spacing, otherwise too messy
            painter->setPen(QPen(QColor(0, 0, 0, myopacity * 255)));
            if (step_spacing > 0 && entity_spacing > 0)
            {
                painter->drawRect(QRectF(x,y,w,h));
            }

            // Change to commBundle method
            QVector<Message *> * msgs = (*evt)->getMessages();
            if (msgs)
                for (QVector<Message *>::Iterator msg = msgs->begin();
                     msg != msgs->end(); ++msg)
                {
                    if (top_entities.contains((*msg)->sender->entity)
                            && top_entities.contains((*msg)->receiver->entity))
                    {
                        drawMessages.insert((*msg));
                    }
                    else if (*evt == (*msg)->sender)
                    {
                        // send
                        if ((*msg)->sender->entity > (*msg)->receiver->entity)
                            painter->drawLine(x + w/2, y + h/2, x + w, y);
                        else
                            painter->drawLine(x + w/2, y + h/2, x + w, y + h);
                    }
                    else
                    {
                        // recv
                        if ((*msg)->sender->entity > (*msg)->receiver->entity)
                            painter->drawLine(x + w/2, y + h/2, x, y + h);
                        else
                            painter->drawLine(x + w/2, y + h/2, x, y);
                    }
                }

            if (options->showAggregateSteps) {
                xa = floor(((*evt)->step - startStep - 1) * blockwidth) + 1 + extents.x();
                wa = barwidth;

                painter->fillRect(QRectF(xa, y, wa, h),
                                  QBrush(options->colormap->color((*evt)->getMetric(metric,
                                                                                    true),
                                                                  myopacity)));

                if (step_spacing > 0 && entity_spacing > 0)
                {
                    painter->drawRect(QRectF(xa, y, wa, h));
                }
                drawnEvents[*evt] = QRect(xa, y, (x - xa) + w, h);
            } else {
                // For selection
                drawnEvents[*evt] = QRect(x, y, w, h);
            }

        }
    }

    // Messages
    // We need to do all of the message drawing after the event drawing
    // for overlap purposes. We draw whether or not the vis options say we should
    // because we are just for the pattern idea

        if (top_entities.size() <= 32)
            painter->setPen(QPen(Qt::black, 2, Qt::SolidLine));
        else
            painter->setPen(QPen(Qt::black, 1, Qt::SolidLine));
        P2PEvent * send_event;
        P2PEvent * recv_event;
        QPointF p1, p2;
        w = barwidth;
        h = barheight;
        for (QSet<Message *>::Iterator msg = drawMessages.begin();
             msg != drawMessages.end(); ++msg) {
            send_event = (*msg)->sender;
            recv_event = (*msg)->receiver;
            y = entityYs[send_event->entity];
            if (options->showAggregateSteps)
                x = floor((send_event->step - startStep) * blockwidth) + 1
                    + extents.x();
            else
                x = floor((send_event->step - startStep) / 2 * blockwidth) + 1
                    + extents.x();
            if (options->showMessages == VisOptions::MSG_TRUE)
            {
                p1 = QPointF(x + w/2.0, y + h/2.0);
                y = entityYs[recv_event->entity];
                if (options->showAggregateSteps)
                    x = floor((recv_event->step - startStep) * blockwidth) + 1
                        + extents.x();
                else
                    x = floor((recv_event->step - startStep) / 2 * blockwidth)
                        + 1 + extents.x();
                p2 = QPointF(x + w/2.0, y + h/2.0);
            }
            else
            {
                p1 = QPointF(x, y + h/2.0);
                y = entityYs[recv_event->entity];
                p2 = QPointF(x + w, y + h/2.0);
            }
            painter->drawLine(p1, p2);
        }

}

// Draw messages between clusters if we have opened to leaves
void Gnome::drawGnomeQtInterMessages(QPainter * painter, int blockwidth,
                                     int startStep, int startx)
{
    if (options->showAggregateSteps)
        startStep -= 1;
    painter->setPen(QPen(Qt::black, 1.5, Qt::SolidLine));
    for (QSet<Message *>::Iterator msg = saved_messages.begin();
         msg != saved_messages.end(); ++msg)
    {
        int x1, y1, x2, y2;
        PartitionCluster * sender_pc = cluster_map->value((*msg)->sender->entity)->get_closed_root();
        PartitionCluster * receiver_pc = cluster_map->value((*msg)->receiver->entity)->get_closed_root();

        x1 = startx + blockwidth * ((*msg)->sender->step - startStep + 0.5);

        // Sender is leaf
        if (sender_pc->children->isEmpty())
            y1 = sender_pc->extents.y() + sender_pc->extents.height() / 2;

        // Sender is lower cluster
        else if (sender_pc->extents.y() > receiver_pc->extents.y())
            y1 = sender_pc->extents.y();

        else
            y1 = sender_pc->extents.y() + sender_pc->extents.height();



        x2 = startx + blockwidth * ((*msg)->receiver->step - startStep + 0.5);

        // Sender is leaf
        if (receiver_pc->children->isEmpty())
            y2 = receiver_pc->extents.y() + receiver_pc->extents.height() / 2;

        // Sender is lower cluster
        else if (receiver_pc->extents.y() > sender_pc->extents.y())
            y2 = receiver_pc->extents.y();

        else
            y2 = receiver_pc->extents.y() + receiver_pc->extents.height();

        painter->drawLine(x1, y1, x2, y2);
    }
}

// Draw hierarchical clustering navigation tree recursively
void Gnome::drawQtTree(QPainter * painter, QRect extents)
{
    int labelwidth = 0;
    if (cluster_root->leaf_open())
    {
        painter->setFont(QFont("Helvetica", 10));
        QFontMetrics font_metrics = painter->fontMetrics();
        QString text = QString::number((*(std::max_element(cluster_root->members->begin(),
                                                           cluster_root->members->end()))));

        // Determine bounding box of FontMetrics
        labelwidth = font_metrics.boundingRect(text).width();
    }
    int depth = cluster_root->max_open_depth();
    int branch_length = 5;
    if (depth == 0)
        branch_length = 0;
    else if (5 * depth < extents.width() - labelwidth)
        branch_length = (extents.width() - labelwidth) / depth;

    int topHeight = getTopHeight(extents);
    int effectiveHeight = extents.height() - topHeight;
    int entitiespan = partition->events->size();
    float blockheight = effectiveHeight / 1.0 / entitiespan;
    if (blockheight >= 1.0)
        blockheight = floor(blockheight);

    int leafx = branch_length * depth + labelwidth;

    drawTreeBranch(painter, QRect(extents.x(),
                                  extents.y() + topHeight,
                                  extents.width(),
                                  extents.height() - topHeight),
                   cluster_root, branch_length, labelwidth, blockheight, leafx);
}



// Draw hierarchical clustering navigation tree recursively
void Gnome::drawTreeBranch(QPainter * painter, QRect current,
                           PartitionCluster * pc,
                           int branch_length, int labelwidth,
                           float blockheight, int leafx)
{
    int pc_size = pc->members->size();
    int my_x = current.x();
    int top_y = current.y();
    int my_y = top_y + pc_size / 2.0 * blockheight;
    int child_x, child_y, used_y = 0;
    painter->setPen(QPen(Qt::black, 2.0, Qt::SolidLine));
    if (pc->open && !pc->children->isEmpty())
    {
        for (QList<PartitionCluster *>::Iterator child = pc->children->begin();
             child != pc->children->end(); ++child)
        {
            // Draw line from wherever we start to correct height -- actually
            // loop through children since info this side? We are in the middle
            // of these extents at current.x() and current.y() + current.h()
            // Though we may want to take the discreteness of the entities
            // into account and figure it out by blockheight
            painter->setPen(QPen(Qt::black, 2.0, Qt::SolidLine));
            int child_size = (*child)->members->size();
            child_y = top_y + child_size / 2.0 * blockheight + used_y;
            painter->drawLine(my_x, my_y, my_x, child_y);

            // Draw forward correct amount of px
            child_x = my_x + branch_length;
            painter->drawLine(my_x, child_y, child_x, child_y);

            QRect node = QRect(my_x - 3, my_y - 3, 6, 6);
            painter->fillRect(node, QBrush(Qt::black));
            drawnNodes[pc] = node;

            drawTreeBranch(painter, QRect(child_x, top_y + used_y,
                                          current.width(), current.height()),
                           *child, branch_length, labelwidth,
                           blockheight, leafx);
            used_y += child_size * blockheight;
        }
    }
    else if (pc->children->isEmpty()) // Draw a leaf with label
    {
        painter->setPen(QPen(Qt::black, 2.0, Qt::SolidLine));
            int entity = pc->members->at(0);
            painter->drawLine(my_x, my_y, leafx - labelwidth, my_y);
            painter->setPen(QPen(Qt::white, 2.0, Qt::SolidLine));
            painter->drawLine(leafx - labelwidth, my_y, leafx, my_y);
            painter->setPen(QPen(Qt::black, 2.0, Qt::SolidLine));
            painter->drawText(leafx - labelwidth, my_y + 3,
                              QString::number(entity));
    }
    else // This is a cluster leaf, no label but we extend the line all the way
    {
        painter->setPen(QPen(Qt::black, 2.0, Qt::SolidLine));
        painter->drawLine(my_x, my_y, leafx, my_y);
    }


}

// This walks through the tree and finds the y position and height at which we
// should draw the cluster in the main vis
void Gnome::drawGnomeQtClusterBranch(QPainter * painter, QRect current,
                                     PartitionCluster * pc,
                                     float blockheight, int blockwidth,
                                     int barheight, int barwidth)
{
    int my_x = current.x();
    int top_y = current.y();
    int used_y = 0;
    painter->setPen(QPen(Qt::black, 2.0, Qt::SolidLine));
    if (pc->open && !pc->children->isEmpty())
    {
        // Draw line to myself
        for (QList<PartitionCluster *>::Iterator child = pc->children->begin();
             child != pc->children->end(); ++child)
        {
            // Draw line from wherever we start to correct height -- actually
            // loop through children since info this side? We are in the middle
            // of these extents at current.x() and current.y() + current.h()
            // Though we may want to take the discreteness of the entities
            // into account and figure it out by blockheight
            int child_size = (*child)->members->size();
            drawGnomeQtClusterBranch(painter, QRect(my_x,
                                                    top_y + used_y,
                                                    current.width(),
                                                    current.height()),
                                     *child, blockheight, blockwidth,
                                     barheight, barwidth);
            used_y += child_size * blockheight;
        }
    }
    else if (pc->children->isEmpty() && pc->members->size() == 1) // A leaf
    {
        int entity = pc->members->at(0);
        painter->setPen(QPen(Qt::black, 2.0, Qt::SolidLine));
        drawGnomeQtClusterLeaf(painter, QRect(current.x(), current.y(),
                                              barwidth, barheight),
                               partition->events->value(entity), blockwidth,
                               partition->min_global_step);
        drawnPCs[pc] = QRect(current.x(), current.y(),
                             current.width(), blockheight);
        pc->extents = drawnPCs[pc];
    }
    else // This is open
    {
        painter->setPen(QPen(Qt::black, 2.0, Qt::SolidLine));
        QRect clusterRect = QRect(current.x(), current.y(), current.width(),
                                  blockheight * pc->members->size());
        if (pc == selected_pc) {
            painter->fillRect(clusterRect, QBrush(QColor(153, 255, 153)));
        } else if (alternation) {
            painter->fillRect(clusterRect, QBrush(QColor(217, 217, 217)));
        } else {
            painter->fillRect(clusterRect, QBrush(QColor(189, 189, 189)));
        }
        alternation = !alternation;
        drawGnomeQtClusterEnd(painter, clusterRect, pc,
                           barwidth, barheight, blockwidth, blockheight,
                           partition->min_global_step);
        drawnPCs[pc] = clusterRect;
        pc->extents = clusterRect;
    }


}

// If a cluster is a leaf with one entity, draw it similarly to StepVis
void Gnome::drawGnomeQtClusterLeaf(QPainter * painter, QRect startxy,
                                   QList<CommEvent *> * elist, int blockwidth,
                                   int startStep)
{
    int y = startxy.y();
    int x, w, h, xa, wa;
    if (options->showAggregateSteps)
        startStep -= 1;
    painter->setPen(QPen(Qt::black, 2.0, Qt::SolidLine));
    for (QList<CommEvent *>::Iterator evt = elist->begin();
         evt != elist->end(); ++evt)
    {
        if (options->showAggregateSteps)
            x = floor(((*evt)->step - startStep) * blockwidth) + 1
                + startxy.x();
        else
            x = floor(((*evt)->step - startStep) / 2 * blockwidth) + 1
                + startxy.x();
        w = startxy.width();
        h = startxy.height();

        // We know it will be complete in this view because we're not doing
        // scrolling or anything here.

        // Draw the event
        if ((*evt)->hasMetric(metric))
            painter->fillRect(QRectF(x, y, w, h),
                              QBrush(options->colormap->color((*evt)->getMetric(metric))));
        else
            painter->fillRect(QRectF(x, y, w, h),
                              QBrush(QColor(180, 180, 180)));

        // Draw border but only if we're doing spacing, otherwise too messy
        if (blockwidth != w)
            painter->drawRect(QRectF(x,y,w,h));

        if (options->showAggregateSteps) {
            xa = floor(((*evt)->step - startStep - 1) * blockwidth) + 1
                 + startxy.x();
            wa = startxy.width();

            if ((*evt)->hasMetric(metric))
                painter->fillRect(QRectF(xa, y, wa, h),
                                  QBrush(options->colormap->color((*evt)->getMetric(metric,
                                                                                    true))));
            else
                painter->fillRect(QRectF(xa, y, wa, h),
                                  QBrush(QColor(180, 180, 180)));
            if (blockwidth != w)
                painter->drawRect(QRectF(xa, y, wa, h));
        }

        // Chnage to commBundle method
        QVector<Message *> * msgs = (*evt)->getMessages();
        if (msgs)
            for (QVector<Message *>::Iterator msg = msgs->begin();
                 msg != msgs->end(); ++msg)
            {
                saved_messages.insert(*msg);
            }

    }
}

// We have a lot of modifiers on our double click.
Gnome::ChangeType Gnome::handleDoubleClick(QMouseEvent * event)
{
    int x = event->x();
    int y = event->y();
    // Find the clicked PartitionCluster
    for (QMap<PartitionCluster *, QRect>::Iterator p = drawnPCs.begin();
         p != drawnPCs.end(); ++p)
    {
        if (p.value().contains(x,y))
        {
            PartitionCluster * pc = p.key();
            if ((Qt::ControlModifier && event->modifiers())
                && (event->button() == Qt::RightButton))
            {
                // Focus entities on centroid of this cluster
                options->topByCentroid = true;
                generateTopEntities(pc);
                return CHANGE_NONE;
            }
            else if (Qt::ControlModifier && event->modifiers())
            {
                // Focus entities on max metric
                options->topByCentroid = false;
                generateTopEntities(pc);
                return CHANGE_NONE;
            }
            else if (event->button() == Qt::RightButton)
            {
                // Select a cluster
                if (selected_pc == pc)
                {
                    selected_pc = NULL;
                }
                else
                {
                    selected_pc = pc;
                }
                return CHANGE_SELECTION;
            }
            else if (!pc->children->isEmpty())
            {
                // Open a cluster if possible
                pc->open = true;
                return CHANGE_CLUSTER;
            }
        }
    }

    return CHANGE_NONE;
}

// Click on tree navigation (close clusters)
void Gnome::handleTreeDoubleClick(QMouseEvent * event)
{
    int x = event->x();
    int y = event->y();

    // Figure out which branch this occurs in, open that branch
    for (QMap<PartitionCluster *, QRect>::Iterator p = drawnNodes.begin();
         p != drawnNodes.end(); ++p)
    {
        if (p.value().contains(x,y))
        {
            PartitionCluster * pc = p.key();
            pc->close();
            return; // Return so we don't look elsewhere.
        }
    }
}


// This is the basic function for drawing a cluster. This should be overriden
// by child classes that want to change this drawing style.
void Gnome::drawGnomeQtClusterEnd(QPainter * painter, QRect clusterRect,
                                  PartitionCluster * pc,
                                  int barwidth, int barheight,
                                  int blockwidth, int blockheight,
                                  int startStep)
{
    bool drawMessages = true;
    // Find height constraints
    if (clusterRect.height() > 2 * clusterMaxHeight)
    {
        blockheight = 2*(clusterMaxHeight - 20);
        barheight = blockheight; // - 3;
    }
    else
    {
        blockheight = clusterRect.height() / 2;
        if (blockheight < 40)
            drawMessages = false;
        else
            blockheight -= 20; // For message drawing
        if (barheight > blockheight - 3)
            barheight = blockheight;
    }

    // Figure out base values
    int base_y = clusterRect.y() + clusterRect.height() / 2 - blockheight / 2;
    int x, ys, yr, yc, w, hs, hr, hc, xa, wa, nsends, nrecvs, ncolls;
    if (options->showAggregateSteps) {
        startStep -= 1;
    }

    // Draw partition cluster events
    painter->setPen(QPen(Qt::black, 1.0, Qt::SolidLine));
    for (QList<ClusterEvent *>::Iterator evt = pc->events->begin();
         evt != pc->events->end(); ++evt)
    {
        if (options->showAggregateSteps)
            x = floor(((*evt)->step - startStep) * blockwidth) + 1
                + clusterRect.x();
        else
            x = floor(((*evt)->step - startStep) / 2 * blockwidth) + 1
                + clusterRect.x();
        w = barwidth;
        nsends = (*evt)->getCount(ClusterEvent::CE_EVENT_COMM, ClusterEvent::CE_COMM_SEND)
                 + (*evt)->getCount(ClusterEvent::CE_EVENT_COMM, ClusterEvent::CE_COMM_ISEND);
        nrecvs = (*evt)->getCount(ClusterEvent::CE_EVENT_COMM, ClusterEvent::CE_COMM_RECV)
                 + (*evt)->getCount(ClusterEvent::CE_EVENT_COMM, ClusterEvent::CE_COMM_WAITALL);
        ncolls = (*evt)->getCount(ClusterEvent::CE_EVENT_COMM, ClusterEvent::CE_COMM_COLL);

        int divisor = pc->members->size();
        if (!options->showInactiveSteps)
            divisor = nsends + nrecvs;

        hs = blockheight * nsends / 1.0 / divisor;
        ys = base_y;
        hr = blockheight * nrecvs / 1.0 / divisor;
        yr = base_y + blockheight - hr;

        hc = blockheight * ncolls / 1.0 / divisor;
        yc = base_y + hs;

        // Draw the event
        if (nsends)
            painter->fillRect(QRectF(x, ys, w, hs),
                              QBrush(options->colormap->color(((*evt)->getMetric(ClusterEvent::CE_EVENT_COMM,
                                                                                ClusterEvent::CE_COMM_SEND,
                                                                                ClusterEvent::CE_THRESH_BOTH)
                                                              + (*evt)->getMetric(ClusterEvent::CE_EVENT_COMM,
                                                                                  ClusterEvent::CE_COMM_ISEND,
                                                                                  ClusterEvent::CE_THRESH_BOTH))
                                                              / nsends)));
        if (nrecvs)
            painter->fillRect(QRectF(x, yr, w, hr),
                              QBrush(options->colormap->color(((*evt)->getMetric(ClusterEvent::CE_EVENT_COMM,
                                                                                 ClusterEvent::CE_COMM_RECV,
                                                                                 ClusterEvent::CE_THRESH_BOTH)
                                                              + (*evt)->getMetric(ClusterEvent::CE_EVENT_COMM,
                                                                                  ClusterEvent::CE_COMM_WAITALL,
                                                                                  ClusterEvent::CE_THRESH_BOTH))
                                                              / nrecvs)));
        if (ncolls)
            painter->fillRect(QRectF(x, yc, w, hc),
                              QBrush(options->colormap->color((*evt)->getMetric(ClusterEvent::CE_EVENT_COMM,
                                                                                ClusterEvent::CE_COMM_COLL,
                                                                                ClusterEvent::CE_THRESH_BOTH)
                                                              / ncolls)));

        // Draw border but only if we're doing spacing, otherwise too messy
        if (blockwidth != w) {
            painter->setPen(QPen(Qt::black, 1.0, Qt::SolidLine));
            painter->drawRect(QRect(x, ys, w, blockheight));
        }

        // Message lines
        if (drawMessages) {
            if (nsends)
            {
                painter->setPen(QPen(Qt::black, nsends * 2.0 / divisor,
                                     Qt::SolidLine));
                painter->drawLine(x + blockwidth / 2, ys, x + barwidth,
                                  ys - 20);
            }
            if (nrecvs)
            {
                painter->setPen(QPen(Qt::black, nrecvs * 2.0 / divisor,
                                     Qt::SolidLine));
                painter->drawLine(x + blockwidth / 2, ys + blockheight, x + 1,
                                  ys + blockheight + 20);
            }

            if (ncolls && hc > 5)
            {
                painter->setPen(QPen(Qt::black, 1.0, Qt::DashLine));
                painter->drawLine(x,yc,x+w,yc+hc);
                painter->drawLine(x,yc+hc,x+w,yc);
            }
            else if (ncolls && hc > 3 && hs > 3)
            {
                painter->setPen(QPen(Qt::black, 1.0, Qt::DashLine));
                painter->drawLine(x,yc,x+w,yc);
            }
        }
        else if (ncolls && nsends && hc > 3 && hs > 3) // Delimit
        {
            painter->setPen(QPen(Qt::black, 1.0, Qt::DashLine));
            painter->drawLine(x,yc,x+w,yc);
        }

        // Aggregate step
        if (options->showAggregateSteps) {
            xa = floor(((*evt)->step - startStep - 1) * blockwidth) + 1
                 + clusterRect.x();
            wa = barwidth;

            if (nsends)
                painter->fillRect(QRectF(xa, ys, wa, hs),
                                  QBrush(options->colormap->color(((*evt)->getMetric(ClusterEvent::CE_EVENT_AGG,
                                                                                    ClusterEvent::CE_COMM_SEND,
                                                                                    ClusterEvent::CE_THRESH_BOTH)
                                                                  + (*evt)->getMetric(ClusterEvent::CE_EVENT_AGG,
                                                                                      ClusterEvent::CE_COMM_ISEND,
                                                                                      ClusterEvent::CE_THRESH_BOTH))
                                                                  / nsends)));
            if (nrecvs)
                painter->fillRect(QRectF(xa, yr, wa, hr),
                                  QBrush(options->colormap->color(((*evt)->getMetric(ClusterEvent::CE_EVENT_AGG,
                                                                                     ClusterEvent::CE_COMM_RECV,
                                                                                     ClusterEvent::CE_THRESH_BOTH)
                                                                  + (*evt)->getMetric(ClusterEvent::CE_EVENT_AGG,
                                                                                      ClusterEvent::CE_COMM_WAITALL,
                                                                                      ClusterEvent::CE_THRESH_BOTH))
                                                                 / nrecvs)));
            if (ncolls)
                painter->fillRect(QRectF(xa, yc, wa, hc),
                                  QBrush(options->colormap->color((*evt)->getMetric(ClusterEvent::CE_EVENT_AGG,
                                                                                    ClusterEvent::CE_COMM_COLL,
                                                                                    ClusterEvent::CE_THRESH_BOTH)
                                                                  / ncolls)));
            if (blockwidth != w)
            {
                painter->setPen(QPen(Qt::black, 1.0, Qt::SolidLine));
                painter->drawRect(QRectF(xa, ys, wa, blockheight));
            }
        }


    }
}


// Divide hover into event and preceding aggregate event in case we want to do
// something different with it
bool Gnome::handleHover(QMouseEvent * event)
{
    return false;
    mousex = event->x();
    mousey = event->y();
    if (options->showAggregateSteps && hover_event
            && drawnEvents[hover_event].contains(mousex, mousey))
    {
        // Need to check if we're changing from aggregate to not or vice versa
        if (!hover_aggregate && mousex <= drawnEvents[hover_event].x()
                                          + stepwidth)
        {
            hover_aggregate = true;
            return true;
        }
        else if (hover_aggregate && mousex >=  drawnEvents[hover_event].x()
                                               + stepwidth)
        {
            hover_aggregate = false;
            return true;
        }
    }
    else if (hover_event == NULL
             || !drawnEvents[hover_event].contains(mousex, mousey))
    {
        // Finding potential new hover
        hover_event = NULL;
        for (QMap<Event *, QRect>::Iterator evt = drawnEvents.begin();
             evt != drawnEvents.end(); ++evt)
        {
            if (evt.value().contains(mousex, mousey))
            {
                hover_aggregate = false;
                if (options->showAggregateSteps && mousex <= evt.value().x()
                                                             + stepwidth)
                    hover_aggregate = true;
                hover_event = evt.key();
            }
        }

        return true;
    }
    return false;
}

// Drawing the hover text, no aggregate yet though
void Gnome::drawHover(QPainter * painter)
{
    if (hover_event == NULL)
        return;

    painter->setFont(QFont("Helvetica", 10));
    QFontMetrics font_metrics = painter->fontMetrics();

    QString text = "";
    if (hover_aggregate)
    {
        return;
        text = "Aggregate for now";
    }
    else
    {
        // Fall through and draw Event
        text = functions->value(hover_event->function)->name;
    }

    // Determine bounding box of FontMetrics
    QRect textRect = font_metrics.boundingRect(text);

    // Draw bounding box
    painter->setPen(QPen(QColor(255, 255, 0, 150), 1.0, Qt::SolidLine));
    painter->drawRect(QRectF(mousex, mousey,
                             textRect.width(), textRect.height()));
    painter->fillRect(QRectF(mousex, mousey,
                             textRect.width(),textRect.height()),
                      QBrush(QColor(255, 255, 144, 150)));

    // Draw text
    painter->setPen(Qt::black);
    painter->drawText(mousex + 2, mousey + textRect.height() - 2, text);
}
