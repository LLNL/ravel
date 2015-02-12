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
#include "commrecord.h"
#include "message.h"
#include <QObject>

CommRecord::CommRecord(unsigned int _s, unsigned long long int _st,
                       unsigned int _r, unsigned long long int _rt,
                       unsigned long long _size, unsigned int _tag, unsigned int _group,
                       unsigned long long _request) :
    sender(_s), send_time(_st), receiver(_r), recv_time(_rt),
    size(_size), tag(_tag), group(_group), send_request(_request),
    send_complete(0), matched(false), message(NULL)
{
}


bool  CommRecord::operator<(const  CommRecord & cr)
{
    return send_time <  cr.send_time;
}

bool  CommRecord::operator>(const  CommRecord & cr)
{
    return send_time >  cr.send_time;
}

bool  CommRecord::operator<=(const  CommRecord & cr)
{
    return send_time <=  cr.send_time;
}

bool  CommRecord::operator>=(const  CommRecord & cr)
{
    return send_time >=  cr.send_time;
}

bool  CommRecord::operator==(const  CommRecord & cr)
{
    return send_time ==  cr.send_time;
}

