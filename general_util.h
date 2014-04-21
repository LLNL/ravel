#ifndef GENERAL_UTIL_H
#define GENERAL_UTIL_H

#include <QString>
#include <iostream>

// For qSorting lists of pointers
template<class T>
bool dereferencedLessThan(T * o1, T * o2) {
    return *o1 < *o2;
}

// For timing information
static void gu_printTime(qint64 nanos)
{
    double seconds = (double)nanos * 1e-9;
    if (seconds < 300)
    {
        std::cout << seconds << " seconds ";
        return;
    }

    double minutes = seconds / 60.0;
    if (minutes < 300)
    {
        std::cout << minutes << " minutes ";
        return;
    }

    double hours = minutes / 60.0;
    std::cout << hours << " hours";
    return;
}

#endif // GENERAL_UTIL_H
