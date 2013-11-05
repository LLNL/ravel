#include "rawtrace.h"

RawTrace::RawTrace()
{
    functions = new QMap<int, QString>();
    events = new QVector<EventRecord *>();
    messages = new QVector<CommRecord *>();
}

// Note we do not delete the function map because
// we know that will get passed to the processed trace
RawTrace::~RawTrace()
{
    for (QVector<EventRecord *>::Iterator itr = events->begin(); itr != events->end(); ++itr) {
        delete *itr;
        *itr = NULL;
    }
    delete events;
    for (QVector<CommRecord *>::Iterator itr = messages->begin(); itr != messages->end(); ++itr) {
        delete *itr;
        *itr = NULL;
    }
    delete messages;
}
