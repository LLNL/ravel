#include "message.h"
#include "commevent.h"
#include "p2pevent.h"
#include "viswidget.h"

Message::Message(unsigned long long send, unsigned long long recv)
    : CommBundle(), sendtime(send), recvtime(recv)
{
}

bool Message::operator<(const Message &message)
{
    return sendtime < message.sendtime;
}

bool Message::operator>(const Message &message)
{
    return sendtime > message.sendtime;
}

bool Message::operator<=(const Message &message)
{
    return sendtime <= message.sendtime;
}

bool Message::operator>=(const Message &message)
{
    return sendtime >= message.sendtime;
}

bool Message::operator==(const Message &message)
{
    return sendtime == message.sendtime;
}


CommEvent * Message::getDesignee()
{
    return sender;
}

void Message::draw(QPainter * painter, VisWidget * vis)
{
    vis->drawMessage(painter, this);
}
