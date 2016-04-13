#include "message.h"
#include "commevent.h"
#include "p2pevent.h"
#include "viswidget.h"

Message::Message(unsigned long long send, unsigned long long recv, int group)
    : CommBundle(), sendtime(send), recvtime(recv),
      sender(NULL), receiver(NULL), entitygroup(group), tag(0)
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

void Message::draw(QPainter * painter, CommDrawInterface *vis)
{
    vis->drawMessage(painter, this);
}
