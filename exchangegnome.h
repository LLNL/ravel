#ifndef EXCHANGEGNOME_H
#define EXCHANGEGNOME_H

#include <QMap>
#include <QList>
#include "gnome.h"
#include "partitioncluster.h"

class ExchangeGnome : public Gnome
{
public:
    ExchangeGnome();
    ~ExchangeGnome();

    bool detectGnome(Partition * part);
    Gnome * create();
    void preprocess();

protected:
    void drawGnomeQtClusterEnd(QPainter * painter, QRect clusterRect, PartitionCluster * pc,
                               int barwidth, int barheight, int blockwidth, int blockheight,
                               int startStep);
    void generateTopProcesses();

private:
    // send-receive, sends-receives, sends-waitall
    enum ExchangeType { SRSR, SSRR, SSWA, UNKNOWN };
    ExchangeType type;
    ExchangeType findType();

    QMap<int, int> SRSRmap;
    QSet<int> SRSRpatterns;
    int maxWAsize;

    void drawGnomeQtClusterSRSR(QPainter * painter, QRect startxy, PartitionCluster * pc, int barwidth, int barheight,
                                               int blockwidth, int blockheight, int startStep);
    void drawGnomeQtClusterSSRR(QPainter * painter, QRect startxy, PartitionCluster * pc,
                                               int barwidth, int barheight, int blockwidth, int blockheight,
                                               int startStep);
    void drawGnomeQtClusterSSWA(QPainter * painter, QRect startxy, PartitionCluster * pc,
                                               int barwidth, int barheight, int blockwidth, int blockheight,
                                               int startStep);

    static const int clusterMaxHeight = 76;
};

#endif // EXCHANGEGNOME_H
