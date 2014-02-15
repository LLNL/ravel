#include "rawtrace.h"

RawTrace::RawTrace(int np)
    : functionGroups(new QMap<int, QString>()),
      functions(new QMap<int, Function *>()),
      events(new QVector<QVector<EventRecord *> *>(np)),
      messages(new QVector<QVector<CommRecord *> *>(np)),
      collectives(new QVector<QVector<CollectiveRecord *> *>(np)),
      num_processes(np)
{

    for (int i = 0; i < np; i++) {
        (*events)[i] = new QVector<EventRecord *>();
    }

    for (int i = 0; i < np; i++) {
        (*messages)[i] = new QVector<CommRecord *>();
    }

    for (int i = 0; i < np; i++) {
        (*collectives)[i] = new QVector<CollectiveRecord *>();
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
