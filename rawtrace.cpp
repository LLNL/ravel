#include "rawtrace.h"

RawTrace::RawTrace(int np) : num_processes(np)
{
    functionGroups = new QMap<int, QString>();
    functions = new QMap<int, Function *>();
    events = new QVector<QVector<EventRecord *> *>(np);
    for (int i = 0; i < np; i++) {
        (*events)[i] = new QVector<EventRecord *>();
    }
    messages = new QVector<QVector<CommRecord *> *>(np);
    for (int i = 0; i < np; i++) {
        (*messages)[i] = new QVector<CommRecord *>();
    }
}

// Note we do not delete the function/functionGroup map because
// we know that will get passed to the processed trace
RawTrace::~RawTrace()
{
    for (QVector<QVector<EventRecord *> *>::Iterator eitr = events->begin(); eitr != events->end(); ++eitr) {
        for (QVector<EventRecord *>::Iterator itr = (*eitr)->begin(); itr != (*eitr)->end(); ++itr) {
            delete *itr;
            *itr = NULL;
        }
        delete *eitr;
        *eitr = NULL;
    }
    delete events;
    for (QVector<QVector<CommRecord *> *>::Iterator eitr = messages->begin(); eitr != messages->end(); ++eitr) {
        for (QVector<CommRecord *>::Iterator itr = (*eitr)->begin(); itr != (*eitr)->end(); ++itr) {
            delete *itr;
            *itr = NULL;
        }
        delete *eitr;
        *eitr = NULL;
    }
    delete messages;
}
