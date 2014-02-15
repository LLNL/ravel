#ifndef COLLECTIVERECORD_H
#define COLLECTIVERECORD_H

class CollectiveRecord
{
public:
    CollectiveRecord(unsigned long long int _matching, unsigned int _process, unsigned int _root,
                     unsigned long long int _enter, unsigned int _collective, unsigned int _communicator,
                     unsigned int _sent, unsigned int _received);

    unsigned long long int matchingId;
    unsigned int process;
    unsigned int root;
    unsigned long long int enter;
    unsigned long long int leave;
    unsigned int collective;
    unsigned int communicator;
    unsigned int sent;
    unsigned int received;

};

#endif // COLLECTIVERECORD_H
