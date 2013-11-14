#include "partition.h"

Partition::Partition()
{
    events = new QMap<int, QList<Event *> *>;
    tindex = -1;

    parents = new QSet<Partition *>();
    children = new QSet<Partition *>();
    old_parents = new QSet<Partition *>();
    old_children = new QSet<Partition *>();
}

Partition::~Partition()
{
    for (QMap<int, QList<Event *> *>::Iterator eitr = events->begin(); eitr != events->end(); ++eitr) {
        /*for (QList<Event *>::Iterator itr = (*eitr)->begin(); itr != (*eitr)->end(); ++itr) {
            delete *itr;
            *itr = NULL;
        }*/ // Don't necessarily delete events due to merging
        delete *eitr;
        *eitr = NULL;
    }
    delete events;

    delete parents;
    delete children;
    delete old_parents;
    delete old_children;
}

// Call when we are sure we want to delete events
void Partition::deleteEvents()
{
    for (QMap<int, QList<Event *> *>::Iterator eitr = events->begin(); eitr != events->end(); ++eitr) {
        for (QList<Event *>::Iterator itr = (*eitr)->begin(); itr != (*eitr)->end(); ++itr) {
            delete *itr;
            *itr = NULL;
        }
    }
}

void Partition::addEvent(Event * e)
{
    if (events->contains(e->process))
    {
        ((*events)[e->process])->append(e);
    }
    else
    {
        (*events)[e->process] = new QList<Event *>();
        ((*events)[e->process])->append(e);
    }
}

void Partition::sortEvents(){
    for (QMap<int, QList<Event *> *>::Iterator eitr = events->begin(); eitr != events->end(); ++eitr) {
        qSort((*eitr)->begin(), (*eitr)->end(), dereferencedLessThan<Event>);
    }
}

unsigned long long int Partition::distance(Partition * other)
{
    unsigned long long int dist = ULLONG_MAX;
    for (QMap<int, QList<Event *> *>::Iterator eitr = events->begin(); eitr != events->end(); ++eitr)
    {
        if (other->events->contains(eitr.key()))
        {
            Event * my_last = (*eitr)->last();
            Event * other_first = ((*(other->events))[eitr.key()])->first();
            if (other_first->enter > my_last->exit)
                dist = std::min(dist, other_first->enter - my_last->exit);
            else
            {
                Event * my_first = (*eitr)->first();
                Event * other_last = ((*(other->events))[eitr.key()])->last();
                dist = std::min(dist, my_first->enter - other_last->exit);
            }
        }
    }
    return dist;
}

void Partition::step()
{
    free_recvs = new QList<Event *>();
    QList<Message *> * message_list = new QList<Message *>();

    int stride, step;
    Event * last_send;
    QList<Event *> * last_recvs;
    for (QMap<int, QList<Event *> *>::Iterator eitr = events->begin(); eitr != events->end(); ++eitr) {
        last_recvs = new QList<Event *>();
        stride = 0;
        step = 0;
        last_send = NULL;
        for (QList<Event *>::Iterator itr = (*eitr)->begin(); itr != (*eitr)->end(); ++itr) {
            is_recv = false;
            (*itr)->step = stride;
            for (QVector<Message *>::Iterator mitr = (*itr)->messages->begin(); mitr != (*itr)->messages->end(); ++mitr)
                if ((*itr) == (*mitr)->sender)
                    message_list->append(*mitr);
                else
                    (*itr)->is_recv = true;
            ++step;
            if ((*itr)->is_recv)
            {
                (*itr)->last_send = last_send;
                last_recvs.append(*itr);
            }
            else
            {
                ++stride;
                step = 0;
                last_send = *itr;
                (*itr)->last_recvs = last_recvs;
                for (QList<Event *>::Iterator ritr = (*itr)->last_recvs->begin(); ritr != (*itr)->last_recvs->end(); ++ritr)
                    (*ritr)->next_send = *itr;
                last_recvs = new QList<Event *>();
            }
        }
        if (last_recvs->size() > 0)
            free_recvs->append(last_recvs[0]);
    }

    qSort(message_list->begin(), message_list->end(), dereferencedLessThan<Message>);
    for (QList<Message *>::Iterator itr = message_list->begin(); itr != message_list->end(); ++itr)
        step_receive(*itr);

    finalize_steps();

    // Delete the last_recvs from all the events here

    delete message_list;
    delete free_recvs;
    delete last_recvs;
}

void Partition::step_receive(Message * msg)
{
    int previous_step;
    Event * previous_event, * next_event;
    if (msg->sender->step <= msg->receiver->step)
    {
        msg->receiver->step = msg->sender->step + 1;
        previous_step = msg->receiver->step;
        previous_event = msg->receiver;
        next_event = msg->receiver->comm_next;
        while (next_event && (next_event->partition == msg->receiver->partition))
        {
            if (previous_event->is_recv)
                next_event->step = previous_step;
            else
                next_event->step = previous_step + 1;

            previous_step = next_event->step;
            previous_event = next_event;
            next_event = next_event->comm_next;
        }
    }
}

