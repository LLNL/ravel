#ifndef VISOPTIONS_H
#define VISOPTIONS_H

#include "colormap.h"

class VisOptions
{
public:
    VisOptions(bool _showAgg = true,
                     bool _metricTraditional = false,
                     QString _metric = "Lateness");
    VisOptions(const VisOptions& copy);
    void setRange(double low, double high);

    enum ColorMapType { SEQUENTIAL, DIVERGING, CATEGORICAL };

    bool showAggregateSteps;
    bool colorTraditionalByMetric;
    bool showMessages;
    bool drawGnomes;
    bool drawTop;
    QString metric;
    ColorMapType maptype;
    ColorMap * divergentmap;
    ColorMap * rampmap;
    ColorMap * catcolormap; // categorical colormap
    ColorMap * colormap;
};

#endif // VISOPTIONS_H
