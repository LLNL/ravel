#include "rawtrace.h"

RawTrace::RawTrace(int np)
    : functionGroups(NULL),
      functions(NULL),
      events(NULL),
      messages(NULL),
      communicators(NULL),
      collective_definitions(NULL),
      collectives(NULL),
      collectiveMap(NULL),
      num_processes(np)
{

}

// Note we do not delete the function/functionGroup map because
// we know that will get passed to the processed trace
RawTrace::~RawTrace()
{
    for (QVector<QVector<EventRecord *> *>::Iterator eitr = events->begin();
         eitr != events->end(); ++eitr)
    {
        for (QVector<EventRecord *>::Iterator itr = (*eitr)->begin();
             itr != (*eitr)->end(); ++itr)
        {
            delete *itr;
            *itr = NULL;
        }
        delete *eitr;
        *eitr = NULL;
    }
    delete events;

    for (QVector<QVector<CommRecord *> *>::Iterator eitr = messages->begin();
         eitr != messages->end(); ++eitr)
    {
        for (QVector<CommRecord *>::Iterator itr = (*eitr)->begin();
             itr != (*eitr)->end(); ++itr)
        {
            delete *itr;
            *itr = NULL;
        }
        delete *eitr;
        *eitr = NULL;
    }
    delete messages;
}
