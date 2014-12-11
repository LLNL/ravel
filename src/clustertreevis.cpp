#include "clustertreevis.h"
#include "gnome.h"

ClusterTreeVis::ClusterTreeVis(QWidget *parent, VisOptions * _options)
    : VisWidget(parent, _options),
      gnome(NULL)
{
    backgroundColor = palette().color(QPalette::Background);
    setAutoFillBackground(true);
}

// We do everything based on the active gnome which is set here.
// This vis does not keep track of anything else like the others do
void ClusterTreeVis::setGnome(Gnome * _gnome)
{
    gnome = _gnome;
    //repaint();
}

// We'll let the active gnome handle this
void ClusterTreeVis::qtPaint(QPainter *painter)
{
    if (!visProcessed)
        return;

    if (gnome)
    {
        gnome->drawTopLabels(painter, rect());
        gnome->drawQtTree(painter, rect());
    }

}

// Pass to active gnome
void ClusterTreeVis::mouseDoubleClickEvent(QMouseEvent * event)
{
    if (gnome)
    {
        gnome->handleTreeDoubleClick(event);
        changeSource = true;
        emit(clusterChange());
        repaint();
    }
}

// Sometimes this is the same on systems, sometimes not
void ClusterTreeVis::mousePressEvent(QMouseEvent * event)
{
    mouseDoubleClickEvent(event);
}

// Active cluster/gnome to paint
void ClusterTreeVis::clusterChanged()
{
    if (changeSource)
    {
        changeSource = false;
        return;
    }

    repaint();
}
