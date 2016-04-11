#include "rawtrace.h"

#include "primarytaskgroup.h"
#include "task.h"
#include "eventrecord.h"
#include "commrecord.h"
#include "taskgroup.h"
#include "otfcollective.h"
#include "collectiverecord.h"
#include "function.h"
#include "counter.h"
#include "counterrecord.h"
#include "otfimportoptions.h"
#include <stdint.h>


RawTrace::RawTrace(int nt, int np)
    : options(new OTFImportOptions()),
      primaries(NULL),
      functionGroups(NULL),
      functions(NULL),
      events(NULL),
      messages(NULL),
      messages_r(NULL),
      taskgroups(NULL),
      collective_definitions(NULL),
      counters(NULL),
      counter_records(NULL),
      collectives(NULL),
      collectiveMap(NULL),
      collectiveBits(NULL),
      num_tasks(nt),
      num_pes(np),
      second_magnitude(1),
      from_saved_version(""),
      metric_names(NULL),
      metric_units(NULL)
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
    delete messages_r;

    for (QVector<QVector<CollectiveBit *> *>::Iterator eitr = collectiveBits->begin();
         eitr != collectiveBits->end(); ++eitr)
    {
        for (QVector<CollectiveBit *>::Iterator itr = (*eitr)->begin();
             itr != (*eitr)->end(); ++itr)
        {
            delete *itr;
            *itr = NULL;
        }
        delete *eitr;
        *eitr = NULL;
    }
    delete collectiveBits;

    delete options;

    if (metric_names)
        delete metric_names;
    if (metric_units)
        delete metric_units;
}
