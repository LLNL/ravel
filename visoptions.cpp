#include "visoptions.h"

VisOptions::VisOptions(bool _showAgg,
                 bool _metricTraditional,
                 QString _metric)
    : showAggregateSteps(_showAgg),
      colorTraditionalByMetric(_metricTraditional),
      showMessages(TRUE),
      topByCentroid(false),
      showInactiveSteps(true),
      metric(_metric),
      maptype(DIVERGING),
      divergentmap(new ColorMap(QColor(173, 216, 230), 0)),
      rampmap(new ColorMap(QColor(255, 247, 222), 0)),
      catcolormap(new ColorMap(QColor(158, 218, 229), 0, true)), // divergent blue
      colormap(divergentmap)
{

    // rampmap from colorbrewer
    rampmap->addColor(QColor(252, 141, 89), 0.5);
    rampmap->addColor(QColor(127, 0, 0), 1);

    divergentmap->addColor(QColor(240, 230, 140), 0.5);
    divergentmap->addColor(QColor(178, 34, 34), 1);

    // Cat colors from d3's category20, lulesh example is 0.1 to 0.4
    catcolormap->addColor(QColor(219, 219, 141), 0.05); // light yellow-green
    catcolormap->addColor(QColor(199, 199, 199), 0.1); // light gray
    catcolormap->addColor(QColor(255, 187, 120), 0.15); // peach
    catcolormap->addColor(QColor(152, 223, 138), 0.2); // light green
    catcolormap->addColor(QColor(196, 156, 148), 0.25); // coffee
    catcolormap->addColor(QColor(247, 182, 210), 0.3); // light pink
    catcolormap->addColor(QColor(148, 192, 189), 0.35); // light blue
    catcolormap->addColor(QColor(197, 176, 213), 0.4); // lavender
    catcolormap->addColor(QColor(140, 86, 75), 0.45); // brown
    catcolormap->addColor(QColor(44, 160, 44), 0.5); // quite green
    catcolormap->addColor(QColor(214, 39, 40), 0.55); // red
    catcolormap->addColor(QColor(227, 119, 194), 0.6); // magenta
    catcolormap->addColor(QColor(255, 152, 150), 0.65); // pink
    catcolormap->addColor(QColor(127, 127, 127), 0.7); // gray
    catcolormap->addColor(QColor(174, 119, 232), 0.75); // purple
    catcolormap->addColor(QColor(188, 189, 34), 0.8);  // chartreuse
    catcolormap->addColor(QColor(255, 127, 14), 0.85); // bright orange
    catcolormap->addColor(QColor(23, 190, 207), 0.9); // electric blue
    catcolormap->addColor(QColor(31, 119, 180), 0.95); // royal blue

}

VisOptions::VisOptions(const VisOptions& copy)
{
    showAggregateSteps = copy.showAggregateSteps;
    colorTraditionalByMetric = copy.colorTraditionalByMetric;
    showMessages = copy.showMessages;
    showInactiveSteps = copy.showInactiveSteps;
    topByCentroid = copy.topByCentroid;
    metric = copy.metric;
    maptype = copy.maptype;
    divergentmap = new ColorMap(*(copy.divergentmap));
    catcolormap = new ColorMap(*(copy.catcolormap));
    rampmap = new ColorMap(*(copy.rampmap));
    if (maptype == SEQUENTIAL)
        colormap = rampmap;
    else if (maptype == CATEGORICAL)
        colormap = catcolormap;
    else
        colormap = divergentmap;
}


void VisOptions::setRange(double low, double high)
{
    rampmap->setRange(low, high);
    divergentmap->setRange(low, high);
    catcolormap->setRange(low, high);
}
