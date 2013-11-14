#ifndef MESSAGE_H
#define MESSAGE_H

class Event;

class Message
{
public:
    Message(unsigned long long send, unsigned long long recv);
    Event * sender;
    Event * receiver;
    unsigned long long sendtime;
    unsigned long long recvtime;

    bool operator<(const Message &);
    bool operator>(const Message &);
    bool operator<=(const Message &);
    bool operator>=(const Message &);
    bool operator==(const Message &);
};

#endif // MESSAGE_H
