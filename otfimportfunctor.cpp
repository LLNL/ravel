#include "otfimportfunctor.h"
#include "general_util.h"
#include <QElapsedTimer>

OTFImportFunctor::OTFImportFunctor(OTFImportOptions * _options)
    : options(_options),
      trace(NULL)
{
}

void OTFImportFunctor::doImport(QString dataFileName)
{
    std::cout << "Processing " << dataFileName.toStdString().c_str() << std::endl;
    QElapsedTimer traceTimer;
    qint64 traceElapsed;

    traceTimer.start();

    OTFConverter * importer = new OTFConverter();
    connect(importer, SIGNAL(finishRead()), this, SLOT(finishInitialRead()));
    connect(importer, SIGNAL(matchingUpdate(int, QString)), this,
            SLOT(updateMatching(int, QString)));
    Trace* trace = importer->importOTF(dataFileName, options);
    delete importer;
    connect(trace, SIGNAL(updatePreprocess(int, QString)), this,
            SLOT(updatePreprocess(int, QString)));
    connect(trace, SIGNAL(updateClustering(int)), this,
            SLOT(updateClustering(int)));
    connect(trace, SIGNAL(startClustering()), this, SLOT(switchProgress()));
    trace->preprocess(options);

    traceElapsed = traceTimer.nsecsElapsed();
    std::cout << "Total trace: ";
    gu_printTime(traceElapsed);
    std::cout << std::endl;
    trace->printStats();

    emit(done(trace));
}

void OTFImportFunctor::finishInitialRead()
{
    emit(reportProgress(25, "Constructing events..."));
}

void OTFImportFunctor::updateMatching(int portion, QString msg)
{
    emit(reportProgress(25 + portion, msg));
}

void OTFImportFunctor::updatePreprocess(int portion, QString msg)
{
    emit(reportProgress(50 + portion / 2.0, msg));
}

void OTFImportFunctor::updateClustering(int portion)
{
    emit(reportClusterProgress(portion, "Clustering..."));
}

void OTFImportFunctor::switchProgress()
{
    emit(switching());
}
