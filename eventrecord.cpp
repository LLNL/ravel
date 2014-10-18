#include "eventrecord.h"

EventRecord::EventRecord(unsigned int _task, unsigned long long int _t,
                         unsigned int _v, bool _e)
    : task(_task),
      time(_t),
      value(_v),
      enter(_e),
      children(QList<Event *>())
{
}
