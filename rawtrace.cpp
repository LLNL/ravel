#include "rawtrace.h"

RawTrace::RawTrace()
{
}

// Note we do not delete the function map because
// we know that will get passed to the processed trace
RawTrace::~RawTrace()
{
    delete events;
    delete messages;
}
