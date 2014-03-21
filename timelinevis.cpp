#include "timelinevis.h"
#include <iostream>

TimelineVis::TimelineVis(QWidget* parent, VisOptions * _options)
    : VisWidget(parent = parent, _options),
      jumped(false),
      mousePressed(false),
      mousex(0),
      mousey(0),
      pressx(0),
      pressy(0),
      stepwidth(0),
      processheight(0),
      labelWidth(0),
      labelHeight(0),
      maxStep(0),
      startPartition(0),
      startStep(0),
      startProcess(0),
      stepSpan(0),
      processSpan(0),
      lastStartStep(0),
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

    int max_process = pow(10,ceil(log10(trace->num_processes)) + 1) - 1;
    QPainter * painter = new QPainter();
    painter->begin(this);
    painter->setPen(Qt::black);
    painter->setFont(QFont("Helvetica", 10));
    QLocale systemlocale = QLocale::system();
    QFontMetrics font_metrics = painter->fontMetrics();
    QString testString = systemlocale.toString(max_process);
    labelWidth = font_metrics.width(testString);
    labelHeight = font_metrics.height();
    painter->end();
    delete painter;

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
    selected_processes.clear();
    selected_gnome = NULL;
    selected_event = event;
    if (changeSource) {
        changeSource = false;
        return;
    }
    if (!closed)
        repaint();
}

void TimelineVis::selectProcesses(QList<int> processes, Gnome * gnome)
{
    selected_processes = processes;
    selected_gnome = gnome;
    selected_event = NULL;
    if (changeSource) {
        changeSource = false;
        return;
    }
    if (!closed)
        repaint();
}

void TimelineVis::drawHover(QPainter * painter)
{
    if (hover_event == NULL)
        return;

    painter->setFont(QFont("Helvetica", 10));
    QFontMetrics font_metrics = painter->fontMetrics();

    QString text = "";
    if (hover_aggregate)
    {
        text = "Aggregate for now";
    }
    else
    {
        // Fall through and draw Event
        text = ((*(trace->functions))[hover_event->function])->name;
    }

    // Determine bounding box of FontMetrics
    QRect textRect = font_metrics.boundingRect(text);

    // Draw bounding box
    painter->setPen(QPen(QColor(255, 255, 0, 150), 1.0, Qt::SolidLine));
    painter->drawRect(QRectF(mousex, mousey, textRect.width(), textRect.height()));
    painter->fillRect(QRectF(mousex, mousey, textRect.width(), textRect.height()), QBrush(QColor(255, 255, 144, 150)));

    // Draw text
    painter->setPen(Qt::black);
    painter->drawText(mousex + 2, mousey + textRect.height() - 2, text);
}

void TimelineVis::drawProcessLabels(QPainter * painter, int effectiveHeight, float barHeight)
{
    painter->setPen(Qt::black);
    painter->setFont(QFont("Helvetica", 10));
    painter->fillRect(0,0,labelWidth,effectiveHeight, QColor(Qt::white));
    int total_labels = floor(effectiveHeight / labelHeight);
    int y;
    int skip = 1;
    if (total_labels < processSpan)
    {
        skip = ceil(float(processSpan) / total_labels);
    }

    int start = std::max(floor(startProcess), 0.0);
    int end = std::min(ceil(startProcess + processSpan), trace->num_processes - 1.0);
    for (int i = start; i <= end; i+= skip) // Do this by order
    {
        y = floor((i - startProcess) * barHeight) + 1 + (barHeight + labelHeight) / 2;
        if (y < effectiveHeight)
            painter->drawText(1, y, QString::number(order_to_proc[i]));
    }
}
