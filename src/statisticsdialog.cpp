#include "statisticsdialog.h"
#include "ui_statisticsdialog.h"

StatisticsDialog::StatisticsDialog(QWidget *parent) :
    QDialog(parent),
    values(QList<double>()),
    labels(QList<QString>()),
    bars(QList<QRect>()),
    brush(QBrush( Qt::black, Qt::BDiagPattern )),
    tooltip(true),
    minimumValue(0),
    maximumValue(0),
    showrelative(false),
    margin(10),
    barwidth(0),
    showlabel(true),
    ui(new Ui::StatisticsDialog)
{
    setMouseTracking(true);
    ui->setupUi(this);
}

StatisticsDialog::~StatisticsDialog()
{
    delete ui;
}

void StatisticsDialog::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    paintHistogram(painter);
}

void StatisticsDialog::mouseMoveEvent(QMouseEvent *event)
{
    if (!tooltip)
        return;
    QPoint position = event->pos();
    QList<QRect>::iterator target;
    bool found = false;
    for (QList<QRect>::iterator it = this->bars.begin();
         it != this->bars.end(); it++)
    {
        if (it->contains(position))
        {
            target = it;
            found = true;
            break;
        }
    }
    if (found)
    {
        int i = std::distance(this->bars.begin(), target);
        setToolTip(getLabel(i));
    }
    else if (!toolTip().isEmpty())
        setToolTip("");
}

void StatisticsDialog::resizeEvent(QResizeEvent *event)
{
    calculateValues();
}

void StatisticsDialog::setData(QList<double> values, QList<QString> labels)
{
    this->values = values;
    this->labels = labels;
    this->minimumValue = *(std::min_element(this->values.begin(), this->values.end()));
    if (showrelative)
    {
        for (QList<double>::iterator it = this->values.begin();
             it != this->values.end(); it++)
            *(it) -= this->minimumValue;
    }
    this->maximumValue = *(std::max_element(this->values.begin(), this->values.end()));
    calculateValues();
    setMinimumSize((int)(this->margin * this->values.size() * 2), this->maximumValue + margin * 5);
}

void StatisticsDialog::calculateValues()
{
    this->barwidth = std::max(this->margin, (int)(width() - this->margin * this->values.size())) / this->values.size();
    int h = height() - (this->margin * 4);
    double factor = ((double) h) / (this->maximumValue + 5);
    if (this->minimumValue < 0)
        h -= this->minimumValue;
    if (this->bars.size() != this->values.size())
    {
        int difference = this->values.size() - this->bars.size();
        if (difference > 0)
        {
            QRect rect;
            this->bars.reserve(difference);
            while (difference--)
                this->bars.append(rect);
        }
        else if (difference < 0)
            this->bars.erase(this->bars.end() + difference, this->bars.end());
    }
    int x = this->margin;
    for (size_t i = 0; i < this->values.size(); i++)
    {
        double barheight = this->values[i] * factor;
        this->bars[i].setRect(x, h - barheight + this->margin, this->barwidth, barheight);
        x += this->margin + this->barwidth;
    }
}

void StatisticsDialog::paintHistogram(QPainter &painter)
{
    painter.setBackgroundMode( Qt::OpaqueMode );
    QPen pen(Qt::black);
    pen.setWidth(2);
    painter.setPen(pen);
    painter.setBrush(this->brush);

    int y = height() - this->margin * 2;
    QFontMetrics fontmet(painter.font());
    int x_label = this->margin + this->barwidth/2;
    for (size_t i = 0; i < this->values.size(); i++)
    {
        painter.drawRect(this->bars[i]);
        if (this->showlabel)
        {
            QRect rectangle = QRect(x_label - fontmet.width(getLabel(i))/2, y, this->bars[i].width() + 5, 20);
            painter.drawText(rectangle, getLabel(i));
        }
        int min = showrelative ? this->minimumValue : 0;
        QString label = "(" + QString::number(this->values[i] + min, 'f', 2) + ")";
        painter.drawText(x_label - fontmet.width(label)/2, y - this->bars[i].height() - fontmet.height() - 5, label);
        x_label += this->margin + this->barwidth;
    }
}

QString StatisticsDialog::getLabel(size_t i)
{
    return this->labels[i];
}
