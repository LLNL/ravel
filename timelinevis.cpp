#include "timelinevis.h"
#include <iostream>

TimelineVis::TimelineVis(QWidget* parent, VisOptions * _options)
    : VisWidget(parent = parent, _options),
      mousePressed(false),
      mousex(0),
      mousey(0),
      pressx(0),
      pressy(0),
      stepwidth(0),
      processheight(0),
      maxStep(0),
      startStep(0),
      startProcess(0),
      stepSpan(0),
      processSpan(0),
      proc_to_order(QMap<int, int>()),
      order_to_proc(QMap<int, int>())
{
    setMouseTracking(true);
}

TimelineVis::~TimelineVis()
{

}

void TimelineVis::processVis()
{
    proc_to_order = QMap<int, int>();
    order_to_proc = QMap<int, int>();
    for (int i = 0; i < trace->num_processes; i++) {
        proc_to_order[i] = i;
        order_to_proc[i] = i;
    }
    visProcessed = true;
}

void TimelineVis::mouseDoubleClickEvent(QMouseEvent * event)
{
    if (!visProcessed)
        return;

    int x = event->x();
    int y = event->y();
    for (QMap<Event *, QRect>::Iterator evt = drawnEvents.begin(); evt != drawnEvents.end(); ++evt)
        if (evt.value().contains(x,y))
        {
            if (evt.key() == selected_event)
                selected_event = NULL;
            else
                selected_event = evt.key();
            break;
        }

    changeSource = true;
    emit eventClicked(selected_event);
    repaint();
}

void TimelineVis::mousePressEvent(QMouseEvent * event)
{
    mousePressed = true;
    mousex = event->x();
    mousey = event->y();
    pressx = mousex;
    pressy = mousey;
}

void TimelineVis::mouseReleaseEvent(QMouseEvent * event)
{
    mousePressed = false;

    // Treat single click as double for now
    if (event->x() == pressx && event->y() == pressy)
        mouseDoubleClickEvent(event);
}

void TimelineVis::leaveEvent(QEvent *event)
{
    Q_UNUSED(event);
    hover_event = NULL;
}


void TimelineVis::selectEvent(Event * event)
{
    if (changeSource) {
        changeSource = false;
        return;
    }
    selected_event = event;
    if (!closed)
        repaint();
}

void TimelineVis::drawHover(QPainter * painter)
{
    if (hover_event == NULL)
        return;
    painter->setFont(QFont("Helvetica", 10));
    QFontMetrics font_metrics = this->fontMetrics();
    QString text = ((*(trace->functions))[hover_event->function])->name + ", " + QString::number(hover_event->step);

    // Determine bounding box of FontMetrics
    QRect textRect = font_metrics.boundingRect(text);

    // Draw bounding box
    //std::cout << "Drawing bounding box" << std::endl;
    painter->fillRect(QRectF(mousex, mousey, textRect.width(), textRect.height()), QBrush(QColor(255, 255, 0, 150)));

    // Draw text
    painter->drawText(mousex + 2, mousey + textRect.height() - 2, text);
}
