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
#include "metricrangedialog.h"
#include "ui_metricrangedialog.h"
#include <QAbstractButton>

MetricRangeDialog::MetricRangeDialog(QWidget *parent, long long int current,
                                     long long int original)
    : QDialog(parent),
    ui(new Ui::MetricRangeDialog),
    start_value(current),
    original_value(original)
{
    ui->setupUi(this);

    ui->metricEdit->setText(QString::number(current));
    connect(ui->buttonBox, SIGNAL(clicked(QAbstractButton*)), this,
            SLOT(onButtonClick(QAbstractButton*)));
    connect(ui->buttonBox, SIGNAL(accepted()), this, SLOT(onOK()));
    connect(ui->buttonBox, SIGNAL(rejected()), this, SLOT(onCancel()));
}

MetricRangeDialog::~MetricRangeDialog()
{
    delete ui;
}

void MetricRangeDialog::onButtonClick(QAbstractButton* qab)
{
    if (ui->buttonBox->buttonRole(qab) == QDialogButtonBox::ResetRole)
    {
        if (ui->metricEdit->text().toLongLong() != original_value)
        {
            start_value = original_value;
            ui->metricEdit->setText(QString::number(original_value));
            emit(valueChanged(original_value));
        }
        this->hide();
    }
}

void MetricRangeDialog::onOK()
{
    if (ui->metricEdit->text().toLongLong() != start_value)
    {
        start_value = ui->metricEdit->text().toLongLong();
        ui->metricEdit->setText(QString::number(start_value));
        emit(valueChanged(start_value));
    }
    this->hide();
}

void MetricRangeDialog::onCancel()
{
    if (ui->metricEdit->text().toLongLong() != start_value)
    {
        ui->metricEdit->setText(QString::number(start_value));
    }
    this->hide();
}

void MetricRangeDialog::setCurrent(long long int current)
{
    start_value = current;
}
