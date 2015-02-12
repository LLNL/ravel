#include "otf2exportfunctor.h"
#include "otf2exporter.h"
#include "general_util.h"

#include <QElapsedTimer>

OTF2ExportFunctor::OTF2ExportFunctor()
{
}

void OTF2ExportFunctor::exportTrace(Trace * trace, const QString& path,
                                    const QString& filename)
{
    std::cout << "Exporting " << filename.toStdString().c_str() << std::endl;
    QElapsedTimer traceTimer;
    qint64 traceElapsed;

    traceTimer.start();

    OTF2Exporter * exporter = new OTF2Exporter(trace);
    exporter->exportTrace(path, filename);

    traceElapsed = traceTimer.nsecsElapsed();
    std::cout << "Total export time: ";
    gu_printTime(traceElapsed);
    std::cout << std::endl;

    emit(done());
}
