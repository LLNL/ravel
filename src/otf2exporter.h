//////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2014, Lawrence Livermore National Security, LLC.
// Produced at the Lawrence Livermore National Laboratory.
//
// This file is part of Ravel.
// Written by Kate Isaacs, kisaacs@acm.org, All rights reserved.
// LLNL-CODE-663885
//
// For details, see https://github.com/scalability-llnl/ravel
// Please also see the LICENSE file for our notice and the LGPL.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License (as published by
// the Free Software Foundation) version 2.1 dated February 1999.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the IMPLIED WARRANTY OF
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the terms and
// conditions of the GNU General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program; if not, write to the Free Software Foundation,
// Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
//////////////////////////////////////////////////////////////////////////////
#ifndef OTF2EXPORTER_H
#define OTF2EXPORTER_H

#include <otf2/otf2.h>
#include <QString>
#include <QMap>
#include <QList>

class Trace;
class Entity;

class OTF2Exporter
{
public:
    OTF2Exporter(Trace * _t);
    ~OTF2Exporter();

    void exportTrace(QString path, QString filename);

    static OTF2_FlushType
    pre_flush( void*            userData,
               OTF2_FileType    fileType,
               OTF2_LocationRef location,
               void*            callerData,
               bool             final )
    {
        Q_UNUSED(userData);
        Q_UNUSED(fileType);
        Q_UNUSED(location);
        Q_UNUSED(callerData);
        Q_UNUSED(final);
        return OTF2_FLUSH;
    }

    static OTF2_TimeStamp
    post_flush( void*            userData,
                OTF2_FileType    fileType,
                OTF2_LocationRef location )
    {
        Q_UNUSED(userData);
        Q_UNUSED(fileType);
        Q_UNUSED(location);
        return 0;
    }

    OTF2_FlushCallbacks flush_callbacks;

private:
    Trace * trace;
    QList<Entity *> * entities;
    int ravel_string;
    int ravel_version_string;

    OTF2_Archive * archive;
    OTF2_GlobalDefWriter * global_def_writer;

    void exportDefinitions();
    void exportStrings();
    int addString(QString str, int counter);
    void exportAttributes();
    void exportFunctions();
    void exportEntities();
    void exportEntityGroups();
    void exportEvents();
    void exportEntityEvents(int entityid);

    QMap<QString, int> inverseStringMap;
    QMap<QString, int> * attributeMap;
};

#endif // OTF2EXPORTER_H
