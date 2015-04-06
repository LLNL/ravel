#ifndef VISOPTIONS_H
#define VISOPTIONS_H

#include <QString>

class ColorMap;

class VisOptions
{
public:
    VisOptions(bool _showAgg = true,
               bool _metricTraditional = true,
               QString _metric = "Lateness");
    VisOptions(const VisOptions& copy);
    void setRange(double low, double high);

    enum ColorMapType { COLOR_SEQUENTIAL, COLOR_DIVERGING, COLOR_CATEGORICAL };
    enum MessageType { MSG_NONE, MSG_TRUE, MSG_SINGLE };

    bool absoluteTime;
    bool showAggregateSteps;
    bool colorTraditionalByMetric; // color physical timeline by metric
    MessageType showMessages; // draw message lines
    bool topByCentroid; // focus processes are centroid of cluster
    bool showInactiveSteps; // default: no color for inactive proportion
    bool traceBack; // trace back idles and such
    QString metric;
    ColorMapType maptype;
    ColorMap * divergentmap;
    ColorMap * rampmap; // sequential colormap
    ColorMap * catcolormap; // categorical colormap
    ColorMap * colormap; // active colormap
};

#endif // VISOPTIONS_H
