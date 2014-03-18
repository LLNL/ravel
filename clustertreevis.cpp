#include "clustertreevis.h"

ClusterTreeVis::ClusterTreeVis(QWidget *parent, VisOptions * _options)
    : VisWidget(parent, _options),
      gnome(NULL)
{
}

QSize ClusterTreeVis::sizeHint() const
{
    return QSize(50, 50);
}

void ClusterTreeVis::setGnome(Gnome * _gnome)
{
    gnome = _gnome;
    repaint();
}

void ClusterTreeVis::qtPaint(QPainter *painter)
{
    if (gnome)
        gnome->drawQtTree(painter, rect());
}

void ClusterTreeVis::mouseDoubleClickEvent(QMouseEvent * event)
{
    if (gnome)
        gnome->handleTreeDoubleClick(event);
}
