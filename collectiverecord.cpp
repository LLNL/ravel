#include "collectiverecord.h"
#include "commevent.h"
#include "collectiveevent.h"
#include "viswidget.h"

CollectiveRecord::CollectiveRecord(unsigned long long _matching,
                                   unsigned int _root,
                                   unsigned int _collective,
                                   unsigned int _communicator)
    : CommBundle(),
      matchingId(_matching),
      root(_root),
      collective(_collective),
      communicator(_communicator),
      mark(false),
      events(new QList<CollectiveEvent *>())/*,
      times(new QMap<int,
            std::pair<unsigned long long int, unsigned long long int> >())*/

{
}


CommEvent * CollectiveRecord::getDesignee()
{
    return events->first();
}


void CollectiveRecord::draw(QPainter * painter, VisWidget * vis)
{
    vis->drawCollective(painter, this);
}

void CollectiveRecord::drawArc(QPainter * painter, VisWidget * vis,
                               QPointF * p1, QPointF * p2,
                               int width, bool forward)
{
    QPainterPath path;
    int effectiveHeight = vis->getHeight();
    // handle effectiveheight -- find correct span angle & start so
    // the arc doesn't draw past effective height
    if (p2->y() > effectiveHeight)
    {
        //
    }

    QRectF bounding(p1->x() - width, p1->y(), width*2, p2->y() - p1->y());
    path.moveTo(p1->x(), p1->y());
    if (forward)
        path.arcTo(bounding, 90, -180);
    else
        path.arcTo(bounding, 90, 180);
    painter->drawPath(path);
}
