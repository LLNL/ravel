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


void CollectiveRecord::draw(QPainter * painter, VisWidget * vis,
                           VisOptions * options, int w, int h)
{
    int x, y, prev_x, prev_y, root_x, root_y;
    QPointF p1, p2;

    int ell_w, ell_h;
    if (w / 5 > 0)
        ell_w = w / 5;
    else
        ell_w = 3;
    if (h / 5 > 0)
        ell_h = h / 5;
    else
        ell_h = 3;

    int coll_type = (*(vis->getTrace()->collective_definitions))[collective]->type;
    int root_offset = 0;
    bool rooted = false;

    // Rooted
    if (coll_type == 2 || coll_type == 3)
    {
        //root = (*(trace->communicators))[(*cr)->communicator]->processes->at((*cr)->root);
        rooted = true;
        root_offset = ell_w;
        if (coll_type == 2)
        {
            root_offset *= -1;
        }
    }

    painter->setPen(QPen(Qt::darkGray, 1, Qt::SolidLine));
    painter->setBrush(QBrush(Qt::darkGray));
    CollectiveEvent * coll_event = events->at(0);
    prev_y = vis->getY(coll_event);
    prev_x = vis->getX(coll_event);

    if (rooted)
    {
        if (coll_event->process == root)
        {
            painter->setBrush(QBrush());
            prev_x += root_offset;
            root_x = prev_x;
            root_y = prev_y;
        }
        else
            prev_x -= root_offset;
    }
    painter->drawEllipse(prev_x + w/2 - ell_w/2,
                         prev_y + h/2 - ell_h/2,
                         ell_w, ell_h);

    for (int i = 1; i < events->size(); i++)
    {
        painter->setPen(QPen(Qt::darkGray, 1, Qt::SolidLine));
        painter->setBrush(QBrush(Qt::darkGray));
        coll_event = events->at(i);

        if (rooted && coll_event->process == root)
        {
            painter->setBrush(QBrush());
        }

        y = vis->getY(coll_event);
        x = vis->getX(coll_event);
        if (rooted)
        {
            if (coll_event->process == root)
                x += root_offset;
            else
                x -= root_offset;
        }
        painter->drawEllipse(x + w/2 - ell_w/2, y + h/2 - ell_h/2,
                             ell_w, ell_h);

        // Arc style
        painter->setPen(QPen(Qt::black, 1, Qt::DashLine));
        painter->setBrush(QBrush());
        if (coll_type == 1) // BARRIER
        {
            p1 = QPointF(prev_x + w/2.0, prev_y + h/2.0);
            p2 = QPointF(x + w/2.0, y + h/2.0);
            drawArc(painter, vis, &p1, &p2, ell_w * 1.5);
        }
        else if (rooted && coll_event->process == root)
        {
            // ONE2ALL / ALL2ONE drawing handled after the loop
            root_x = x;
            root_y = y;
        }
        else if (coll_type == 4) // ALL2ALL
        {
            // First Arc
            p1 = QPointF(prev_x + w/2.0, prev_y + h/2.0);
            p2 = QPointF(x + w/2.0, y + h/2.0);
            drawArc(painter, vis, &p1, &p2, ell_w * 1.5);
            drawArc(painter, vis, &p1, &p2, ell_w * -1.5, false);
        }

        prev_x = x;
        prev_y = y;
    }

    if (rooted)
    {
        painter->setPen(QPen(Qt::black, 1, Qt::DashLine));
        painter->setBrush(QBrush());
        CollectiveEvent * other;
        int ox, oy;
        p1 = QPointF(root_x + w/2, root_y + h/2);
        for (int j = 0; j < events->size(); j++)
        {
            other = events->at(j);
            if (other->process == root)
                continue;

            oy = vis->getY(other);
            ox = vis->getX(other) - root_offset;

            p2 = QPointF(ox + w/2, oy + h/2);
            drawLine(painter, vis, &p1, &p2);
        }
    }
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
