#include "viswidget.h"
#include <iostream>
#include <math.h>
#include <climits>
#include <cmath>

#include <QVector>
#include <QList>
#include <QPaintEvent>
#include <QLocale>

#include "trace.h"
#include "general_util.h"


VisWidget::VisWidget(QWidget *parent, VisOptions * _options) :
    QGLWidget(QGLFormat(QGL::SampleBuffers), parent),
    container(parent),
    trace(NULL),
    visProcessed(false),
    options(_options),
    backgroundColor(Qt::white),
    selectColor(QBrush(Qt::yellow)),
    changeSource(false),
    border(20),
    drawnEvents(QMap<Event *, QRect>()),
    selected_tasks(QList<int>()),
    selected_gnome(NULL),
    selected_event(NULL),
    selected_aggregate(false),
    overdraw_selected(false),
    hover_event(NULL),
    hover_aggregate(NULL),
    closed(false)
{
    // GLWidget options
    setMinimumSize(30, 50);
    setAutoFillBackground(false);
    setWindowTitle("");
}

VisWidget::~VisWidget()
{
}

QSize VisWidget::sizeHint() const
{
    return QSize(30, 50);
}

void VisWidget::initializeGL()
{
    glEnable(GL_MULTISAMPLE);
    glDisable(GL_DEPTH);
}

void VisWidget::setSteps(float start, float stop, bool jump)
{
    Q_UNUSED(start);
    Q_UNUSED(stop);
    Q_UNUSED(jump);
}

void VisWidget::selectEvent(Event * evt, bool aggregate, bool overdraw)
{
    Q_UNUSED(evt);
    Q_UNUSED(aggregate);
    Q_UNUSED(overdraw);
}

void VisWidget::selectTasks(QList<int> tasks, Gnome *gnome)
{
    Q_UNUSED(tasks);
    Q_UNUSED(gnome);
}


void VisWidget::setTrace(Trace * t)
{
    trace = t;
}

void VisWidget::prepaint()
{

}

void VisWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    prepaint();

    // Clear
    qglClearColor(backgroundColor);
    glClear(GL_COLOR_BUFFER_BIT);

    beginNativeGL();
    {
        drawNativeGL();
    }
    endNativeGL();

    QPainter painter(this);
    //painter.begin(this);
    painter.setRenderHint(QPainter::Antialiasing);
    qtPaint(&painter);
    painter.end();
}

void VisWidget::drawNativeGL()
{
}

void VisWidget::beginNativeGL()
{
    makeCurrent();
    glViewport(0, 0, width(), height());
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Switch for 2D drawing
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
}

void VisWidget::endNativeGL()
{
    // Revert settings for painter
    glShadeModel(GL_FLAT);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glDisable(GL_BLEND);


    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
}

void VisWidget::processVis()
{
    visProcessed = true;
}

void VisWidget::qtPaint(QPainter *painter)
{
    Q_UNUSED(painter);
}

// If a described box falls outside the given extents
// We only draw the border where to the edge of the extents.
// We use this when we draw partial boxes and cannot rely on automatic clipping.
void VisWidget::incompleteBox(QPainter *painter, float x, float y, float w, float h, QRect * extents)
{
    bool left = true;
    bool right = true;
    bool top = true;
    bool bottom = true;
    if (x <= extents->x())
        left = false;
    if (x + w >= extents->width())
        right = false;
    if (y <= extents->y())
        top = false;
    if (y + h >= extents->height())
        bottom = false;

    if (left)
        painter->drawLine(QPointF(x, y), QPointF(x, y + h));

    if (right)
        painter->drawLine(QPointF(x + w, y), QPointF(x + w, y + h));

    if (top)
        painter->drawLine(QPointF(x, y), QPointF(x + w, y));

    if (bottom)
        painter->drawLine(QPointF(x, y + h), QPointF(x + w, y + h));
}


// If we want an odd step, we actually need the step after it since that is
// where in the information is stored. This function computes that.
int VisWidget::boundStep(float step) {
    int bstep = ceil(step);
    if (bstep % 2)
        bstep++;
    return bstep;
}

void VisWidget::setClosed(bool _closed)
{
    if (closed && !_closed)
    {
        repaint();
    }
    closed = _closed;
}

void VisWidget::setVisOptions(VisOptions * _options)
{
    options = _options;
}


// Draws a timescale (physical). Note this can fail if the span is short
// enough which can happen when we find no events.
QString VisWidget::drawTimescale(QPainter * painter, unsigned long long start,
                              unsigned long long span, int margin)
{
    // Draw the scale bar
    int lineHeight = rect().height() - (timescaleHeight - 1);
    painter->setPen(QPen(Qt::black, 1, Qt::SolidLine));
    painter->drawLine(margin, lineHeight, rect().width() - margin, lineHeight);

    if (!visProcessed)
        return "";

    painter->setFont(QFont("Helvetica", 10));
    QFontMetrics font_metrics = this->fontMetrics();

    // Figure out the units and number of ticks ( may handle units later )
    QLocale systemlocale = QLocale::system();
    QString text = systemlocale.toString(start);
    int textWidth = font_metrics.width(text) * 1.4;

    // We can't have more than this and fit text
    int max_ticks = floor((rect().width() - 2*margin) / 1.0 / textWidth);

    int y = lineHeight + timescaleTickHeight + font_metrics.xHeight() + 4;

    // We want a round number
    unsigned long long tick_span = span / max_ticks; // Not round
    int power = floor(log10(tick_span)); // How many zeros
    unsigned long long roundfactor = std::max(1.0, pow(10, power));
    tick_span = (tick_span / roundfactor) * roundfactor; // Now round
    if (tick_span < 1)
        tick_span = 1;

    // Now we must find the first number after startTime divisible by
    // the tick_span. We don't want to just find the same roundness
    // because then panning doesn't work.
    unsigned long long tick = (tick_span - start % tick_span) + start;

    // TODO: MAKE THIS PART OPTIONAL
    QString seconds = getUnits(trace->units);
    int tick_divisor = 1;
    unsigned long long tick_base = 0;
    if (!options->absoluteTime)
    {
        tick_base = tick - tick_span;
        int span_unit = (int) floor(log10(tick_span));
        tick_divisor = pow(3 * floor(span_unit / 3), 10);
        if (!tick_divisor)
            tick_divisor = 1;
        seconds = systemlocale.toString(tick_base
                                        / pow((double) trace->units, 10),
                                        'f', trace->units - span_unit)
                                        + "s + "
                                        + getUnits(trace->units
                                                   - 3 * floor(span_unit / 3))
                                        + ":";
    }


    // And now we draw
    while (tick < start + span)
    {
        int x = margin + round((tick - start) / 1.0 / span
                               * (rect().width() - 2*margin));
        painter->drawLine(x, lineHeight, x, lineHeight + timescaleTickHeight);
        text = systemlocale.toString((tick - tick_base) / tick_divisor);
        textWidth = font_metrics.width(text) / 3;
        painter->drawText(x - textWidth, y, text);

        tick += tick_span;
    }
    return seconds;
}
