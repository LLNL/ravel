//////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2014, Lawrence Livermore National Security, LLC.
// Produced at the Lawrence Livermore National Laboratory.
//
// This file is part of Ravel.
// Written by Kate Isaacs, kisaacs@acm.org, All rights reserved.
// LLNL-CODE-663885
//
// For details, see https://scalability-llnl.github.io/ravel
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
#include "verticallabel.h"

#include <QPainter>

// Adapted from
// http://stackoverflow.com/questions/9183050/vertical-qlabel-or-the-equivalent
VerticalLabel::VerticalLabel(QWidget *parent)
    : QLabel(parent)
{

}

VerticalLabel::VerticalLabel(const QString &text, QWidget *parent)
: QLabel(text, parent)
{
}

void VerticalLabel::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setFont(QFont("Helvetica", 9));

    // The offsets here are kind of magical. Perhaps a better understanding of
    // boundingRect is needed so we don't need to do this.
    QFontMetrics font_metrics = painter.fontMetrics();
    int labeloffset = 9 * (sizeHint().height()
                           - font_metrics.boundingRect(text()).width()) / 10;

    painter.translate( sizeHint().width() - 8,
                       sizeHint().height() - labeloffset);
    painter.rotate(270);

    painter.drawText(0,0, text());
}

QSize VerticalLabel::minimumSizeHint() const
{
    QSize s = QLabel::minimumSizeHint();
    return QSize(s.height(), s.width());
}

QSize VerticalLabel::sizeHint() const
{
    QSize s = QLabel::sizeHint();
    return QSize(s.height(), s.width());
}
