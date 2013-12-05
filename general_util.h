#ifndef GENERAL_UTIL_H
#define GENERAL_UTIL_H

#include <QString>

template<class T>
bool dereferencedLessThan(T * o1, T * o2) {
    return *o1 < *o2;
}

#endif // GENERAL_UTIL_H
