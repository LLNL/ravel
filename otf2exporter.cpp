#include "otf2exporter.h"
#include "trace.h"
#include "event.h"
#include <climits>
#include <cmath>

OTF2Exporter::OTF2Exporter(Trace *_t)
    : trace(_t),
      archive(NULL),
      global_def_writer(NULL)
{
}

void OTF2Exporter::exportTrace(QString path, QString filename)
{
    archive = OTF2_Archive_Open(path.toStdString().c_str(),
                                filename.toStdString().c_str(),
                                OTF2_FILEMODE_WRITE,
                                1024 * 1024, 4 * 1024 * 1024,
                                OTF2_SUBSTRATE_POSIX, OTF2_COMPRESSION_NONE);

   // OTF2_Archive_SetSerialCollectiveCallback(archive);


    OTF2_Archive_Close(archive);
}

void OTF2Exporter::exportEvents()
{
    OTF2_Archive_OpenEvtFiles(archive);

    OTF2_Archive_CloseEvtFiles(archive);
}

void OTF2Exporter::exportDefinitions()
{
    global_def_writer = OTF2_Archive_GetGlobalDefWriter(archive);

    // Write time properties
    unsigned long long start = ULLONG_MAX, end = 0;
    for (QVector<QVector<Event *> *>::Iterator evts = trace->events->begin();
         evts != trace->events->end(); ++evts)
    {
        if ((*evts)->first()->enter < start)
            start = (*evts)->first()->enter;
        if ((*evts)->last()->exit > end)
            end = (*evts)->last()->enter;
    }
    OTF2_GlobalDefWriter_WriteClockProperties(global_def_writer,
                                              pow10(trace->units),
                                              start,
                                              end - start + 1);

    // Write Ravel information
    OTF2_GlobalDefWriter_WriteAttribute(global_def_writer,
                                        0, // fix me
                                        0, // Ravel string ref
                                        0, // Ravel string version
                                        OTF2_TYPE_NONE);

}

// The strings we have are:
// Function names
// Task names
// TaskGroup names
// Metric names (counters)
// Personal metric names
void OTF2Exporter::exportStrings()
{

}
