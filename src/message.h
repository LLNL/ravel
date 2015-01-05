#ifndef MESSAGE_H
#define MESSAGE_H

class P2PEvent;
class CommEvent;

#include "commbundle.h"

// Holder of message info
class Message : public CommBundle
{
public:
    Message(unsigned long long send, unsigned long long recv,
            int group);
    P2PEvent * sender;
    P2PEvent * receiver;
    unsigned long long sendtime;
    unsigned long long recvtime;
    int taskgroup;

    CommEvent * getDesignee();

    bool operator<(const Message &);
    bool operator>(const Message &);
    bool operator<=(const Message &);
    bool operator>=(const Message &);
    bool operator==(const Message &);

    void draw(QPainter * painter, CommDrawInterface * vis);
};

#endif // MESSAGE_H
