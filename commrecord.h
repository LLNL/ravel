#ifndef COMMRECORD_H
#define COMMRECORD_H

#include "message.h"
#include "p2pevent.h"

// Holder of OTF Comm Info
class CommRecord
{
public:
    CommRecord(unsigned int _s, unsigned long long int _st,
               unsigned int _r, unsigned long long int _rt,
               unsigned int _size, unsigned int _tag);

    unsigned int sender;
    unsigned long long int send_time;
    unsigned int receiver;
    unsigned long long int recv_time;

    unsigned int size;
    unsigned int tag;
    unsigned int type;
    bool matched;

    Message * message;

    bool operator<(const  CommRecord &);
    bool operator>(const  CommRecord &);
    bool operator<=(const  CommRecord &);
    bool operator>=(const  CommRecord &);
    bool operator==(const  CommRecord &);
};

#endif // COMMRECORD_H
