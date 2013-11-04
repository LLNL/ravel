#include "eventrecord.h"

EventRecord::EventRecord(unsigned int _p, unsigned long long _t, unsigned int _v) : process(_p),
    time(_t), value(_v)
{
}
