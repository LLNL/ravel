#include "exchangegnome.h"

ExchangeGnome::ExchangeGnome()
    : Gnome()
{
}

bool ExchangeGnome::detectGnome(Partition * part)
{
    bool gnome = true;
    QSet<int> sends = QSet<int>();
    QSet<int> recvs = QSet<int>();
    for (QMap<int, QList<Event *> *>::Iterator event_list = part->events->begin();
         event_list != part->events->end(); ++event_list)
    {
        sends.clear();
        recvs.clear();
        for (QList<Event *>::Iterator evt = (event_list.value())->begin();
             evt != (event_list.value())->end(); ++evt)
        {
            for (QVector<Message *>::Iterator msg = (*evt)->messages->begin();
                 msg != (*evt)->messages->end(); ++msg)
            {
                if ((*msg)->sender == (*evt))
                {
                    recvs.insert((*msg)->receiver->process);
                }
                else
                {
                    sends.insert((*msg)->sender->process);
                }
            }
        }
        if (sends != recvs)
            gnome = false;
    }
    return gnome;
}
