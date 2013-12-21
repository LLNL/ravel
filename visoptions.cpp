#include "visoptions.h"

VisOptions::VisOptions(bool _showAgg,
                 bool _metricTraditional,
                 QString _metric)
    : showAggregateSteps(_showAgg),
      colorTraditionalByMetric(_metricTraditional),
      metric(_metric),
      colormap(new ColorMap(QColor(173, 216, 230), 0))
{
    colormap->addColor(QColor(240, 230, 140), 0.5);
    colormap->addColor(QColor(178, 34, 34), 1);
}

VisOptions::VisOptions(const VisOptions& copy)
{
    showAggregateSteps = copy.showAggregateSteps;
    colorTraditionalByMetric = copy.colorTraditionalByMetric;
    metric = copy.metric;
    colormap = new ColorMap(*(copy.colormap));
}
