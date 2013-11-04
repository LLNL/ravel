#ifndef COMMRECORD_H
#define COMMRECORD_H

// Holder of OTF Info

class CommRecord
{
public:
    CommRecord(unsigned int _s, unsigned long long _st,
               unsigned int _r, unsigned long long _rt,
               unsigned int _size, unsigned int _tag);

    unsigned int sender;
    unsigned long long send_time;
    unsigned int receiver;
    unsigned long long recv_time;

    unsigned int size;
    unsigned int tag;
    unsigned int type;
    bool matched;
};

#endif // COMMRECORD_H
