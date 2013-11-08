#ifndef OTFCONVERTER_H
#define OTFCONVERTER_H

#include "otfimporter.h"
#include "rawtrace.h"
#include "trace.h"
#include "general_util.h"

class OTFConverter
{
public:
    OTFConverter();
    ~OTFConverter();
    Trace * importOTF(QString filename);

private:
    void matchEvents();

    void matchMessages();
    Event * search_child_ranges(QVector<Event *> * children, unsigned long long int time);
    Event * find_comm_event(Event * evt, unsigned long long int time);

    void initializePartitions();
    Partition * mergePartitions(Partition * p1, Partition * p2);

    RawTrace * rawtrace;
    Trace * trace;
    int mpi_group;

    QVector<Partition * > * partitions;
    QVector<QList<Event *> *> * mpi_events;
};

#endif // OTFCONVERTER_H
