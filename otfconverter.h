#ifndef OTFCONVERTER_H
#define OTFCONVERTER_H

#include "otfimporter.h"
#include "rawtrace.h"
#include "trace.h"
#include "general_util.h"
#include <QStack>
#include <QSet>
#include <cmath>

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

    RawTrace * rawtrace;
    Trace * trace;
};

#endif // OTFCONVERTER_H