void Partition::restep()
{
    // Set step dict
    QMap<int, QList<Event *> *> * step_dict = new QMap<int, QList<Event *> *>();
    for (QMap<int, QList<Event *> *>::Iterator eitr = events->begin(); eitr != events->end(); ++eitr)
    {
        for (QList<Event *>::Iterator itr = (eitr.value())->begin(); itr != (eitr.value())->end(); ++itr)
        {
            if (!(step_dict->contains((*itr)->step)))
                (*step_dict)[(*itr)->step] = new QList<Event *>();
            ((*step_dict)[(*itr)->step])->append(*itr);
        }
    }

    // Find the send steps/build the working dict
    QList<int> steps = step_dict->keys();
    qSort(steps);
    QMap<int, int> last_step();
    QList<int> processes = events->keys();
    QMap<int, QMap<int, Event *>> working_step();
    for (QList<int>::Iterator itr = processes.begin(); itr != processes.end(); ++itr)
    {
        last_step[*itr] = -1;
        working_step[*itr] = QMap<int, Event *>();
    }

    for (QMap<int, QList<Event *> *>::Iterator sitr = step_dict->begin(); sitr != step_dict->end(); ++sitr)
    {
        for (QList<Event *>::Iterator itr = (sitr.value())->begin(); itr != (sitr.value())->end(); ++itr)
        {
            if (!((*itr)->is_recv))
            {
                (working_step[(*itr)->process])[sitr.key()] = *itr;
                last_step[(*itr)->process] = sitr.key();
            }
        }
    }

    // Set the receives
    QMap<int, int> send_step();
    send_step[-1] = -1;
    QList<Event *> sends();
    int current_step, submax, partition_step = 0;
    // For each step, we expand out working one process at a time putting final steps on.
    for (QList<int>::Iterator step = steps.begin(); step != steps.end(); ++step)
    {
        sends.clear();
        submax = partition_step;
        for (QList<int>::Iterator process = processes.begin(); process != processes.end(); ++process)
        {
            if ((working_step[*process]).contains(*step))
            {
                Event * evt = sstep.value();
                sends.append(evt); // Collect sends at this step
                current_step = send_step[last_step[*process]] + 1; // Where we were at this step for this process

                // For all the recvs before the send, set the step, shifting forward if need be
                for (QList<Event *>::Iterator recv = evt->last_recvs->begin(); recv != evt->last_recvs->end(); ++recv)
                {
                    (*recv)->step = current_step;
                    // Did we break our rules? If so fix. We know the step would have gotten set for the
                    // sender by the time we're here so we can use 'step'
                    for (QVector<Message *>::Iterator msg = (*recv)->messages->begin(); msg != (*recv)->messages->end; ++msg)
                        if ((*msg)->sender != (*recv) && (*msg)->sender->step >= current_step)
                        {
                            current_step = (*msg)->sender->step + 1;
                            (*recv)->step = current_step;
                        }

                    ++current_step;
                }
                submax = std::max(current_step, submax); // Sends get sent to maximum
            }
        }

        // Set the step for all the sends
        for (QList<Event *>::Iterator send = sends.begin(); send != sends.end(); ++send)
            (*send)->step = submax;
        send_step[*step] = submax;
    }

    // Handle free receives
    int last_step;
    Event * next_recv;
    for (QList<Event *>::Iterator recv = free_recvs->begin(); recv != free_recvs->end(); ++recv)
    {
        // Find the step of that first free recv
        int step = 0;
        if ((*recv)->comm_prev && (*recv)->comm_prev->partition == (*recv)->partition)
            step = (*recv)->comm_prev->step + 1;
        for (QVector<Message *>::Iterator msg = (*recv)->messages->begin(); msg != (*recv)->messages->end(); ++msg)
            if ((*msg)->sender != (*recv))
                step = std::max(step, (*msg)->sender->step + 1);
        (*recv)->step = step;
        last_step = step;

        // Set the steps of all the free recvs after it
        next_recv = (*recv)->comm_next;
        while (next_recv && next_recv->partition == (*recv)->partition)
        {
            step = last_step;
            for (QVector<Message *>::Iterator msg = next_recv->messages->begin(); msg != next_recv->messages->end(); ++msg)
                if ((*msg)->sender != next_recv)
                    step = std::max(step, (*msg)->sender->step + 1);
            next_recv->step = step + 1;
            last_step = next_recv->step;
            next_recv = next_recv->comm_next;
        }
    }

    // delete step dict
    for (QMap<int, QList<Event *> *>::Iterator eitr = step_dict->begin(); eitr != step_dict->end(); ++eitr)
        delete *eitr;
    delete step_dict;
}

void Partition::finalize_steps()
{
    // Unlike the Python version, this does nothing for waitall
    // This makes this function quite short in that it is:
    // restepping (expanding)
    // building the step dict <-- not needed for this lateness
    // determining the max step

    restep();

    // Find partition max_step
    max_step = 0;
    for (QMap<int, QList<Event *> *>::Iterator eitr = events->begin(); eitr != events->end(); ++eitr)
        max_step = std::max((*eitr)->last()->step, max_step);
}
