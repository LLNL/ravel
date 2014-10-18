#ifndef EXCHANGEGNOME_H
#define EXCHANGEGNOME_H

#include <QMap>
#include <QList>
#include "gnome.h"
#include "partitioncluster.h"

// Gnome with detector & special drawing functions for exchange patterns
class ExchangeGnome : public Gnome
{
public:
    ExchangeGnome();

    bool detectGnome(Partition * part);
    Gnome * create();
    void preprocess();

protected:
    void drawGnomeQtClusterEnd(QPainter * painter, QRect clusterRect,
                               PartitionCluster * pc,
                               int barwidth, int barheight, int blockwidth,
                               int blockheight, int startStep);
    void generateTopTasks();

private:
    // send-receive, sends-receives, sends-waitall
    enum ExchangeType { EXCH_SRSR, EXCH_SSRR, EXCH_SSWA, EXCH_UNKNOWN };
    ExchangeType type;
    ExchangeType findType();

    // Trying to grow SRSR patterns to capture large number of them
    // can be used for automatically determining neighborhood size
    QMap<int, int> SRSRmap;
    QSet<int> SRSRpatterns;

    int maxWAsize; // for drawing Waitall pies

    void drawGnomeQtClusterSRSR(QPainter * painter, QRect startxy,
                                PartitionCluster * pc, int barwidth,
                                int barheight, int blockwidth, int blockheight,
                                int startStep);
    void drawGnomeQtClusterSSWA(QPainter * painter, QRect startxy,
                                PartitionCluster * pc, int barwidth,
                                int barheight, int blockwidth, int blockheight,
                                int startStep);


};

#endif // EXCHANGEGNOME_H
