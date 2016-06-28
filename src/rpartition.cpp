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
#include "rpartition.h"
#include <iostream>
#include <fstream>
#include <climits>

#include "event.h"
#include "commevent.h"
#include "collectiverecord.h"
#include "ravelutils.h"
#include "message.h"
#include "p2pevent.h"
#include "function.h"
#include "metrics.h"

#include "trace.h"

Partition::Partition()
    : events(new QMap<unsigned long, QList<CommEvent *> *>),
      max_time(-1),
      min_time(-1),
      mark(false),
      parents(new QSet<Partition *>()),
      children(new QSet<Partition *>()),
      old_parents(new QSet<Partition *>()),
      old_children(new QSet<Partition *>()),
      new_partition(NULL),
      tindex(-1),
      lowlink(-1),
      metrics(new Metrics()),
      gvid(""),
      debug_mark(false),
      debug_name(-1),
      debug_functions(NULL)
{
}

Partition::~Partition()
{
    for (QMap<unsigned long, QList<CommEvent *> *>::Iterator eitr = events->begin();
         eitr != events->end(); ++eitr)
    {
        // Don't necessarily delete events as they are saved by merging
        delete eitr.value();
    }
    delete events;

    delete parents;
    delete children;
    delete old_parents;
    delete old_children;
    delete metrics;
}

// Call when we are sure we want to delete events held in this partition
// (e.g. destroying the trace)
void Partition::deleteEvents()
{
    for (QMap<unsigned long, QList<CommEvent *> *>::Iterator eitr = events->begin();
         eitr != events->end(); ++eitr)
    {
        for (QList<CommEvent *>::Iterator itr = (eitr.value())->begin();
             itr != (eitr.value())->end(); ++itr)
        {
            delete *itr;
            *itr = NULL;
        }
    }
}

bool Partition::operator<(const Partition &partition)
{
    return min_time < partition.min_time;
}

bool Partition::operator>(const Partition &partition)
{
    return min_time > partition.min_time;
}

bool Partition::operator<=(const Partition &partition)
{
    return min_time <= partition.min_time;
}

bool Partition::operator>=(const Partition &partition)
{
    return min_time >= partition.min_time;
}

bool Partition::operator==(const Partition &partition)
{
    return min_time == partition.min_time;
}


// Does not order, just places them at the end
void Partition::addEvent(CommEvent * e)
{
    if (events->contains(e->entity))
    {
        ((*events)[e->entity])->append(e);
    }
    else
    {
        (*events)[e->entity] = new QList<CommEvent *>();
        ((*events)[e->entity])->append(e);
    }
}

void Partition::sortEvents(){
    for (QMap<unsigned long, QList<CommEvent *> *>::Iterator event_list = events->begin();
         event_list != events->end(); ++event_list)
    {
        qSort((event_list.value())->begin(), (event_list.value())->end(),
              dereferencedLessThan<CommEvent>);
    }
}

// String giving process IDs involved in this partition
QString Partition::generate_process_string()
{
    QString ps = "";
    bool first = true;
    for (QMap<unsigned long, QList<CommEvent *> *>::Iterator itr = events->begin();
         itr != events->end(); ++itr)
    {
        if (!first)
            ps += ", ";
        else
            first = false;
        ps += QString::number(itr.key());
    }
    return ps;
}


int Partition::num_events()
{
    int count = 0;
    for (QMap<unsigned long, QList<CommEvent *> *>::Iterator itr = events->begin();
         itr != events->end(); ++itr)
        count += (*itr)->length();
    return count;
}

// Find what partition this one has been merged into
Partition * Partition::newest_partition()
{
    Partition * p = this;
    while (p != p->new_partition)
        p = p->new_partition;
    return p;
}

QString Partition::get_callers(QMap<int, Function *> * functions)
{
    QSet<QString> callers = QSet<QString>();
    for (QMap<unsigned long, QList<CommEvent *> *>::Iterator evtlist = events->begin();
         evtlist != events->end(); ++evtlist)
    {
        for (QList<CommEvent *>::Iterator evt = evtlist.value()->begin();
             evt != evtlist.value()->end(); ++evt)
        {
            callers.insert(functions->value((*evt)->caller->function)->name);
        }
    }

    QList<QString> caller_list = callers.toList();
    qSort(caller_list);
    QString str = "";
    for (QList<QString>::Iterator caller = caller_list.begin();
         caller != caller_list.end(); ++caller)
    {
        str += "\n ";
        str += *caller;
    }
    return str;
}

