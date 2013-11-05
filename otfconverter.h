#ifndef OTFCONVERTER_H
#define OTFCONVERTER_H

#include "otfimporter.h"
#include "rawtrace.h"
#include "trace.h"

class OTFConverter
{
public:
    OTFConverter();
    ~OTFConverter();
    Trace * importOTF(QString filename);

private:
    RawTrace * rawtrace;
    Trace * trace;
};

#endif // OTFCONVERTER_H
