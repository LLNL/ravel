#ifndef COMMRECORD_H
#define COMMRECORD_H

class Message;

// Holder of OTF Comm Info
class CommRecord
{
public:
    CommRecord(unsigned long _s, unsigned long long int _st,
               unsigned long _r, unsigned long long int _rt,
               unsigned long long _size, unsigned int _tag,
               unsigned int _group,
               unsigned long long int _request = 0);

    unsigned long sender;
    unsigned long long int send_time;
    unsigned long receiver;
    unsigned long long int recv_time;

    unsigned long long size;
    unsigned int tag;
    unsigned int type;
    unsigned int group;
    unsigned long long int send_request;
    unsigned long long int send_complete;
    bool matched;

    Message * message;

    bool operator<(const  CommRecord &);
    bool operator>(const  CommRecord &);
    bool operator<=(const  CommRecord &);
    bool operator>=(const  CommRecord &);
    bool operator==(const  CommRecord &);
};

#endif // COMMRECORD_H
