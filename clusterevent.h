#ifndef CLUSTEREVENT_H
#define CLUSTEREVENT_H

class ClusterEvent
{
public:
    ClusterEvent(int _step);

    ClusterEvent(const ClusterEvent& copy);
    ClusterEvent(int _step, const ClusterEvent * copy1,
                 const ClusterEvent * copy2);

    enum EventType { COMM, AGG };
    enum CommType { SEND, ISEND, RECV, WAITALL, COLL, ALL };
    enum Threshhold { LOW, HIGH, BOTH };

    void setMetric(int count, long long int value,
                   EventType etype = COMM,
                   CommType ctype = SEND,
                   Threshhold thresh = LOW);
    void addMetric(int count, long long int value,
                   EventType etype = COMM,
                   CommType ctype = SEND,
                   Threshhold thresh = LOW);
    long long int getMetric(EventType etype = COMM,
                            CommType ctype = ALL,
                            Threshhold thresh = BOTH);
    int getCount(EventType etype = COMM,
                 CommType ctype = ALL,
                 Threshhold thresh = BOTH);

    int step;
    int waitallrecvs; // How many individual receives there are
    int isends;
    long long int metric[2][5][2]; // [COMM/AGG] [SEND/RECV/WAITALL/COLL] [LOW/HIGH]

     // This counts a waitall as a single event rather than all the receives
    int counts[2][5][2];
};

#endif // CLUSTEREVENT_H
