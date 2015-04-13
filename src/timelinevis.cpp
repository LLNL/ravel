#include "timelinevis.h"
#include <iostream>
#include <cmath>

#include <QLocale>
#include <QMouseEvent>
#include <QCursor>
#include <QBitmap>

#include "trace.h"
#include "event.h"
#include "function.h"
#include "rpartition.h"

TimelineVis::TimelineVis(QWidget* parent, VisOptions * _options)
    : VisWidget(parent = parent, _options),
      jumped(false),
      mousePressed(false),
      rightPressed(false),
      mousex(0),
      mousey(0),
      pressx(0),
      pressy(0),
      stepwidth(0),
      entityheight(0),
      labelWidth(0),
      labelHeight(0),
      labelDescent(0),
      cursorWidth(0),
      maxStep(0),
      maxEntities(0),
      startPartition(0),
      startStep(0),
      startEntity(0),
      stepSpan(0),
      entitySpan(0),
      lastStartStep(0),
      idleFunction(-1),
      proc_to_order(QMap<int, int>()),
      order_to_proc(QMap<int, int>())
{
    setMouseTracking(true);
    cursorWidth = 16;
    if (cursor().bitmap())
    {
        cursorWidth = cursor().bitmap()->size().width() / 2;
    }
}

TimelineVis::~TimelineVis()
{

}

void TimelineVis::processVis()
{
    proc_to_order = QMap<int, int>();
    order_to_proc = QMap<int, int>();
    for (int i = 0; i < maxEntities; i++) {
        proc_to_order[i] = i;
        order_to_proc[i] = i;
    }

    for (QMap<int, Function *>::Iterator fxn = trace->functions->begin();
         fxn != trace->functions->end(); ++fxn)
    {
        if ((fxn.value())->name == "Idle")
        {
            idleFunction = fxn.key();
            break;
        }
    }

    // Determine needs for task labels
    int max_task = pow(10,ceil(log10(maxEntities)) + 1) - 1;
    QPainter * painter = new QPainter();
    painter->begin(this);
    painter->setPen(Qt::black);
    painter->setFont(QFont("Helvetica", 10));
    QLocale systemlocale = QLocale::system();
    QFontMetrics font_metrics = painter->fontMetrics();
    QString testString = systemlocale.toString(max_task);
    labelWidth = font_metrics.width(testString);
    labelHeight = font_metrics.height();
    labelDescent = font_metrics.descent();
    painter->end();
    delete painter;

    visProcessed = true;
}

// Figure out which event has been selected.
// Relies on drawnEvents which is set by child classes.
void TimelineVis::mouseDoubleClickEvent(QMouseEvent * event)
{
    if (!visProcessed)
        return;

    int x = event->x();
    int y = event->y();
    for (QMap<Event *, QRect>::Iterator evt = drawnEvents.begin();
         evt != drawnEvents.end(); ++evt)
    {
        if (evt.value().contains(x,y)) // We've found the event
        {
            if (evt.key() == selected_event) // We were in this event
            {
                if (options->showAggregateSteps)
                {
                    // we're in the aggregate event
                    if (x < evt.value().x() + evt.value().width() / 2)
                    {
                        if (selected_aggregate)
                        {
                            selected_event = NULL;
                            selected_aggregate = false;
                        }
                        else
                        {
                            selected_aggregate = true;
                        }
                    }
                    else // We're in the normal event
                    {
                        if (selected_aggregate)
                        {
                            selected_aggregate = false;
                        }
                        else
                        {
                            selected_event = NULL;
                        }
                    }
                }
                else
                    selected_event = NULL;
            }
            else // This is a new event to us
            {
                // we're in the aggregate event
                if (options->showAggregateSteps
                    && x < evt.value().x() + evt.value().width() / 2)
                {
                    selected_aggregate = true;
                }
                else
                {
                    selected_aggregate = false;
                }
                selected_event = evt.key();
            }
            break;
        }
    }

    overdraw_selected = false;
    if (Qt::MetaModifier && event->modifiers()
        && selected_event && !selected_aggregate)
    {
        overdraw_selected = true;
    }

    changeSource = true;
    emit eventClicked(selected_event, selected_aggregate, overdraw_selected);
    repaint();
}


