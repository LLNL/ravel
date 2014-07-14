#include "commbundle.h"
#include "viswidget.h"

CommBundle::CommBundle()
{
}

void CommBundle::drawLine(QPainter * painter, VisWidget * vis, QPointF * p1, QPointF * p2)
{
    int effectiveHeight = vis->getHeight();
    if (p1->y() > effectiveHeight && p2->y() > effectiveHeight)
    {
        return;
    }
    else if (p1->y() > effectiveHeight)
    {
        float slope = float(p1->y() - p2->y()) / (p1->x() - p2->x());
        float intercept = p1->y() - slope * p1->x();
        painter->drawLine(QPointF((effectiveHeight - intercept) / slope,
                                  effectiveHeight), *p2);
    }
    else if (p2->y() > effectiveHeight)
    {
        float slope = float(p1->y() - p2->y()) / (p1->x() - p2->x());
        float intercept = p1->y() - slope * p1->x();
        painter->drawLine(*p1, QPointF((effectiveHeight - intercept) / slope,
                                       effectiveHeight));
    }
    else
    {
        painter->drawLine(*p1, *p2);
    }
}
