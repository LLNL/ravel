#include "partition.h"

Partition::Partition()
{
    events = new QMap<int, QVector<Event *> *>;
    tindex = -1;

    next = new QMap<int, Partition *>();
    prev = new QMap<int, Partition *>();
    parents = new QSet<Partition *>();
    children = new QSet<Partition *>();
    old_parents = new QSet<Partition *>();
    old_children = new QSet<Partition *>();
}

Partition::~Partition()
{
    for (QMap<int, QVector<Event *> *>::Iterator eitr = events->begin(); eitr != events->end(); ++eitr) {
        /*for (QVector<Event *>::Iterator itr = (*eitr)->begin(); itr != (*eitr)->end(); ++itr) {
            delete *itr;
            *itr = NULL;
        }*/ // Don't necessarily delete events due to merging
        delete *eitr;
        *eitr = NULL;
    }
    delete events;

    delete next;
    delete prev;
    delete parents;
    delete children;
    delete old_parents;
    delete old_children;
}

// Call when we are sure we want to delete events
void Partition::deleteEvents()
{
    for (QMap<int, QVector<Event *> *>::Iterator eitr = events->begin(); eitr != events->end(); ++eitr) {
        for (QVector<Event *>::Iterator itr = (*eitr)->begin(); itr != (*eitr)->end(); ++itr) {
            delete *itr;
            *itr = NULL;
        }
    }
}

void Partition::addEvent(Event * e)
{
    if (events->contains(e->process))
    {
        ((*events)[e->process])->append(e);
    }
    else
    {
        (*events)[e->process] = new QVector<Event *>();
        ((*events)[e->process])->append(e);
    }
}

void Partition::sortEvents(){
    for (QMap<int, QVector<Event *> *>::Iterator eitr = events->begin(); eitr != events->end(); ++eitr) {
        qSort((*eitr)->begin(), (*eitr)->end(), dereferencedLessThan<Event>);
    }
}
