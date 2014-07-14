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

void Message::draw(QPainter * painter, VisWidget * vis, VisOptions *options,
                   int w, int h)
{
    if (vis->getHeight() / h <= 32)
        painter->setPen(QPen(Qt::black, 2, Qt::SolidLine));
    else
        painter->setPen(QPen(Qt::black, 1, Qt::SolidLine));

    QPointF p1, p2;
    int y = vis->getY(sender);
    int x = vis->getX(sender);
    if (options->showMessages == VisOptions::TRUE)
    {
        p1 = QPointF(x + w/2.0, y + h/2.0);
        y = vis->getY(receiver);
        x = vis->getX(receiver);
        p2 = QPointF(x + w/2.0, y + h/2.0);
    }
    else
    {
        p1 = QPointF(x, y + h/2.0);
        y = vis->getY(receiver);
        p2 = QPointF(x + w, y + h/2.0);
    }
    drawLine(painter, vis, &p1, &p2);
}
