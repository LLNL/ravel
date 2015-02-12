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
#ifndef GENERAL_UTIL_H
#define GENERAL_UTIL_H

#include <QString>
#include <iostream>

// For qSorting lists of pointers
template<class T>
bool dereferencedLessThan(T * o1, T * o2) {
    return *o1 < *o2;
}

// For units
static QString getUnits(int zeros)
{
    if (zeros >= 18)
        return "as";
    else if (zeros >= 15)
        return "fs";
    else if (zeros >= 12)
        return "ps";
    else if (zeros >= 9)
        return "ns";
    else if (zeros >= 6)
        return "us";
    else if (zeros >= 3)
        return "ms";
    else
        return "s";
}

// For timing information
static void gu_printTime(qint64 nanos)
{
    double seconds = (double)nanos * 1e-9;
    if (seconds < 300)
    {
        std::cout << seconds << " seconds ";
        return;
    }

    double minutes = seconds / 60.0;
    if (minutes < 300)
    {
        std::cout << minutes << " minutes ";
        return;
    }

    double hours = minutes / 60.0;
    std::cout << hours << " hours";
    return;
}

#endif // GENERAL_UTIL_H
