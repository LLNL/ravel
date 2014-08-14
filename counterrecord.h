#ifndef COUNTERRECORD_H
#define COUNTERRECORD_H

class CounterRecord
{
public:
    CounterRecord(unsigned int _counter, unsigned long long _time,
                  unsigned long long _value);

    unsigned int counter;
    unsigned long long time;
    unsigned long long value;
};

#endif // COUNTERRECORD_H
