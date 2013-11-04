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
    delete events;
    delete messages;
}
