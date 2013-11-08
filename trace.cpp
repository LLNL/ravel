#include "trace.h"

Trace::Trace(int np) : num_processes(np)
{
    functionGroups = new QMap<int, QString>();
    functions = new QMap<int, Function *>();
    partitions = new QVector<Partition *>();
    events = new QVector<QVector<Event *> *>(np);
    roots = new QVector<QVector<Event *> *>(np);
    comm_events = new QVector<QVector<Event *> *>(np);
    for (int i = 0; i < np; i++) {
        (*events)[i] = new QVector<Event *>();
        (*roots)[i] = new QVector<Event *>();
        (*comm_events)[i] = new QVector<Event *>();
    }

}

Trace::~Trace()
{
    delete functionsGroups;

    for (QMap<int, Function *>::Iterator itr = functions->begin(); itr != functions->end(); ++itr)
    {
        delete (*itr);
        *itr = NULL;
    }
    delete functions;

    for (QVector<Partition *>::Iterator itr = partitions->begin(); itr != partitions->end(); ++itr) {
        (*itr)->deleteEvents();
        delete *itr;
        *itr = NULL;
    }
    delete partitions;

    for (QVector<QVector<Event *> *>::Iterator eitr = events->begin(); eitr != events->end(); ++eitr) {
        for (QVector<Event *>::Iterator itr = (*eitr)->begin(); itr != (*eitr)->end(); ++itr) {
            delete *itr;
            *itr = NULL;
        }
        delete *eitr;
        *eitr = NULL;
    }
    delete events;

    // Don't need to delete Events, they were deleted above
    for (QVector<QVector<Event *> *>::Iterator eitr = roots->begin(); eitr != roots->end(); ++eitr) {
        delete *eitr;
        *eitr = NULL;
    }
    delete roots;

    for (QVector<QVector<Event *> *>::Iterator eitr = comm_events->begin(); eitr != comm_events->end(); ++eitr) {
        delete *eitr;
        *eitr = NULL;
    }
    delete comm_events;
}

