#ifndef COMMRECORD_H
#define COMMRECORD_H

// Holder of OTF Info

class CommRecord
{
public:
    CommRecord();

    unsigned int sender;
    unsigned long long send_time;
    unsigned int receiver;
    unsigned long long recv_time;

    unsigned int size;
    unsigned int tag;
    unsigned int type;
};

#endif // COMMRECORD_H
