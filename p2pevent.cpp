#include "p2pevent.h"

P2PEvent::P2PEvent(unsigned long long _enter, unsigned long long _exit,
                   int _function, int _process, int _phase,
                   QVector<Message *> *_messages)
    : CommEvent(_enter, _exit, _function, _process, _phase),
      subevents(NULL),
      messages(_messages),
      is_recv(false)
{
}

P2PEvent::P2PEvent(QList<P2PEvent *> * _subevents)
    : CommEvent(_subevents->first()->enter, _subevents->last()->exit,
                _subevents->first()->function, _subevents->first()->process,
                _subevents->first()->phase),
      subevents(_subevents),
      messages(new QVector<Message *>()),
      is_recv(_subevents->first()->is_recv)
{
    this->depth = _subevents->first()->depth;
    for (QList<P2PEvent *>::Iterator evt = _subevents->begin();
         evt != subevents->end(); ++evt)
    {
        for (QVector<Message *>::Iterator msg = (*evt)->messages->begin();
             msg != (*evt)->messages->end(); ++msg)
        {
            if (is_recv)
                (*msg)->receiver = this;
            else
                (*msg)->sender = this;
            messages->append(*msg);
        }
    }
}

P2PEvent::~P2PEvent()
{
    for (QVector<Message *>::Iterator itr = messages->begin();
         itr != messages->end(); ++itr)
    {
            delete *itr;
            *itr = NULL;
    }
    delete messages;

    if (subevents)
        delete subevents;
}

bool P2PEvent::isReceive()
{
    return is_recv;
}


void P2PEvent::set_stride_relationships(CommEvent * base)
{
    base->stride_children->insert(this);
    stride_parents->insert(base);
}

QSet<Partition *> * P2PEvent::mergeForMessagesHelper()
{
    QSet<Partition *> * parts = new QSet<Partition *>();
    for (QVector<Message *>::Iterator msg = messages->begin();
         msg != messages->end(); ++msg)
    {
        parts->insert((*msg)->receiver->partition);
        parts->insert((*msg)->sender->partition);
    }
}
