#ifndef VISOPTIONS_H
#define VISOPTIONS_H

#include "colormap.h"

class VisOptions
{
public:
    VisOptions(bool _showAgg = true,
               bool _metricTraditional = true,
               QString _metric = "Lateness");
    VisOptions(const VisOptions& copy);
    void setRange(double low, double high);

    enum ColorMapType { SEQUENTIAL, DIVERGING, CATEGORICAL };
    enum MessageType { NONE, TRUE, SINGLE };

    bool showAggregateSteps;
    bool colorTraditionalByMetric; // color physical timeline by metric
    MessageType showMessages; // draw message lines
    bool topByCentroid; // focus processes are centroid of cluster
    bool showInactiveSteps; // default: no color for inactive proportion
    QString metric;
    ColorMapType maptype;
    ColorMap * divergentmap;
    ColorMap * rampmap; // sequential colormap
    ColorMap * catcolormap; // categorical colormap
    ColorMap * colormap; // active colormap
};

#endif // VISOPTIONS_H
