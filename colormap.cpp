#include "colormap.h"
#include <iostream>

ColorMap::ColorMap(QColor color, float value)
    : minValue(0),
      maxValue(1),
      colors(new QVector<ColorValue *>())
{
    colors->push_back(new ColorValue(color, value));
}

ColorMap::~ColorMap()
{
    for (QVector<ColorValue *>::Iterator itr = colors->begin(); itr != colors->end(); ++itr) {
        delete *itr;
        *itr = NULL;
    }
    delete colors;
}

void ColorMap::setRange(double low, double high)
{
    minValue = low;
    maxValue = high;
}

void ColorMap::addColor(QColor color, float stop)
{
    bool added = false;
    colors->begin();
    for (QVector<ColorValue *>::Iterator itr = colors->begin(); itr != colors->end(); ++itr) {
        if (stop < (*itr)->value) {
            colors->insert(itr, new ColorValue(color, stop));
            added = true;
        }
    }
    if (!added) {
        colors->push_back(new ColorValue(color, stop));
    }
}

QColor ColorMap::color(double value)
{
    ColorValue base1 = ColorValue(QColor(0,0,0), 0);
    ColorValue base2 = ColorValue(QColor(0,0,0), 1);
    ColorValue* low = &base1;
    ColorValue* high = &base2;
    double norm_value = (value - minValue) / (maxValue - minValue);
    for (QVector<ColorValue* >::Iterator itr = colors->begin(); itr != colors->end(); itr++) {
        if ((*itr)->value > norm_value) {
            high = (*itr);
            return average(low, high, norm_value);
        } else {
            low = (*itr);
        }
    }
    return low->color;
}

QColor ColorMap::average(ColorValue * low, ColorValue * high, double norm)
{
    int r, g, b, a;
    float alpha = (norm - low->value) / (high->value - low->value);
    r = (1 - alpha) * low->color.red() + alpha * high->color.red();
    g = (1 - alpha) * low->color.green() + alpha * high->color.green();
    b = (1 - alpha) * low->color.blue() + alpha * high->color.blue();
    a = (1 - alpha) * low->color.alpha() + alpha * high->color.alpha();
    return QColor(r, g, b, a);
}
