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
#ifndef IMPORTOPTIONSDIALOG_H
#define IMPORTOPTIONSDIALOG_H

#include <QDialog>
#include "otfimportoptions.h"

// GUI to set import options, must functions are GUI handlers
namespace Ui {
class ImportOptionsDialog;
}

class ImportOptionsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ImportOptionsDialog(QWidget *parent = 0,
                                 OTFImportOptions * _options = NULL);
    ~ImportOptionsDialog();

public slots:
    void onCancel();
    void onOK();
    void onPartitionByFunction(bool value);
    void onPartitionByHeuristic(bool value);
    void onWaitallMerge(bool merge);
    void onCallerMerge(bool merge);
    void onLeapMerge(bool merge);
    void onLeapSkip(bool skip);
    void onGlobalMerge(bool merge);
    void onIsend(bool coalesce);
    void onMessageSize(bool enforce);
    void onAdvancedStep(bool advanced);
    void onFunctionEdit(const QString& text);
    void onCluster(bool cluster);
    void onSeedEdit(const QString& text);


private:
    Ui::ImportOptionsDialog *ui;

    OTFImportOptions * options;
    OTFImportOptions saved;

    void setUIState();
};

#endif // IMPORTOPTIONSDIALOG_H
