#include "eventrecord.h"

EventRecord::EventRecord(unsigned int _p, unsigned long long int _t,
                         unsigned int _v, bool _e)
    : process(_p),
      time(_t),
      value(_v),
      enter(_e),
      children(QList<Event *>())
{
}
