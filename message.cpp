#include "message.h"

Message::Message(unsigned long long send, unsigned long long recv)
    : sendtime(send), recvtime(recv)
{
}
