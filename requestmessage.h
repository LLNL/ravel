#ifndef REQUESTMESSAGE_H
#define REQUESTMESSAGE_H

#include "message.h"
#include "event.h"

class RequestMessage : public Message
{
public:
    RequestMessage(unsigned long long send, unsigned long long recv,
                   unsigned long long request,
                   unsigned long long complete);

    // For sends... someday, we might do this with recvs
    unsigned long long send_request; // request ID
    unsigned long long send_complete_time; // complete time
    Event * send_handler; // event at complete time (e.g. Waitall)

    // Note the send_handler may not have any receives associated with
    // it -- therefore it is not necessarily a CommEvent
};

#endif // REQUESTMESSAGE_H
