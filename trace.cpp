#include "trace.h"

Trace::Trace(int np) : num_processes(np)
{
    functions = new QMap<int, QString>();
    events = new QVector<QVector<Event *> *>(np);
    for (int i = 0; i < np; i++) {
        (*events)[i] = new QVector<Event *>();
    }
}

Trace::~Trace()
{
    delete functions;
    for (QVector<QVector<Event *> *>::Iterator eitr = events->begin(); eitr != events->end(); ++eitr) {
        for (QVector<Event *>::Iterator itr = (*eitr)->begin(); itr != (*eitr)->end(); ++itr) {
            delete *itr;
            *itr = NULL;
        }
        delete *eitr;
        *eitr = NULL;
    }
    delete events;
}

