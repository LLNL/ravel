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
    void setTrace(Trace * t);

signals:
    void clusterChange();
    
public slots:
    void setSteps(float start, float stop, bool jump);
    void setGnome(Gnome * _gnome);
    void clusterChanged();

protected:
    void qtPaint(QPainter *painter);
    void mouseDoubleClickEvent(QMouseEvent * event);
    void prepaint();
    
private:
    Gnome * gnome;
    bool jumped;
    int mousex;
    int mousey;
    int pressx;
    int pressy;
    float stepwidth;
    float processheight;
    int labelWidth;
    int labelHeight;

    int maxStep;
    int startPartition;
    float startStep;
    float startProcess; // refers to order rather than process really
    float stepSpan;
    float processSpan;
    float lastStartStep;
};

#endif // CLUSTERTREEVIS_H
