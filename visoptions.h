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

    bool showAggregateSteps;
    bool colorTraditionalByMetric;
    bool showMessages;
    QString metric;
    ColorMap * colormap;
};

#endif // VISOPTIONS_H
