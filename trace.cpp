#include "trace.h"

Trace::Trace(int np) : num_processes(np)
{
    functions = new QMap<int, QString>();
    events = new QVector<Event *>();
}

Trace::~Trace()
{
    delete functions;
    for (QVector<Event *>::Iterator itr = events->begin(); itr != events->end(); ++itr) {
        delete *itr;
        *itr = NULL;
    }
    delete events;
}

int Trace::addEvent(Event * e)
{
    events->push_back(e);
    return 0;
}
