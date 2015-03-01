#include "function.h"

Function::Function(QString _n, int _g, int _c)
    : name(_n),
      group(_g),
      comms(_c),
      isMain(false)
{
}
