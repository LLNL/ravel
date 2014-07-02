#ifndef MESSAGE_H
#define MESSAGE_H

class P2PEvent;

// Holder of message info
class Message
{
public:
    Message(unsigned long long send, unsigned long long recv);
    P2PEvent * sender;
    P2PEvent * receiver;
    unsigned long long sendtime;
    unsigned long long recvtime;

    bool operator<(const Message &);
    bool operator>(const Message &);
    bool operator<=(const Message &);
    bool operator>=(const Message &);
    bool operator==(const Message &);
};

#endif // MESSAGE_H
