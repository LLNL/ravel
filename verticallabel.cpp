#include "verticallabel.h"

#include <QPainter>

// Adapted from http://stackoverflow.com/questions/9183050/vertical-qlabel-or-the-equivalent

VerticalLabel::VerticalLabel(QWidget *parent)
    : QLabel(parent)
{

}

VerticalLabel::VerticalLabel(const QString &text, QWidget *parent)
: QLabel(text, parent)
{
}

void VerticalLabel::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setFont(QFont("Helvetica", 9));
    painter.setPen(Qt::black);
    painter.setBrush(Qt::Dense1Pattern);

    QFontMetrics font_metrics = painter.fontMetrics();
    int labeloffset = 9 * (sizeHint().height() - font_metrics.boundingRect(text()).width()) / 10;

    painter.translate( sizeHint().width() - 8, sizeHint().height() - labeloffset);
    painter.rotate(270);

    painter.drawText(0,0, text());
}

QSize VerticalLabel::minimumSizeHint() const
{
    QSize s = QLabel::minimumSizeHint();
    return QSize(s.height(), s.width());
}

QSize VerticalLabel::sizeHint() const
{
    QSize s = QLabel::sizeHint();
    return QSize(s.height(), s.width());
}
