#include "collectiverecord.h"

CollectiveRecord::CollectiveRecord(unsigned long long _matching,
                                   unsigned int _process,
                                   unsigned int _root,
                                   unsigned long long _enter,
                                   unsigned int _collective,
                                   unsigned int _communicator,
                                   unsigned int _sent,
                                   unsigned int _received)
    : matchingId(_matching),
      process(_process),
      root(_root),
      enter(_enter),
      leave(0),
      collective(_collective),
      communicator(_communicator),
      sent(_sent),
      received(_received)
{
}
