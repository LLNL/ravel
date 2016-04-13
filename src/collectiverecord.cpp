#include "collectiverecord.h"
#include "commevent.h"
#include "collectiveevent.h"
#include "viswidget.h"

CollectiveRecord::CollectiveRecord(unsigned long long _matching,
                                   unsigned int _root,
                                   unsigned int _collective,
                                   unsigned int _entitygroup)
    : CommBundle(),
      matchingId(_matching),
      root(_root),
      collective(_collective),
      entitygroup(_entitygroup),
      mark(false),
      events(new QList<CollectiveEvent *>())

{
}


CommEvent * CollectiveRecord::getDesignee()
{
    return events->first();
}


void CollectiveRecord::draw(QPainter * painter, CommDrawInterface *vis)
{
    vis->drawCollective(painter, this);
}

// Return stride set to this collective. Will return zero if cannot set.
int CollectiveRecord::set_basic_strides()
{
    int max_stride;
    for (QList<CollectiveEvent *>::Iterator evt = events->begin();
         evt != events->end(); ++evt)
    {
        for (QSet<CommEvent *>::Iterator parent = (*evt)->stride_parents->begin();
             parent != (*evt)->stride_parents->end(); ++parent)
        {
            if (!((*parent)->stride)) // Equals zero meaning its unset
                return 0;
            else if ((*parent)->stride > max_stride)
                max_stride = (*parent)->stride;
        }
    }

    max_stride++;
    for (QList<CollectiveEvent *>::Iterator evt = events->begin();
         evt != events->end(); ++evt)
    {
        (*evt)->stride = max_stride;
    }
    return max_stride;
}
