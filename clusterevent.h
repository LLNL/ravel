#ifndef CLUSTEREVENT_H
#define CLUSTEREVENT_H

class ClusterEvent
{
public:
    ClusterEvent(int _step);

    ClusterEvent(const ClusterEvent& copy);
    ClusterEvent(int _step, const ClusterEvent * copy1, const ClusterEvent * copy2);

    enum EventType { COMM, AGG };
    enum CommType { SEND, RECV, WAITALL, BOTH };
    enum Threshhold { LOW, HIGH, ALL };

    void setMetric(int count, long long int value,
                   EventType etype = COMM,
                   CommType ctype = SEND,
                   Threshhold thresh = LOW);
    void addMetric(int count, long long int value,
                   EventType etype = COMM,
                   CommType ctype = SEND,
                   Threshhold thresh = LOW);
    long long int getMetric(EventType etype = COMM,
                            CommType ctype = BOTH,
                            Threshhold thresh = ALL);
    int getCount(EventType etype = COMM,
                 CommType ctype = BOTH,
                 Threshhold thresh = ALL);

    int step;
    int waitallrecvs;
    // On the way up, use metrics as total, then once the root, go back down and average them
    long long int metric[2][3][2]; // [COMM/AGG] [SEND/RECV] [LOW/HIGH]
    int counts[2][3][2];
};

#endif // CLUSTEREVENT_H
