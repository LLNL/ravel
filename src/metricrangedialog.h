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
#ifndef METRICRANGEDIALOG_H
#define METRICRANGEDIALOG_H

#include <QDialog>

class QAbstractButton;

// Dialog for changing colorbar range
namespace Ui {
class MetricRangeDialog;
}

class MetricRangeDialog : public QDialog
{
    Q_OBJECT
    
public:
    explicit MetricRangeDialog(QWidget *parent = 0, long long int current = 0,
                               long long int original = 0);
    ~MetricRangeDialog();
    void setCurrent(long long int current);
    
signals:
    void valueChanged(long long int);

public slots:
    void onButtonClick(QAbstractButton* qab);
    void onOK();
    void onCancel();

private:
    Ui::MetricRangeDialog *ui;
    long long int start_value;
    long long int original_value;

};

#endif // METRICRANGEDIALOG_H
