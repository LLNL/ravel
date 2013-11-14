#include "otfconverter.h"

OTFConverter::OTFConverter()
{
}

OTFConverter::~OTFConverter()
{
}

Trace * OTFConverter::importOTF(QString filename)
{
    // Start with the rawtrace similar to what we got from PARAVER
    OTFImporter * importer = new OTFImporter();
    rawtrace = importer->importOTF(filename.toStdString().c_str());
    trace = new Trace(rawtrace->num_processes);

    // Start setting up new Trace
    delete trace->functions;
    trace->functions = rawtrace->functions;

    delete trace->functionGroups;
    trace->functionGroups = rawtrace->functionGroups;

    // Find the MPI Group key
    for (QMap<int, QString>::Iterator itr = trace->functionGroups->begin(); itr != trace->functionGroups->end(); ++itr)
    {
        if (itr.value().contains("MPI")) {
            trace->mpi_group = itr.key();
            break;
        }
    }

    // Convert the events into matching enter and exit
    trace->mpi_events = new QVector<QList<Event *> * >(rawtrace->num_processes);
    for (int i = 0; i < rawtrace->num_processes; ++i)
        (*(trace->mpi_events))[i] = new QList<Event *>();
    matchEvents();

    // Match messages to previously connected events
    matchMessages();

    delete importer;
    delete rawtrace;

    return trace;
}

// Determine events as blocks of matching enter and exit
void OTFConverter::matchEvents()
{
    // We can handle each set of events separately
    QStack<Event *> * stack = new QStack<Event *>();
    for (QVector<QVector<EventRecord *> *>::Iterator pitr = rawtrace->events->begin(); pitr != rawtrace->events->end(); ++pitr) {
        int depth = 0;

        for (QVector<EventRecord *>::Iterator itr = (*pitr)->begin(); itr != (*pitr)->end(); ++itr)
        {
            if ((*itr)->value == 0) // End of a subroutine
            {
                Event * e = stack->pop();
                e->exit = (*itr)->time;
                if (!stack->isEmpty()) {
                    e->caller = stack->top();
                    (*(stack->top))->callees->append(e);
                }
                depth--;
            }
            else // Begin a subroutine
            {
                Event * e = new  Event((*itr)->time, 0, (*itr)->value, (*itr)->process -1, -1);

                e->depth = depth;
                if (depth == 0)
                    (*(trace->roots))[(*itr)->process]->append(e);
                depth++;

                // Keep track of the mpi_events for partitioning
                if ((((*trace)->functions)[e->process])->group == trace->mpi_group)
                    ((*(trace->mpi_events))[e->process])->prepend(e);

                stack->push(e);
                (*(trace->events))[(*itr)->process]->append(e);
            }
        }
        stack->clear();
    }
}


// Match the messages to the events.
void OTFConverter::matchMessages()
{
    for (QVector<CommRecord *>::Iterator itr = rawtrace->messages->begin(); itr != rawtrace->messages->end(); ++itr)
    {
        Message * m = new Message((*itr)->send_time, (*itr)->recv_time);

        Event * recv_evt = find_comm_event(search_child_ranges( (*((*(trace->roots))[(*itr)->receiver]))->children,
                                           (*itr->recv_time)), (*itr->recv_time));
        if (recv_evt) {
            recv_evt->messages->append(m);
            m->receiver = recv_evt;
        } else {
            std::cout << "Error finding recv event for " << (*itr)->sender << "->" << (*itr)->receiver
                      << " (" << (*itr)->send_time << ", " << (*itr)->recv_time << std::endl;
        }

        Event * send_evt = find_comm_event(search_child_ranges( (*((*(trace->roots))[(*itr)->sender]))->children,
                                           (*itr->send_time)), (*itr->send_time));
        if (send_evt) {
            send_evt->messages->append(m);
            m->sender = send_evt;
            trace->send_events->append(send_evt); // Keep track of the send events for merging later
        } else {
            std::cout << "Error finding send event for " << (*itr)->sender << "->" << (*itr)->receiver
                      << " (" << (*itr)->send_time << ", " << (*itr)->recv_time << std::endl;
        }

    }
}

// Binary search for event containing time
Event * OTFConverter::search_child_ranges(QVector<Event *> * children, unsigned long long int time)
{
    int imin = 0;
    int imax = children->size() - 1;
    while (imax >= imin):
        imid = (imin + imax) / 2;
        if (((*(children))[imid])->exit < time)
            imin = imid + 1;
        else if (((*(children))[imid])->enter > time)
            imax = imid - 1;
        else
            return (*children)[imid];

    return NULL;
}

// Find event containing this comm
Event * OTFConverter::find_comm_event(Event * evt, unsigned long long int time)
{
    // Pass back up null if something went wrong
    if (!evt)
        return evt;

    // No children, this must be it
    if (evt->callees->size() == 0)
        return evt;

    // Otherwise, continue search
    return find_comm_event(search_child_ranges(evt->callees, time), time);
}
