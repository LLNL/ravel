#include "commrecord.h"
#include "message.h"
#include <QObject>

CommRecord::CommRecord(unsigned long _s, unsigned long long int _st,
                       unsigned long _r, unsigned long long int _rt,
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

