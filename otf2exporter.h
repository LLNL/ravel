#ifndef OTF2EXPORTER_H
#define OTF2EXPORTER_H

#include <otf2/otf2.h>
#include <QString>

class Trace;

class OTF2Exporter
{
public:
    OTF2Exporter(Trace * _t);

    void exportTrace(QString path, QString filename);

private:
    Trace * trace;

    OTF2_Archive * archive;
    OTF2_GlobalDefWriter * global_def_writer;

    void exportDefinitions();
    void exportStrings();
    void exportEvents();
};

#endif // OTF2EXPORTER_H
