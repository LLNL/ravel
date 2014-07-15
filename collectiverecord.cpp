#include "collectiverecord.h"
#include "commevent.h"
#include "collectiveevent.h"
#include "viswidget.h"

CollectiveRecord::CollectiveRecord(unsigned long long _matching,
                                   unsigned int _root,
                                   unsigned int _collective,
                                   unsigned int _communicator)
    : CommBundle(),
      matchingId(_matching),
      root(_root),
      collective(_collective),
      communicator(_communicator),
      mark(false),
      events(new QList<CollectiveEvent *>())/*,
      times(new QMap<int,
            std::pair<unsigned long long int, unsigned long long int> >())*/

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
