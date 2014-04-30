#include "collectiverecord.h"

CollectiveRecord::CollectiveRecord(unsigned long long _matching,
                                   unsigned int _root,
                                   unsigned int _collective,
                                   unsigned int _communicator)
    : matchingId(_matching),
      root(_root),
      collective(_collective),
      communicator(_communicator),
      mark(false),
      events(new QList<Event *>())/*,
      times(new QMap<int,
            std::pair<unsigned long long int, unsigned long long int> >())*/

{
}
