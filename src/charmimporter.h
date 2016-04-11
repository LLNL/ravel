#ifndef CHARMIMPORTER_H
#define CHARMIMPORTER_H

#include <QString>
#include <zlib.h>
#include "otfimportoptions.h"
#include "function.h"

class CharmImporter
{
public:
    CharmImporter();
    void importCharmLog(QString filename, OTFImportOptions *_options);

private:
    void readSts();
};

#endif // CHARMIMPORTER_H
