#include "message.h"

Message::Message(unsigned long long send, unsigned long long recv)
    : sendtime(send), recvtime(recv)
{
}

bool Message::operator<(const Message &Message)
{
    return sendtime < Message.sendtime;
}

bool Message::operator>(const Message &Message)
{
    return sendtime > Message.sendtime;
}

bool Message::operator<=(const Message &Message)
{
    return sendtime <= Message.sendtime;
}

bool Message::operator>=(const Message &Message)
{
    return sendtime >= Message.sendtime;
}

bool Message::operator==(const Message &Message)
{
    return sendtime == Message.sendtime;
}
