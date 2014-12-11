#include "counterrecord.h"

CounterRecord::CounterRecord(unsigned int _counter, unsigned long long _time, unsigned long long _value)
    : counter(_counter),
      time(_time),
      value(_value)
{
}
