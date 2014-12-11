#ifndef CLUSTERTREEVIS_H
#define CLUSTERTREEVIS_H

#include "viswidget.h"

class ClusterTreeVis : public VisWidget
{
    Q_OBJECT
public:
    explicit ClusterTreeVis(QWidget *parent = 0,
                            VisOptions * _options = new VisOptions());
    Gnome * getGnome() { return gnome; }

signals:
    void clusterChange();
    
public slots:
    void setGnome(Gnome * _gnome);
    void clusterChanged();

protected:
    void qtPaint(QPainter *painter);
    void mouseDoubleClickEvent(QMouseEvent * event);
    void mousePressEvent(QMouseEvent * event);
    
private:
    Gnome * gnome;
};

#endif // CLUSTERTREEVIS_H