void TimelineVis::mousePressEvent(QMouseEvent * event)
{
    mousePressed = true;
    rightPressed = false;
    if (event->button() == Qt::RightButton)
        rightPressed = true;
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
    else if (rightPressed)
    {
        rightPressed = false;
        rightDrag(event);
    }
}

void TimelineVis::leaveEvent(QEvent *event)
{
    Q_UNUSED(event);
    hover_event = NULL;
}

// We can either select a single event exclusive-or select a
// number of tasks in a gnome right now.
void TimelineVis::selectEvent(Event * event, bool aggregate, bool overdraw)
{
    selected_tasks.clear();
    selected_gnome = NULL;
    selected_event = event;
    selected_aggregate = aggregate;
    overdraw_selected = overdraw;
    if (changeSource) {
        changeSource = false;
        return;
    }
    if (!closed)
        repaint();
}

void TimelineVis::selectTasks(QList<int> tasks, Gnome * gnome)
{
    selected_tasks = tasks;
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
    if (!visProcessed || hover_event == NULL)
        return;

    painter->setFont(QFont("Helvetica", 10));
    QFontMetrics font_metrics = painter->fontMetrics();

    QString text = "";
    if (hover_aggregate) // Do not draw
    {
        return;
    }
    else
    {
        // Fall through and draw Event
        if (hover_event->caller)
        {
            text = trace->functions->value(hover_event->caller->function)->name;
            text += " : ";
        }
        text += trace->functions->value(hover_event->function)->name;
        text += ", task: " + QString::number(hover_event->task);
        //text += ", atomic: " + QString::number(hover_event->atomic);
        // + ", " + QString::number(hover_event->step).toStdString().c_str();
        if (hover_event->partition)
        {
            text += ((hover_event->partition->runtime) ? ", runtime" : ", application");
        }
        text += ", atomic: " + QString::number(hover_event->atomic);
        text += '\n' + " " + QString::number(hover_event->enter) + " - " + QString::number(hover_event->exit);
        text += ", added: " + QString::number(hover_event->add_order);
    }

    // Determine bounding box of FontMetrics
    QRect textRect = font_metrics.boundingRect(text);

    // Draw bounding box
    painter->setPen(QPen(QColor(200, 200, 0, 150), 1.0, Qt::SolidLine));
    painter->drawRect(QRectF(mousex + cursorWidth, mousey,
                             textRect.width() + 4, textRect.height() + 4));
    painter->setPen(QPen(QColor(255, 255, 0, 150), 1.0, Qt::SolidLine));
    painter->fillRect(QRectF(mousex + cursorWidth, mousey,
                             textRect.width() + 4, textRect.height() + 4),
                      QBrush(QColor(255, 255, 144, 150)));

    // Draw text
    painter->setPen(Qt::black);
    painter->drawText(mousex + 2 + cursorWidth, mousey + textRect.height() - 2, text);
}

void TimelineVis::drawEntityLabels(QPainter * painter, int effectiveHeight,
                                    float barHeight)
{
    painter->setPen(Qt::black);
    painter->setFont(QFont("Helvetica", 10));
    painter->fillRect(0,0,labelWidth,effectiveHeight, QColor(Qt::white));
    int total_labels = floor(effectiveHeight / labelHeight);
    int y;
    int skip = 1;
    if (total_labels < entitySpan)
    {
        skip = ceil(float(entitySpan) / total_labels);
    }

    int start = std::max(floor(startEntity), 0.0);
    int end = std::min(ceil(startEntity + entitySpan),
                       maxEntities - 1.0);
    for (int i = start; i <= end; i+= skip) // Do this by order
    {
        y = floor((i - startEntity) * barHeight) + 1 + barHeight / 2
            + labelDescent;
        if (y < effectiveHeight)
            painter->drawText(1, y, QString::number(order_to_proc[i]));
    }
}
