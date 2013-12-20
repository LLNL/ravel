#include "viswidget.h"
#include <iostream>

VisWidget::VisWidget(QWidget *parent) :
    QGLWidget(QGLFormat(QGL::SampleBuffers), parent),
    container(parent),
    trace(NULL),
    visProcessed(false),
    backgroundColor(Qt::white),
    selectColor(QBrush(Qt::yellow)),
    changeSource(false),
    border(20),
    drawnEvents(QMap<Event *, QRect>()),
    selected_event(NULL),
    hover_event(NULL),
    closed(false)
{
    // GLWidget options
    setMinimumSize(200, 200);
    setAutoFillBackground(false);
    setWindowTitle("");
}

VisWidget::~VisWidget()
{
}

QSize VisWidget::sizeHint() const
{
    return QSize(400, 400);
}

void VisWidget::initializeGL()
{
    glEnable(GL_MULTISAMPLE);
    glDisable(GL_DEPTH);
}

void VisWidget::setSteps(float start, float stop)
{
    Q_UNUSED(start);
    Q_UNUSED(stop);
}

void VisWidget::selectEvent(Event * evt)
{
    Q_UNUSED(evt);
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

    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
}

void VisWidget::processVis()
{

}

void VisWidget::qtPaint(QPainter *painter)
{
    Q_UNUSED(painter);
}

void VisWidget::incompleteBox(QPainter *painter, float x, float y, float w, float h)
{
    bool left = true;
    bool right = true;
    bool top = true;
    bool bottom = true;
    if (x <= 0)
        left = false;
    if (x + w >= rect().width())
        right = false;
    if (y <= 0)
        top = false;
    if (y + h >= rect().height())
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


void VisWidget::drawTimescale(QPainter * painter, unsigned long long start, unsigned long long span, int margin)
{
    // Draw the scale bar
    int lineHeight = rect().height() - (timescaleHeight - 1);
    painter->setPen(QPen(Qt::black, 1, Qt::SolidLine));
    painter->drawLine(margin, lineHeight, rect().width() - margin, lineHeight);

    if (!visProcessed)
        return;

    painter->setFont(QFont("Helvetica", 10));
    QFontMetrics font_metrics = this->fontMetrics();

    // Figure out the units and number of ticks ( may handle units later )
    QLocale systemlocale = QLocale::system();
    QString text = systemlocale.toString(start);
    int textWidth = font_metrics.width(text) * 1.4;
    int max_ticks = floor((rect().width() - 2*margin) / 1.0 / textWidth); // We can't have more than this and fit text
    int y = lineHeight + timescaleTickHeight + font_metrics.xHeight() + 4;

    // We want a round number
    unsigned long long tick_span = span / max_ticks; // Not round
    int power = floor(log10(span / max_ticks)); // How many zeros
    unsigned long long roundfactor = pow(10, power);
    //std::cout << "span " << span << " max_ticks " << max_ticks << " tick_span " << tick_span << std::endl;
    //std::cout << "power " << power << " roundfactor " << roundfactor << std::endl;
    tick_span = (tick_span / roundfactor) * roundfactor; // Now round

    // Now we must find the first number after startTime divisible by
    // the tick_span. We don't want to just find the same roundness
    // because then panning doesn't work.
    //std::cout << "tick_span " << tick_span << "  start " << start << std::endl;
    //std::cout << "start % tick_span " << (start % tick_span) << std::endl;
    //std::cout << "ts - s%ts " << (tick_span - start % tick_span) << std::endl;
    unsigned long long tick = (tick_span - start % tick_span) + start;

    // And now we draw
    while (tick < start + span)
    {
        int x = margin + round((tick - start) / 1.0 / span * (rect().width() - 2*margin));
        painter->drawLine(x, lineHeight, x, lineHeight + timescaleTickHeight);
        text = systemlocale.toString(tick);
        textWidth = font_metrics.width(text) / 3;
        painter->drawText(x - textWidth, y, text);

        tick += tick_span;
    }
}
