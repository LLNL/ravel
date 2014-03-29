#ifndef COLORMAP_H
#define COLORMAP_H

#include <QVector>
#include <QColor>

class ColorMap
{
public:
    ColorMap(QColor color, float value, bool _categorical = false);
    ~ColorMap();
    ColorMap(const ColorMap& copy);
    void addColor(QColor color, float stop);
    QColor color(double value, double opacity = 1.0);
    void setRange(double low, double high);
    void setClamp(double clamp);
    double getMax() { return maxValue; }

private:
    class ColorValue {
    public:
        ColorValue(QColor c, float v) {
           color = c;
           value = v;
        }

        QColor color;
        float value;
    };

    QColor average(ColorValue * low, ColorValue * high, double norm, double opacity = 1.0);
    QColor categorical_color(double value);

    double minValue;
    double maxValue;
    double maxClamp;
    bool categorical;
    QVector<ColorValue *> * colors;
};

#endif // COLORMAP_H
