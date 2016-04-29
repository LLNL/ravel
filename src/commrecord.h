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
#ifndef COMMRECORD_H
#define COMMRECORD_H

class Message;

// Holder of OTF Comm Info
class CommRecord
{
public:
    CommRecord(unsigned long _s, unsigned long long int _st,
               unsigned long _r, unsigned long long int _rt,
               unsigned long long _size, unsigned int _tag,
               unsigned int _group,
               unsigned long long int _request = 0);

    unsigned long sender;
    unsigned long long int send_time;
    unsigned long receiver;
    unsigned long long int recv_time;

    unsigned long long size;
    unsigned int tag;
    unsigned int type;
    unsigned int group;
    unsigned long long int send_request;
    unsigned long long int send_complete;
    bool matched;

    Message * message;

    bool operator<(const  CommRecord &);
    bool operator>(const  CommRecord &);
    bool operator<=(const  CommRecord &);
    bool operator>=(const  CommRecord &);
    bool operator==(const  CommRecord &);
};

#endif // COMMRECORD_H