// use GraphViz to see partition graph for debugging
void Partition::output_graph(QString filename, Trace * trace)
{
    std::ofstream graph;
    graph.open(filename.toStdString().c_str());
    std::ofstream graph2;
    QString graph2string = filename + ".eventlist";
    graph2.open(graph2string.toStdString().c_str());

    QString indent = "     ";

    graph << "digraph {\n";
    graph << indent.toStdString().c_str() << "graph [bgcolor=transparent];\n";
    graph << indent.toStdString().c_str() << "node [label=\"\\N\"];\n";

    graph2 << "digraph {\n";
    graph2 << indent.toStdString().c_str() << "graph [bgcolor=transparent];\n";
    graph2 << indent.toStdString().c_str() << "node [label=\"\\N\"];\n";

    QMap<int, QString> entities = QMap<int, QString>();

    int id = 0;
    for (QMap<unsigned long, QList<CommEvent *> *>::Iterator evtlist = events->begin();
         evtlist != events->end(); ++evtlist)
    {
        entities.insert(evtlist.key(), QString::number(id));
        graph2 << indent.toStdString().c_str() << entities.value(evtlist.key()).toStdString().c_str();
        graph2 << " [label=\"";
        graph2 << "t=" << QString::number(evtlist.key()).toStdString().c_str();\
        graph2 << "\"];\n";
        id++;
        for (QList<CommEvent *>::Iterator evt = evtlist.value()->begin();
             evt != evtlist.value()->end(); ++evt)
        {
            (*evt)->gvid = QString::number(id);
            graph << indent.toStdString().c_str() << (*evt)->gvid.toStdString().c_str();
            graph << " [label=\"";
            graph << "t=" << QString::number((*evt)->entity).toStdString().c_str();\
            graph << ", p=" << QString::number((*evt)->pe).toStdString().c_str();
            graph << ", e: " << QString::number((*evt)->exit).toStdString().c_str();
            if ((*evt)->caller)
                graph << "\n " << trace->functions->value((*evt)->caller->function)->name.toStdString().c_str();
            graph << "\"];\n";

            graph2 << indent.toStdString().c_str() << (*evt)->gvid.toStdString().c_str();
            graph2 << " [label=\"";
            graph2 << "t=" << QString::number((*evt)->entity).toStdString().c_str();\
            graph2 << ", p=" << QString::number((*evt)->pe).toStdString().c_str();
            graph2 << ", e: " << QString::number((*evt)->exit).toStdString().c_str();
            if ((*evt)->caller)
                graph2 << "\n " << trace->functions->value((*evt)->caller->function)->name.toStdString().c_str();
            graph2 << "\"];\n";
            ++id;
        }
    }


    CommEvent * prev = NULL;
    for (QMap<unsigned long, QList<CommEvent *> *>::Iterator evtlist = events->begin();
         evtlist != events->end(); ++evtlist)
    {
        prev = NULL;
        for (QList<CommEvent *>::Iterator evt = evtlist.value()->begin();
             evt != evtlist.value()->end(); ++evt)
        {
            if (prev)
            {
                graph2 << indent.toStdString().c_str() << prev->gvid.toStdString().c_str();
                graph2 << " -> " << (*evt)->gvid.toStdString().c_str() << ";\n";
            }
            else
            {
                graph2 << indent.toStdString().c_str() << entities.value(evtlist.key()).toStdString().c_str();
                graph2 << " -> " << (*evt)->gvid.toStdString().c_str() << ";\n";
            }
            if ((*evt)->comm_next && (*evt)->comm_next->partition == this)
            {
                graph << "edge [color=red];\n";
                graph << indent.toStdString().c_str() << (*evt)->gvid.toStdString().c_str();
                graph << " -> " << (*evt)->comm_next->gvid.toStdString().c_str() << ";\n";
            }
            if ((*evt)->isP2P() && !(*evt)->isReceive())
            {
                QVector<Message *> * messages = (*evt)->getMessages();
                for (QVector<Message *>::Iterator msg = messages->begin();
                     msg != messages->end(); ++msg)
                {
                    graph << "edge [color=black];\n";
                    graph << indent.toStdString().c_str() << (*evt)->gvid.toStdString().c_str();
                    graph << " -> " << (*msg)->receiver->gvid.toStdString().c_str() << ";\n";
                }
            }
            prev = *evt;
        }
    }

    graph << "}";
    graph.close();

    graph2 << "}";
    graph2.close();
}
