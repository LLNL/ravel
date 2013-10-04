#include "viswidget.h"

VisWidget::VisWidget(QWidget *parent)
{
    QGLWidget(parent = parent);

    // GLWidget options
    setMinimumSize(200, 200);
    setAutoFillBackground(true);

    // Set painting variables
    backgroundColor = QBrush(QColor(204, 229, 255));
    selectColor = QBrush(Qt::yellow);
    visProcessed = false;
}

void VisWidget::setSteps(int start, int stop)
{

}


void VisWidget::setTrace(Trace * t)
{
    trace = t;
}

void VisWidget::paintEvent(QPaintEvent *event)
{
    QPainter painter;
    painter.begin(this);
    painter.setRenderHint(QPainter::Antialiasing);
    paint(&painter, event, 0);
    painter.end();
}

void VisWidget::processVis()
{

}

void VisWidget::paint(QPainter *painter, QPaintEvent *event, int elapsed)
{

}
