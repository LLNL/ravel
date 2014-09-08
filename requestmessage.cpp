#include "requestmessage.h"

RequestMessage::RequestMessage(unsigned long long send,
                               unsigned long long recv,
                               unsigned long long request,
                               unsigned long long complete)
    : Message(send, recv),
      send_request(request),
      send_complete_time(complete)
{
}
