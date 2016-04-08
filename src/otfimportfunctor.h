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
#ifndef OTFIMPORTFUNCTOR_H
#define OTFIMPORTFUNCTOR_H

#include <QObject>
#include <QString>

class Trace;
class OTFImportOptions;

// Handle signaling for progress bar
class OTFImportFunctor : public QObject
{
    Q_OBJECT
public:
    OTFImportFunctor(OTFImportOptions * _options);
    Trace * getTrace() { return trace; }

public slots:
    void doImportOTF(QString dataFileName);
    void doImportOTF2(QString dataFileName);
    void finishInitialRead();
    void updateMatching(int portion, QString msg);
    void updatePreprocess(int portion, QString msg);
    void updateClustering(int portion);
    void switchProgress();

signals:
    void switching();
    void done(Trace *);
    void reportProgress(int, QString);
    void reportClusterProgress(int, QString);

private:
    OTFImportOptions * options;
    Trace * trace;
};

#endif // OTFIMPORTFUNCTOR_H
