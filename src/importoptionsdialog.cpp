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
#include "importoptionsdialog.h"
#include "ui_importoptionsdialog.h"
#include "importoptions.h"

ImportOptionsDialog::ImportOptionsDialog(QWidget *parent,
                                         ImportOptions * _options)
    : QDialog(parent),
    ui(new Ui::ImportOptionsDialog),
    options(_options),
    saved(ImportOptions(*_options))
{
    ui->setupUi(this);
    connect(ui->buttonBox, SIGNAL(accepted()), this, SLOT(onOK()));
    connect(ui->buttonBox, SIGNAL(rejected()), this, SLOT(onCancel()));
    connect(ui->functionRadioButton, SIGNAL(clicked(bool)), this,
            SLOT(onPartitionByFunction(bool)));
    connect(ui->heuristicRadioButton, SIGNAL(clicked(bool)), this,
            SLOT(onPartitionByHeuristic(bool)));
    connect(ui->waitallCheckbox, SIGNAL(clicked(bool)), this,
            SLOT(onWaitallMerge(bool)));
    connect(ui->callerCheckbox, SIGNAL(clicked(bool)), this,
            SLOT(onCallerMerge(bool)));
    connect(ui->leapCheckbox, SIGNAL(clicked(bool)), this,
            SLOT(onLeapMerge(bool)));
    connect(ui->skipCheckbox, SIGNAL(clicked(bool)), this,
            SLOT(onLeapSkip(bool)));
    connect(ui->globalMergeBox, SIGNAL(clicked(bool)), this,
            SLOT(onGlobalMerge(bool)));
    connect(ui->functionEdit, SIGNAL(textChanged(QString)), this,
            SLOT(onFunctionEdit(QString)));
    connect(ui->breakEdit, SIGNAL(textChanged(QString)), this,
            SLOT(onBreakEdit(QString)));
    connect(ui->clusterCheckbox, SIGNAL(clicked(bool)), this,
            SLOT(onCluster(bool)));
    connect(ui->isendCheckbox, SIGNAL(clicked(bool)), this,
            SLOT(onIsend(bool)));
    connect(ui->messageSizeCheckbox, SIGNAL(clicked(bool)), this,
            SLOT(onMessageSize(bool)));
    connect(ui->stepCheckbox, SIGNAL(clicked(bool)), this,
            SLOT(onAdvancedStep(bool)));
    connect(ui->recvReorderCheckbox, SIGNAL(clicked(bool)), this,
            SLOT(onRecvReorder(bool)));
    connect(ui->seedEdit, SIGNAL(textChanged(QString)), this,
            SLOT(onSeedEdit(QString)));
    setUIState();
}

ImportOptionsDialog::~ImportOptionsDialog()
{
    delete ui;
}

void ImportOptionsDialog::onOK()
{
    this->close();
}

void ImportOptionsDialog::onCancel()
{
    *options = saved;
    setUIState();
}

void ImportOptionsDialog::onPartitionByFunction(bool value)
{
    options->partitionByFunction = value;
    setUIState();
}

void ImportOptionsDialog::onPartitionByHeuristic(bool value)
{
    options->partitionByFunction = !value;
    setUIState();
}

void ImportOptionsDialog::onWaitallMerge(bool merge)
{
    options->waitallMerge = merge;
}

void ImportOptionsDialog::onCallerMerge(bool merge)
{
    options->callerMerge = merge;
}

void ImportOptionsDialog::onLeapMerge(bool merge)
{
    options->leapMerge = merge;
    setUIState();
}

void ImportOptionsDialog::onLeapSkip(bool skip)
{
    options->leapSkip = skip;
}

void ImportOptionsDialog::onGlobalMerge(bool merge)
{
    options->globalMerge = merge;
}

void ImportOptionsDialog::onCluster(bool cluster)
{
    options->cluster = cluster;
    setUIState();
}

void ImportOptionsDialog::onIsend(bool coalesce)
{
    options->isendCoalescing = coalesce;
}

void ImportOptionsDialog::onMessageSize(bool enforce)
{
    options->enforceMessageSizes = enforce;
}

void ImportOptionsDialog::onAdvancedStep(bool advanced)
{
    options->advancedStepping = advanced;
}


void ImportOptionsDialog::onRecvReorder(bool reorder)
{
    options->reorderReceives = reorder;
    setUIState();
}

void ImportOptionsDialog::onFunctionEdit(const QString& text)
{
    options->partitionFunction = text;
}

void ImportOptionsDialog::onBreakEdit(const QString& text)
{
    options->breakFunctions = text;
}

void ImportOptionsDialog::onSeedEdit(const QString& text)
{
    if (text.length())
    {
        options->clusterSeed = text.toULong();
        options->seedClusters = true;
    }
    else
    {
        options->seedClusters = false;
        options->clusterSeed = 0;
    }
}

// Based on currently operational options, set the UI state to
// something consistent (e.g., in certain modes other options are
// unavailable)
void ImportOptionsDialog::setUIState()
{
    ui->waitallCheckbox->setChecked(options->waitallMerge);
    ui->callerCheckbox->setChecked(options->callerMerge);
    ui->skipCheckbox->setChecked(options->leapSkip);
    ui->leapCheckbox->setChecked(options->leapMerge);
    ui->globalMergeBox->setChecked(options->globalMerge);
    ui->clusterCheckbox->setChecked(options->cluster);
    ui->isendCheckbox->setChecked(options->isendCoalescing);
    ui->messageSizeCheckbox->setChecked(options->enforceMessageSizes);
    ui->stepCheckbox->setChecked(options->advancedStepping);
    ui->recvReorderCheckbox->setChecked(options->reorderReceives);

    ui->functionEdit->setText(options->partitionFunction);
    ui->breakEdit->setText(options->breakFunctions);

    // Make available leap merge options
    if (options->leapMerge)
    {
        ui->skipCheckbox->setEnabled(true);
    }
    else
    {
        ui->skipCheckbox->setEnabled(false);
    }

    if (options->seedClusters)
    {
        ui->seedEdit->setText(QString::number(options->clusterSeed));
    }
    else
    {
        ui->seedEdit->setText("");
    }
    ui->seedEdit->setEnabled(options->cluster);


    ui->recvReorderCheckbox->setEnabled(!options->cluster);
    ui->clusterCheckbox->setEnabled(!options->reorderReceives);

    // Enable or Disable heuristic v. given partition
    if (options->partitionByFunction)
    {
        ui->heuristicRadioButton->setChecked(false);
        ui->functionRadioButton->setChecked(true);
        ui->waitallCheckbox->setEnabled(false);
        ui->callerCheckbox->setEnabled(false);
        ui->leapCheckbox->setEnabled(false);
        ui->skipCheckbox->setEnabled(false);
        ui->functionEdit->setEnabled(true);
        ui->breakEdit->setEnabled(false);
        ui->globalMergeBox->setEnabled(false);
    }
    else
    {
        ui->heuristicRadioButton->setChecked(true);
        ui->functionRadioButton->setChecked(false);
        ui->waitallCheckbox->setEnabled(true);
        ui->callerCheckbox->setEnabled(true);
        ui->leapCheckbox->setEnabled(true);
        ui->functionEdit->setEnabled(false);
        ui->breakEdit->setEnabled(true);
        ui->globalMergeBox->setEnabled(true);
    }
}
