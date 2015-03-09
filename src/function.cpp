#include "function.h"

Function::Function(QString _n, int _g, QString _s, int _c)
    : name(_n),
      shortname(_s),
      group(_g),
      comms(_c),
      isMain(false)
{
}
