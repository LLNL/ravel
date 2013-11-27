#ifndef OTFCONVERTER_H
#define OTFCONVERTER_H

#include "otfimporter.h"
#include "otfimportoptions.h"
#include "rawtrace.h"
#include "trace.h"
#include <QStack>
#include <QSet>
#include <cmath>

class OTFConverter
{
public:
    OTFConverter();
    ~OTFConverter();
    Trace * importOTF(QString filename, OTFImportOptions * _options);

private:
    void matchEvents();

    void matchMessages();
    Event * search_child_ranges(QVector<Event *> * children, unsigned long long int time);
    Event * find_comm_event(Event * evt, unsigned long long int time);

    RawTrace * rawtrace;
    Trace * trace;
    OTFImportOptions * options;
    int phaseFunction;

};

#endif // OTFCONVERTER_H
