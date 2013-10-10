#ifndef COLORMAP_H
#define COLORMAP_H

#include <QVector>
#include <QColor>

class ColorMap
{
public:
    ColorMap();
    ~ColorMap();
    void addColor(QColor color, float stop);
    QColor color(double value);
    void setRange(double low, double high);

private:
    QColor average(QColor * low, QColor * high);

    double minValue;
    double maxValue;
    QVector< std::pair<QColor, float> > * colors;
};

#endif // COLORMAP_H
