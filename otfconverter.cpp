#include "otfconverter.h"

OTFConverter::OTFConverter()
{
}

OTFConverter::~OTFConverter()
{
    delete rawtrace;
}

Trace * OTFConverter::importOTF(QString filename)
{
    OTFImporter * importer = new OTFImporter();
    rawtrace = importer->importOTF(filename.toStdString().c_str());
    trace = new Trace(rawtrace->num_processes);

    // Convert the events

    // Match comms to events

    // Partition

    // Step

    // Calculate Step metrics

    delete importer;
    return trace;
}
