#ifndef VISOPTIONS_H
#define VISOPTIONS_H

#include "colormap.h"

class VisOptions
{
public:
    VisOptions(bool _showAgg = true,
                     bool _metricTraditional = false,
                     QString _metric = "lateness");
    VisOptions(const VisOptions& copy);

    bool showAggregateSteps;
    bool colorTraditionalByMetric;
    QString metric;
    ColorMap * colormap;
};

#endif // VISOPTIONS_H
