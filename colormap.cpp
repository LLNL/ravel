#include "colormap.h"
#include <iostream>

ColorMap::ColorMap(QColor color, float value)
{
    colors = new QVector<ColorValue *>();
    colors->push_back(new ColorValue(color, value));
    std::cout << "Constructor: " << colors->size() << std::endl;
    minValue = 0;
    maxValue = 1;
}

ColorMap::~ColorMap()
{
    for (QVector<ColorValue *>::Iterator itr = colors->begin(); itr != colors->end(); itr++) {
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
    std::cout << "Add: " << colors->size() << std::endl;
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
    QColor low(0,0,0);
    QColor high(0,0,0);
    double norm_value = (value - minValue) / (maxValue - minValue);
    for (QVector<ColorValue* >::Iterator itr = colors->begin(); itr != colors->end(); itr++) {
        if ((*itr)->value > norm_value) {
            high = (*itr)->color;
            return average(&low, &high);
        } else {
            low = (*itr)->color;
        }
    }
    return low;
}

QColor ColorMap::average(QColor * low, QColor * high)
{
    int r, g, b, a;
    r = 0.5 * (low->red() + high->red());
    g = 0.5 * (low->green() + high->green());
    b = 0.5 * (low->blue() + high->blue());
    a = 0.5 * (low->alpha() + high->alpha());
    return QColor(r, g, b, a);
}
