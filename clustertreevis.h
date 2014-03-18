#ifndef CLUSTERTREEVIS_H
#define CLUSTERTREEVIS_H

#include "viswidget.h"
#include "gnome.h"

class ClusterTreeVis : public VisWidget
{
    Q_OBJECT
public:
    explicit ClusterTreeVis(QWidget *parent = 0, VisOptions * _options = new VisOptions());
    QSize sizeHint() const;

signals:
    
public slots:
    void setGnome(Gnome * _gnome);

protected:
    void qtPaint(QPainter *painter);
    void mouseDoubleClickEvent(QMouseEvent * event);
    
private:
    Gnome * gnome;
};

#endif // CLUSTERTREEVIS_H
