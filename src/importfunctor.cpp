#include "importfunctor.h"
#include "charmimporter.h"
#include "ravelutils.h"
#include <QElapsedTimer>

#include "trace.h"
#include "otfconverter.h"
#include "importoptions.h"
#include "otf2importer.h"

ImportFunctor::ImportFunctor(ImportOptions * _options)
    : options(_options),
      trace(NULL)
{
}

void ImportFunctor::doImportCharm(QString dataFileName)
{
    std::cout << "Processing " << dataFileName.toStdString().c_str() << std::endl;
    QElapsedTimer traceTimer;
    qint64 traceElapsed;

    traceTimer.start();

    CharmImporter * importer = new CharmImporter();
    importer->importCharmLog(dataFileName, options);

    Trace* trace = importer->getTrace();
    delete importer;
    trace->fullpath = dataFileName;
    //delete converter;
    connect(trace, SIGNAL(updatePreprocess(int, QString)), this,
            SLOT(updatePreprocess(int, QString)));
    connect(trace, SIGNAL(updateClustering(int)), this,
            SLOT(updateClustering(int)));
    connect(trace, SIGNAL(startClustering()), this, SLOT(switchProgress()));
    trace->preprocess(options);

    traceElapsed = traceTimer.nsecsElapsed();
    RavelUtils::gu_printTime(traceElapsed, "Total trace: ");

    emit(done(trace));
}

void ImportFunctor::doImportOTF2(QString dataFileName)
{
    std::cout << "Processing " << dataFileName.toStdString().c_str() << std::endl;
    QElapsedTimer traceTimer;
    qint64 traceElapsed;

    traceTimer.start();

    OTFConverter * importer = new OTFConverter();
    connect(importer, SIGNAL(finishRead()), this, SLOT(finishInitialRead()));
    connect(importer, SIGNAL(matchingUpdate(int, QString)), this,
            SLOT(updateMatching(int, QString)));
    Trace* trace = importer->importOTF2(dataFileName, options);
    delete importer;

    if (trace)
    {
        connect(trace, SIGNAL(updatePreprocess(int, QString)), this,
                SLOT(updatePreprocess(int, QString)));
        connect(trace, SIGNAL(updateClustering(int)), this,
                SLOT(updateClustering(int)));
        connect(trace, SIGNAL(startClustering()), this, SLOT(switchProgress()));
        if (trace->options.origin == ImportOptions::OF_SAVE_OTF2)
            trace->preprocessFromSaved();
        else
            trace->preprocess(options);
    }

    traceElapsed = traceTimer.nsecsElapsed();
    RavelUtils::gu_printTime(traceElapsed, "Total trace: ");

    emit(done(trace));
}


void ImportFunctor::doImportOTF(QString dataFileName)
{
    #ifdef OTF1LIB
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

    if (trace)
    {
        connect(trace, SIGNAL(updatePreprocess(int, QString)), this,
                SLOT(updatePreprocess(int, QString)));
        connect(trace, SIGNAL(updateClustering(int)), this,
                SLOT(updateClustering(int)));
        connect(trace, SIGNAL(startClustering()), this, SLOT(switchProgress()));
        trace->preprocess(options);
    }

    traceElapsed = traceTimer.nsecsElapsed();
    RavelUtils::gu_printTime(traceElapsed, "Total trace: ");

    emit(done(trace));
    #endif
}

void ImportFunctor::finishInitialRead()
{
    emit(reportProgress(25, "Constructing events..."));
}

void ImportFunctor::updateMatching(int portion, QString msg)
{
    emit(reportProgress(25 + portion, msg));
}

void ImportFunctor::updatePreprocess(int portion, QString msg)
{
    emit(reportProgress(50 + portion / 2.0, msg));
}

void ImportFunctor::updateClustering(int portion)
{
    emit(reportClusterProgress(portion, "Clustering..."));
}

void ImportFunctor::switchProgress()
{
    emit(switching());
}
