#include "charmimporter.h"
#include "eventrecord.h"
#include "commrecord.h"
#include "rpartition.h"
#include "commevent.h"
#include "message.h"
#include "p2pevent.h"
#include "event.h"
#include "taskgroup.h"
#include <iostream>
#include <QStringList>
#include <QDir>
#include <QFileInfo>
#include <QStack>
#include <zlib.h>
#include <sstream>
#include <fstream>
#include "trace.h"
#include "task.h"
#include "taskgroup.h"
#include "function.h"
#include "message.h"
#include "p2pevent.h"
#include "commevent.h"
#include "event.h"
#include "otfimportoptions.h"
#include "primarytaskgroup.h"

#include "general_util.h"

CharmImporter::CharmImporter()
    : chares(new QMap<int, Chare*>()),
      entries(new QMap<int, Entry*>()),
      options(NULL),
      version(0),
      processes(0),
      hasPAPI(false),
      numPAPI(0),
      main(-1),
      traceChare(-1),
      reductionChare(-1),
      addContribution(-1),
      ckArrayChare(-1)
,      recvMsg(-1),
      contribute(-1),
      traceEnd(0),
      num_application_tasks(0),
      trace(NULL),
      unmatched_recvs(NULL),
      sends(NULL),
      charm_stack(new QStack<CharmEvt *>()),
      charm_events(NULL),
      task_events(NULL),
      pe_events(NULL),
      //roots(NULL),
      charm_p2ps(NULL),
      pe_p2ps(NULL),
      idle_to_next(new QMap<Event *, int>()),
      messages(new QVector<CharmMsg *>()),
      primaries(new QMap<int, PrimaryTaskGroup *>()),
      taskgroups(new QMap<int, TaskGroup *>()),
      functiongroups(new QMap<int, QString>()),
      functions(new QMap<int, Function *>()),
      arrays(new QMap<int, ChareArray *>()),
      groups(new QMap<int, ChareGroup *>()),
      chare_to_task(new QMap<ChareIndex, int>()),
      last(NULL),
      last_send(NULL),
      idles(QList<Event *>()),
      seen_chares(QSet<QString>()),
      application_chares(QSet<int>())
{
}

CharmImporter::~CharmImporter()
{
    for (QMap<int, Chare*>::Iterator itr = chares->begin();
         itr != chares->end(); ++itr)
    {
        delete itr.value();
    }
    delete chares;

    for (QMap<int, Entry*>::Iterator itr = entries->begin();
         itr != entries->end(); ++itr)
    {
        delete itr.value();
    }
    delete entries;

    for (QVector<CharmMsg *>::Iterator itr = messages->begin();
         itr != messages->end(); ++itr)
    {
        delete *itr;
    }

    delete charm_stack;

    delete idle_to_next;

    for (int i = 0; i < processes; i++)
    {
        delete (*unmatched_recvs)[i];
        delete (*sends)[i];
        delete (*charm_events)[i];
        delete (*pe_events)[i];
        delete (*pe_p2ps)[i];
    }

    delete unmatched_recvs;
    delete sends;
    delete charm_events;
    delete pe_events;
    delete pe_p2ps;
}

void CharmImporter::importCharmLog(QString dataFileName, OTFImportOptions * _options)
{
    std::cout << "Reading " << dataFileName.toStdString().c_str() << std::endl;
    options = _options;
    readSts(dataFileName);

    unmatched_recvs = new QVector<QMap<int, QList<CharmMsg *> *> *>(processes);
    sends = new QVector<QMap<int, QList<CharmMsg *> *> *>(processes);
    charm_events = new QVector<QVector<CharmEvt *> *>(processes);
    pe_events = new QVector<QVector<Event *> *>(processes);
    pe_p2ps = new QVector<QVector<P2PEvent *> *>(processes);
    for (int i = 0; i < processes; i++) {
        (*unmatched_recvs)[i] = new QMap<int, QList<CharmMsg *> *>();
        (*sends)[i] = new QMap<int, QList<CharmMsg *> *>();
        (*charm_events)[i] = new QVector<CharmEvt *>();
        (*pe_events)[i] = new QVector<Event *>();
        (*pe_p2ps)[i] = new QVector<P2PEvent *>();
    }

    processDefinitions();

    // Read individual log files
    QFileInfo file_info = QFileInfo(dataFileName);
    QDir directory = file_info.dir();
    QString basename = file_info.completeBaseName();
    QString suffix = ".log";
    QString path = file_info.absolutePath();
    bool gzflag = false;
    if (directory.exists(basename + ".0.log.gz"))
    {
        gzflag = true;
        suffix += ".gz";
    }
    for (int i = 0; i < processes; i++)
    {
        charm_stack->clear();
        readLog(path + "/" + basename + "." + QString::number(i) + suffix,
                gzflag, i);
    }

    // At this point, I have a list of events per PE
    // Now I have to check to see what chares are actually arrays
    // and then convert these by-PE events into by-chare
    // events based on these arrays. I should also make taskgroups and stuff
    int num_tasks = makeTasks();

    // Now that we have num_tasks, create the rawtrace to hold the conversion
    trace = new Trace(num_tasks, processes);
    trace->units = 9;
    trace->functions = functions;
    trace->functionGroups = functiongroups;
    trace->taskgroups = taskgroups;
    trace->primaries = primaries;
    trace->collective_definitions = new QMap<int, OTFCollective *>();
    trace->collectives = new QMap<unsigned long long, CollectiveRecord *>();
    //trace->counters = new QMap<unsigned int, Counter *>();
    //trace->counter_records = new QVector<QVector<CounterRecord *> *>(num_tasks);
    trace->collectiveMap = new QVector<QMap<unsigned long long, CollectiveRecord *> *>(num_tasks);
    task_events = new QVector<QVector<CharmEvt *> *>(num_tasks);
    charm_p2ps = new QVector<QVector<P2PEvent *> *>(num_tasks);
    for (int i = 0; i < num_tasks; i++) {
        (*(trace->collectiveMap))[i] = new QMap<unsigned long long, CollectiveRecord *>();
        (*(trace->events))[i] = new QVector<Event *>();
        (*(task_events))[i] = new QVector<CharmEvt *>();
        (*(charm_p2ps))[i] = new QVector<P2PEvent *>();
    }
    trace->pe_events = pe_events;
    //delete trace->roots;
    //trace->roots = roots;

    // Change per-PE stuff to per-chare stuff
    makeTaskEvents();

    // Build partitions
    buildPartitions();

    // Keep a list of what chares have been seen
    if (verbose)
    {
        QList<QString> tmp = seen_chares.toList();
        qSort(tmp.begin(), tmp.end());
        for (QList<QString>::Iterator ch = tmp.begin(); ch != tmp.end(); ++ch)
        {
            std::cout << "Seen chare: " << (*ch).toStdString().c_str() << std::endl;
        }
    }

    // Delete newly created temp stuff
    cleanUp();
}

void CharmImporter::cleanUp()
{
    for (QVector<QMap<int, QList<CharmMsg *> *> *>::Iterator itr
         = unmatched_recvs->begin(); itr != unmatched_recvs->end(); ++itr)
    {
        for (QMap<int, QList<CharmMsg *> *>::Iterator eitr
             = (*itr)->begin(); eitr != (*itr)->end(); ++eitr)
        {
            // CharmMsgs deleted in charm events;
            delete *eitr;
        }
        delete *itr;
    }
    delete unmatched_recvs;

    for (QVector<QMap<int, QList<CharmMsg *> *> *>::Iterator itr
         = sends->begin(); itr != sends->end(); ++itr)
    {
        for (QMap<int, QList<CharmMsg *> *>::Iterator eitr
             = (*itr)->begin(); eitr != (*itr)->end(); ++eitr)
        {
            // CharmMsgs deleted in charm events;
            delete *eitr;
        }
        delete *itr;
    }
    delete sends;

    for (QVector<QVector<CharmEvt *> *>::Iterator itr
         = charm_events->begin(); itr != charm_events->end(); ++itr)
    {
        for (QVector<CharmEvt *>::Iterator eitr
             = (*itr)->begin(); eitr != (*itr)->end(); ++eitr)
        {
            delete *eitr;
            *eitr = NULL;
        }
        delete *itr;
    }
    delete charm_events;

    for (QVector<QVector<CharmEvt *> *>::Iterator itr
         = task_events->begin(); itr != task_events->end(); ++itr)
    {
        // Evts deleted in charm_events
        delete *itr;
    }
    delete task_events;
}

void CharmImporter::readLog(QString logFileName, bool gzipped, int pe)
{
    last = NULL;
    if (gzipped)
    {
        gzFile logfile = gzopen(logFileName.toStdString().c_str(), "r");
        char * buffer = new char[1024];
        gzgets(logfile, buffer, 1024); // Skip first line
        while (gzgets(logfile, buffer, 1024))
            parseLine(QString::fromUtf8(buffer).simplified(), pe);

        gzclose(logfile);
    }
    else
    {
        std::ifstream logfile(logFileName.toStdString().c_str());
        std::string line;
        std::getline(logfile, line); // Skip first line
        while(std::getline(logfile, line))
            parseLine(QString::fromStdString(line), pe);

        logfile.close();
    }

}

// Take CharmEvt tidbits and make Events out of them
// Will fill the trace->pe_events and the trace->roots
// After this is done we will take the trace events and
// turn them into partitions.
void CharmImporter::makeTaskEvents()
{

    // Now make the task event hierarchy with roots and such
    QStack<CharmEvt *>  * stack = new QStack<CharmEvt *>();
    int depth, phase;
    CharmEvt * bgn = NULL;
    int counter = 0;

    bool have_arrays = false;
    if (arrays->size() > 0)
        have_arrays = true;

    if (verbose)
    {
        for (QMap<ChareIndex, int>::Iterator map = chare_to_task->begin();
             map != chare_to_task->end(); ++map)
        {
            std::cout << map.key().toVerboseString().toStdString().c_str() << " <---> " << map.value() << std::endl;
        }
        for (QSet<int>::Iterator appchare = application_chares.begin();
             appchare != application_chares.end(); ++appchare)
        {
            std::cout << "App Chare: " << (*appchare) << std::endl;
        }
    }

    // Go through each PE separately, creating events and if necessary
    // putting them into charm_p2ps and setting their tasks appropriately
    for (QVector<QVector<CharmEvt *> *>::Iterator event_list = charm_events->begin();
         event_list != charm_events->end(); ++event_list)
    {
        stack->clear();
        depth = 0;
        phase = 0;
        counter++;
        for (QVector<CharmEvt *>::Iterator evt = (*event_list)->begin();
             evt != (*event_list)->end(); ++evt)
        {
            if ((*evt)->enter)
            {
                if (verbose)
                {
                    std::cout << "    Push stack " << " on pe " << (*evt)->pe << " my array " << (*evt)->arrayid;
                    std::cout << " for " << chares->value(entries->value((*evt)->entry)->chare)->name.toStdString().c_str();
                    std::cout << "::" << entries->value((*evt)->entry)->name.toStdString().c_str();
                    std::cout << " with index " << (*evt)->index.toVerboseString().toStdString().c_str();
                    std::cout << " at " << (*evt)->time << std::endl;
                }

                // Enter is the only place with the true array id so we must
                // get the task id here.
                if (have_arrays)
                {
                    if (verbose)
                        std::cout << "            ArrayID " << (*evt)->arrayid << " and chare " << (*evt)->index.chare << std::endl;
                    if (((*evt)->arrayid == 0 && (!application_chares.contains((*evt)->index.chare)
                                                 || ((*evt)->index.chare == main && (*evt)->pe != 0)))
                         || (*evt)->index.chare == -1)
                    {
                        (*evt)->task = -1;
                        if (verbose)
                            std::cout << "            setting to -1" << std::endl;
                    }
                    else if (!stack->isEmpty()
                             && (*evt)->index.chare == main
                             && (*evt)->entry == SEND_FXN
                             && stack->top()->chare != main)
                    {
                        // This is the case where we attributed to main incorrectly since
                        // a send records its send destination chare rather than its called
                        // chare.
                        (*evt)->task = num_application_tasks + (*evt)->pe;
                        if (verbose)
                            std::cout << "            setting to runtime" << std::endl;
                    }
                    else // Should get main if nothing else works
                    {
                        (*evt)->task = chare_to_task->value((*evt)->index);
                        if (verbose)
                            std::cout << "            setting from chare_to_task" << std::endl;
                    }
                }
                else
                {
                    if (verbose)
                        std::cout << "            setting from no arrays" << std::endl;
                    if (chares->value((*evt)->index.chare)->indices->size() <= 1
                        && !application_chares.contains((*evt)->index.chare))
                        (*evt)->task = -1;
                    else
                        (*evt)->task = chare_to_task->value((*evt)->index);
                }

                if ((*evt)->task == -1) // runtime chare
                {
                    (*evt)->task = num_application_tasks + (*evt)->pe;
                    if (verbose)
                        std::cout << "            setting to runtime from -1" << std::endl;
                }
                if (verbose)
                    std::cout << "            Setting Task to " << (*evt)->task << std::endl;

                stack->push(*evt);
                depth++;
            } // Handle enter stuff
            else
            {
                // In the case we start in the middle
                if (stack->isEmpty())
                    continue;

                bgn = stack->pop();
                // We may have stuff on the stack that doesn't match,
                // in which case we have to search for something that does match.
                // This is impossible given our current understanding of nesting but
                // not sure if runtime will upset this.
                /*while (bgn->entry != (*evt)->entry
                       || bgn->event != (*evt)->event
                       || bgn->pe != (*evt)->pe)
                {
                    if (stack->isEmpty())
                        bgn = NULL;
                    else
                        bgn = stack->pop();
                } */

                if (bgn)
                    depth = makeTaskEventsPop(stack, bgn, (*evt)->time, phase, depth);
            } // Handle this Event
        } // Loop Event

        // Unfinished begins
        while (!stack->isEmpty())
            depth = makeTaskEventsPop(stack, stack->pop(), traceEnd, phase, depth);
    } // Loop Event Lists


    // Sort chare events in time
    for (QVector<QVector<P2PEvent *> *>::Iterator event_list
         = charm_p2ps->begin(); event_list != charm_p2ps->end();
         ++event_list)
    {
        qSort((*event_list)->begin(), (*event_list)->end(), dereferencedLessThan<P2PEvent>);
    }

    delete stack;
}

int CharmImporter::makeTaskEventsPop(QStack<CharmEvt *> * stack, CharmEvt * bgn,
                                     long endtime, int phase, int depth)
{
    Event * e = NULL;

    if (verbose)
    {
        std::cout << "    Pop stack " << " on pe " << bgn->pe << " my array " << bgn->arrayid;
        std::cout << " for " << chares->value(entries->value(bgn->entry)->chare)->name.toStdString().c_str();
        std::cout << "::" << entries->value(bgn->entry)->name.toStdString().c_str();
        std::cout << " with index " << bgn->index.toVerboseString().toStdString().c_str();
        std::cout << " indicating task " << bgn->task;
        std::cout << " at " << endtime << std::endl;
    }

    trace->metrics->append("Idle");
    (*(trace->metric_units))["Idle"] = getUnits(trace->units);

    if (trace->functions->value(bgn->entry)->group == 0) // Send or Recv
    {
        // We need to go and wire up all the mesages appropriately
        // Then we need to make sure these get sorted in the right
        // place for charm messages so we can make partitions out of them appropriately.
        // Note this has to happen after the fact so we can sort appropriately
        if (bgn->task < 0)
        {
            return --depth;
        }
        QVector<Message *> * msgs = new QVector<Message *>();
        for (QList<CharmMsg *>::Iterator cmsg = bgn->charmmsgs->begin();
             cmsg != bgn->charmmsgs->end(); ++cmsg)
        {
            if ((*cmsg)->tracemsg)
            {
                msgs->append((*cmsg)->tracemsg);
            }
            else if (!(*cmsg)->send_evt || !(*cmsg)->recv_evt) // incomplete, discard
            {
               continue;
            }
            else
            {
                Message * msg = new Message((*cmsg)->sendtime,
                                            (*cmsg)->recvtime,
                                            0);
                (*cmsg)->tracemsg = msg;
                msgs->append(msg);
            }

            if (bgn->entry == SEND_FXN)
            {
                if (!((*cmsg)->send_evt->trace_evt))
                {
                    (*cmsg)->tracemsg->sender = new P2PEvent(bgn->time,
                                                             endtime,
                                                             bgn->entry,
                                                             bgn->task,
                                                             bgn->pe,
                                                             phase,
                                                             msgs);
                    (*cmsg)->send_evt->trace_evt = (*cmsg)->tracemsg->sender;
                    charm_p2ps->at(bgn->task)->append((*cmsg)->tracemsg->sender);
                    if (verbose)
                        std::cout << "                      Adding" << std::endl;

                    e = (*cmsg)->tracemsg->sender;
                    pe_p2ps->at(bgn->pe)->append((*cmsg)->tracemsg->sender);
                    (*cmsg)->tracemsg->sender->addMetric("Idle", 0, 0);

                }
                else
                {
                    (*cmsg)->tracemsg->sender = (*cmsg)->send_evt->trace_evt;
                    // We have already set e here.
                }

            }
            else if (bgn->entry == RECV_FXN)
            {
                (*cmsg)->tracemsg->receiver = new P2PEvent(bgn->time,
                                                           endtime,
                                                           bgn->entry,
                                                           bgn->task,
                                                           bgn->pe,
                                                           phase,
                                                           msgs);

                (*cmsg)->tracemsg->receiver->is_recv = true;
                charm_p2ps->at(bgn->task)->append((*cmsg)->tracemsg->receiver);
                if (verbose)
                    std::cout << "                      Adding" << std::endl;

                e = (*cmsg)->tracemsg->receiver;
                (*cmsg)->tracemsg->receiver->addMetric("Idle", 0, 0);
                pe_p2ps->at(bgn->pe)->append((*cmsg)->tracemsg->receiver);
            }
        }
        if (!e) // No messages were recorded with this, skip for now and don't include
        {
            return --depth;
        }
    }
    else // Non-comm event
    {
        e = new Event(bgn->time, endtime, bgn->entry,
                      bgn->task, bgn->pe);
        if (bgn->entry == IDLE_FXN)
        {
            // Index of the next comm event after this IDLE
            idle_to_next->insert(e, pe_p2ps->at(bgn->pe)->size());
        }
    }

    if (trace->functions->value(bgn->entry)->group == 2) // IDLE
        idles.append(e);

    depth--;
    e->depth = depth;
    if (depth == 0)
    {
        (*(trace->roots))[bgn->pe]->append(e);
        if (verbose)
            std::cout << "                    Adding as root event between " << e->enter << " and " << e->exit << std::endl;
    }

    if (e->exit > endtime)
        endtime = e->exit;
    if (!stack->isEmpty())
    {
        stack->top()->children->append(e);
    }
    for (QList<Event *>::Iterator child = bgn->children->begin();
         child != bgn->children->end(); ++child)
    {
        e->callees->append(*child);
        (*child)->caller = e;
    }

    (*(trace->pe_events))[bgn->pe]->append(e);
    return depth;
}


// Look through all idle events and check which events after them
// are triggered by events that occur before them. This is difficult
// when clocks are not synchronized. This is also difficult because
// it may occur chainings of happened before relationships.
// So I guess we need to look at all the happened before relationships
// for each recv, which there is only one, and see whether there's a sizable gap
void CharmImporter::chargeIdleness()
{
    Event * idle_evt = NULL;
    P2PEvent * comm_evt = NULL;
    P2PEvent * sender = NULL;
    int idle_pos = -1;
    bool flag = false;
    long idle_diff;
    for (QMap<Event *, int>::Iterator idle = idle_to_next->begin();
         idle != idle_to_next->end(); ++idle)
    {
        idle_evt = idle.key();
        idle_pos = idle.value();
        flag = true;
        while (flag)
        {
            comm_evt = pe_p2ps->at(idle_evt->pe)->at(idle_pos);
            if (comm_evt->is_recv) // only makes sense for recvs
            {
                // Basically if there's required send doesn't take place
                // until after the idle begins, charge it to that sender.
                // If we have a caller, we know the send doesn't happen until
                // the end of the caller, so use that as the true time.
                // One we get to an event where the sender doesn't exhibit
                // this behavior, then we figure the rest of it must be okay
                // because it was no longer being held up.
                sender = comm_evt->messages->first()->sender;
                idle_diff = sender->enter - idle_evt->enter;
                if (sender->caller)
                {
                    idle_diff = sender->caller->exit - idle_evt->enter;
                }

                if (idle_diff > 0)
                {
                    idle_diff += sender->getMetric("Idle");
                    sender->setMetric("Idle", idle_diff, 0);
                }
                else
                {
                    flag = false;
                }
            }
            idle_pos++;
        }
    }
}

void CharmImporter::buildPartitions()
{
    QList<P2PEvent *> * events = new QList<P2PEvent *>();

    P2PEvent * prev = NULL;
    P2PEvent * true_prev = NULL;

    QList<QString> breakables = QList<QString>();
    if (!options->partitionByFunction && options->breakFunctions.length() > 0)
        breakables = options->breakFunctions.split(",");

    int count = 0;
    int taskid = 0;
    for (QVector<QVector<P2PEvent *> *>::Iterator p2plist = charm_p2ps->begin();
         p2plist != charm_p2ps->end(); ++p2plist)
    {

        // first make sure everything is in the absolutely right order.
        QList<int> toswap = QList<int>();
        prev = NULL;
        true_prev = NULL;
        for (int i = 0; i < (*p2plist)->size(); i++)
        {
            P2PEvent * p2p = (*p2plist)->value(i);
            if (prev && true_prev && prev->enter == true_prev->enter
                && p2p->caller != true_prev->caller
                && p2p->caller == prev->caller)
            {
                toswap.append(i-2);
                std::cout << "Swapping list " << count;
                std::cout << " for " << trace->functions->value(true_prev->caller->function)->name.toStdString().c_str();
                std::cout << " at " << p2p->enter << std::endl;
            }
            prev = true_prev;
            true_prev = p2p;
        }
        for (QList<int>::Iterator swapper = toswap.begin();
             swapper != toswap.end(); ++swapper)
        {
            P2PEvent * a = (*p2plist)->value(*swapper);
            (**p2plist)[*swapper] = (*p2plist)->at(*swapper + 1);
            (**p2plist)[*swapper + 1] = a;
        }

        prev = NULL;
        true_prev = NULL;

        if (verbose)
            std::cout << "PARTITIONING ON " << count << std::endl;

        count++;
        for (QVector<P2PEvent *>::Iterator p2p = (*p2plist)->begin();
             p2p != (*p2plist)->end(); ++p2p)
        {
            // We always set these
            /*if (prev)
                prev->true_next = *p2p;
            (*p2p)->true_prev = prev;
            */


            // comm_next / comm_prev only set within the same caller
            // as that's what we know is a true ordering
            // This is true for charm++, may not be true for
            // general task-based models
            /*if (verbose)
            {
                if (!(prev && prev->caller))
                {
                   std::cout << "-- no prev or prev->caller at time " << (*p2p)->enter << std::endl;
                }
                else
                {
                    std::cout << "-- prev caller is" << trace->functions->value(prev->caller->function)->name.toStdString().c_str();
                    std::cout << " and my caller is " << trace->functions->value((*p2p)->caller->function)->name.toStdString().c_str();
                    std::cout << " at " << (*p2p)->enter << std::endl;
                }
            }*/
            if (prev && prev->caller != NULL
                && (prev->caller == (*p2p)->caller))
            {
                prev->comm_next = *p2p;
                (*p2p)->comm_prev = prev;
            }
            prev = *p2p;

            // The ordering of
            if (!(*p2p)->is_recv && (*p2p)->comm_prev == NULL)
            {
                (*p2p)->true_prev = true_prev;
                if (true_prev)
                {
                    true_prev->true_next = *p2p;
                }
                true_prev = *p2p;
            }


            // End previous by making it into a partition if we have
            // changed functions... we may also split the function if:
            // 1) We are in the main function
            // 2) We are changing from app->runtime or runtime->app
            // 3) This is one of the breakable functions
            // 4) We are starting a a multicast? (probably a sign?)

            if (!events->isEmpty())
            {
                // 1) & 2)
                if ((*p2p)->caller == NULL
                    || ((*p2p)->caller != prev->caller)
                    || trace->functions->value((*p2p)->caller->function)->isMain) // 1)
                {
                    if (verbose)
                        std::cout << " ----- Making a partition due to caller split" << std::endl;
                    makePartition(events);
                    events->clear();
                }
                else if (prev && taskid < num_application_tasks) // 2)
                {
                    // First figure out if the previous comm event was app only or not.
                    bool prev_app = true, my_app = true;
                    if (prev->is_recv)
                    {
                        if (prev->messages->first()->sender->task >= num_application_tasks)
                        {
                            prev_app = false;
                        }
                    }
                    else
                    {
                        if (prev->messages->first()->receiver->task >= num_application_tasks)
                        {
                            prev_app = false;
                        }
                    }

                    // Check if we are application only
                    if ((*p2p)->is_recv)
                    {
                        if ((*p2p)->messages->first()->sender->task >= num_application_tasks)
                        {
                            my_app = false;
                        }
                    }
                    else
                    {
                        if ((*p2p)->messages->first()->receiver->task >= num_application_tasks)
                        {
                            my_app = false;
                        }
                    }

                    if (verbose)
                        std::cout << " --- my_app is " << my_app << " and theirs is " << prev_app << std::endl;
                    // These should be the same or split.
                    if (my_app != prev_app)
                    {
                        if (verbose)
                            std::cout << " ----- Making a partition due to runtime split" << std::endl;
                        makePartition(events);
                        events->clear();
                    }
                }

                if (!events->isEmpty() && !options->partitionByFunction)// 3)
                {
                    QString caller = trace->functions->value((*p2p)->caller->function)->shortname;
                    for (QList<QString>::Iterator fxn = breakables.begin();
                         fxn != breakables.end(); ++fxn)
                    {
                        if (caller.startsWith(*fxn, Qt::CaseInsensitive))
                        {
                            if (verbose)
                                std::cout << " ----- Making a partition due to given split" << std::endl;
                            makePartition(events);
                            events->clear();
                            break;
                        }
                    }
                }
            }


            events->append(*p2p);
            trace->events->at((*p2p)->task)->append(*p2p);
        }

        // Clear remaining in events;
        if (!events->isEmpty())
        {
            makePartition(events);
            events->clear();
        }

        taskid++;
    }

    delete events;
}


// We expect events to be in order, therefore we don't sort the partition at the end.
void CharmImporter::makePartition(QList<P2PEvent *> * events)
{
    Partition * p = new Partition();
    for (QList<P2PEvent *>::Iterator evt = events->begin();
         evt != events->end(); ++evt)
    {
        p->addEvent(*evt);
        (*evt)->partition = p;
    }
    if (events->first()->task >= num_application_tasks)
        p->runtime = true;
    p->new_partition = p;
    trace->partitions->append(p);
}


int CharmImporter::makeTasks()
{
    // Setup main task
    primaries->insert(0, new PrimaryTaskGroup(0, "main"));
    Task * mainTask = new Task(0, "main", primaries->value(0));
    primaries->value(0)->tasks->append(mainTask);
    chare_to_task->insert(ChareIndex(main, 0, 0, 0, 0), 0);
    application_chares.insert(main);

    int taskid = 1;
    if (arrays->size() > 0)
    {
        QMap<int, int> chare_count = QMap<int, int>();
        for (QMap<int, ChareArray *>::Iterator array = arrays->begin();
             array != arrays->end(); ++array)
        {
            if (!chare_count.contains(array.value()->chare))
                chare_count.insert(array.value()->chare, 1);
            else
                chare_count.insert(array.value()->chare,
                                   1 + chare_count.value(array.value()->chare));
        }

        QString ptg_name;
        for (QMap<int, ChareArray *>::Iterator array = arrays->begin();
             array != arrays->end(); ++array)
        {
            ptg_name = chares->value(array.value()->chare)->name;
            if (chare_count.value(array.value()->chare) > 1)
                ptg_name += "_" + QString::number(array.key());
            primaries->insert(array.key(),
                              new PrimaryTaskGroup(array.key(),
                                                   ptg_name));
            QList<ChareIndex> indices_list = array.value()->indices->toList();
            qSort(indices_list.begin(), indices_list.end());

            for (QList<ChareIndex>::Iterator index = indices_list.begin();
                 index != indices_list.end(); ++index)
            {
                Task * task = new Task(taskid,
                                       //chares->value(array.value()->chare)->name
                                       //+ "_" +
                                       (*index).toString(),
                                       primaries->value(array.key()));
                primaries->value(array.key())->tasks->append(task);
                chare_to_task->insert(*index, taskid);
                taskid++;
            }

            application_chares.insert(array.value()->chare);
        }
    }
    else
    {
        for (QMap<int, Chare *>::Iterator chare = chares->begin();
             chare != chares->end(); ++chare)
        {
            if (chare.value()->indices->size() > 1)
            {
                primaries->insert(chare.key(),
                                  new PrimaryTaskGroup(chare.key(),
                                                       chare.value()->name));
                QList<ChareIndex> indices_list = chare.value()->indices->toList();
                qSort(indices_list.begin(), indices_list.end());
                for (QList<ChareIndex>::Iterator index = indices_list.begin();
                     index != indices_list.end(); ++index)
                {
                    Task * task = new Task(taskid,
                                           (*index).toString(),
                                           //chare.value()->name + "_" + (*index).toString(),
                                           primaries->value(chare.key()));
                    primaries->value(chare.key())->tasks->append(task);
                    chare_to_task->insert(*index, taskid);
                    taskid++;
                }

                application_chares.insert(chare.key());
            }
        }
    }

    num_application_tasks = taskid;
    primaries->insert(chares->size(),
                      new PrimaryTaskGroup(chares->size(),
                                           "pe 0"));
    for (int i = 0; i < processes; i++)
    {
        Task * task = new Task(taskid,
                               "pe " + QString::number(i),
                               primaries->value(chares->size()));
        primaries->value(chares->size())->tasks->append(task);
        taskid++;
    }
    return taskid;
}


void CharmImporter::parseLine(QString line, int my_pe)
{
    int index, mtype, entry, event, pe;
    int arrayid = 0;
    ChareIndex id = ChareIndex(-1, 0,0,0,0);
    long time, msglen, sendTime, recvTime, numpes, cpuStart, cpuEnd;

    QStringList lineList = line.split(" ");
    int rectype = lineList.at(0).toInt();
    if (rectype == CREATION || rectype == CREATION_BCAST || rectype == CREATION_MULTICAST)
    {
        // Some type of (multi-send)
        mtype = lineList.at(1).toInt();
        entry = lineList.at(2).toInt();
        time = lineList.at(3).toLong();
        event = lineList.at(4).toInt();
        pe = lineList.at(5).toInt(); // Should be my pe
        msglen = -1;
        index = 6;
        if (version >= 2.0)
        {
            msglen = lineList.at(5).toLong();
            index++;
        }
        if (version >= 5.0)
        {
            sendTime = lineList.at(index).toLong();
            index++;
        }
        if (rectype == CREATION_BCAST || rectype == CREATION_MULTICAST)
        {
            numpes = lineList.at(index).toInt();
            index++;
            if (rectype == CREATION_MULTICAST)
            {
                index += numpes;
                // Let's not do anything with the destination yet.
                /*
                for (int j = 0; j < numpes; j++)
                {
                    lineList.at(index).toInt();
                    index++;
                }
                */
            }
        }
        if (version >= 7.0 && lineList.size() > index)
        {
            arrayid = lineList.at(index).toInt();
            index++;

            int chare = entries->value(entry)->chare;
            if (chare == reductionChare || chare == traceChare || chare == ckArrayChare)
                arrayid = 0;
            if (arrayid > 0 && !arrays->contains(arrayid))
            {
                arrays->insert(arrayid,
                               new ChareArray(arrayid,
                                              entries->value(entry)->chare));
            }
        }

        if (last)
        {
            arrayid = last->index.array;
        }

        CharmEvt * evt = new CharmEvt(SEND_FXN, time, my_pe,
                                      entries->value(entry)->chare, arrayid,
                                      true);
        // We are inside a begin_processing, so the create event we have
        // here naturally has the index of the begin_processing which is
        // what last should hold. However, the send may not be meaningful
        // should its recv not exist or go to a not-kept chare, so this needs
        // to be processed later.
        if (last)
        {
            evt->index = last->index;
        }

        charm_events->at(my_pe)->append(evt);

        /*seen_chares.insert(chares->value(entries->value(entry)->chare)->name
                            + "::" + entries->value(entry)->name);

                            */
        CharmEvt * send_end = new CharmEvt(SEND_FXN, time+1, my_pe,
                                           entries->value(entry)->chare, arrayid,
                                           false);
        charm_events->at(my_pe)->append(send_end);
        if (last)
            send_end->index = last->index;

        if (rectype == CREATION)
        {
            // Look for our message -- note this send may actually be a
            // broadcast. If that's the case, we weant to find all matches,
            // process them, and then remove them from the list, and finally
            // insert ourself so that future recvs to this broadcast can find us.
            // If we find nothig, then we just insert ourself.
            CharmMsg * msg = NULL;
            QList<CharmMsg *> * candidates = unmatched_recvs->at(my_pe)->value(event);
            QList<CharmMsg *> toremove = QList<CharmMsg *>();
            if (candidates)
            {
                for (QList<CharmMsg *>::Iterator candidate = candidates->begin();
                     candidate != candidates->end(); ++candidate)
                {
                    if ((*candidate)->entry == entry)
                    {
                        (*candidate)->sendtime = time;
                        (*candidate)->send_evt = evt;
                        evt->charmmsgs->append(*candidate);
                        toremove.append(*candidate);
                    }
                }
            }

            if (toremove.size() > 0) // Found!
            {
                for (QList<CharmMsg *>::Iterator rmsg = toremove.begin();
                     rmsg != toremove.end(); ++rmsg)
                {
                    candidates->removeOne(*rmsg);
                }
            }
            else
            {
                msg = new CharmMsg(mtype, msglen, my_pe, entry, event, pe);
                if (!sends->at(my_pe)->contains(event))
                {
                    sends->at(my_pe)->insert(event, new QList<CharmMsg *>());
                }
                sends->at(my_pe)->value(event)->append(msg);
                //messages->append(msg); <-- only append from recv side
                msg->sendtime = time;
                msg->send_evt = evt;
            }

            if (verbose)
            {
                std::cout << "CREATE!" << " on pe " << my_pe << " to " << pe << " at " << time;
                std::cout << " with event " << event << " my array " << arrayid << " and send_end index array " << send_end->index.array;
                std::cout << " for " << chares->value(entries->value(entry)->chare)->name.toStdString().c_str();
                std::cout << "::" << entries->value(entry)->name.toStdString().c_str();
                if (last)
                    std::cout << " with index " << last->index.toVerboseString().toStdString().c_str() << std::endl;
                else
                    std::cout << std::endl;
            }
        }
        else // True CREATION_BCAST / CREATION_MULTICAST
        {
            if (verbose)
            {
                std::cout << "BCAST/MULTICAST!" << " on pe " << pe;
                std::cout << " with event " << event;
                std::cout << " for " << chares->value(entries->value(entry)->chare)->name.toStdString().c_str();
                std::cout << "::" << entries->value(entry)->name.toStdString().c_str();
                if (last)
                    std::cout << " with index " << last->index.toVerboseString().toStdString().c_str();
                std::cout << " at " << time << std::endl;
            }
            /*for (int i = 0; i < numpes; i++)
            {
                CharmMsg * msg = new CharmMsg(mtype, msglen, pe, entry, event, my_pe);
                msg->sendtime = time;
                sends->append(msg);
                unmatched_sends->at(pe)->append(msg);
            }*/
        }

        if (time > traceEnd)
            traceEnd = time;
    }
    else if (rectype == BEGIN_PROCESSING)
    {
        // A receive immediately followed by a function
        mtype = lineList.at(1).toInt();
        entry = lineList.at(2).toInt();
        time = lineList.at(3).toLong();
        event = lineList.at(4).toInt();
        pe = lineList.at(5).toInt(); // Should be the senders pe
        index = 6;
        msglen = -1;
        if (version >= 2.0)
        {
            msglen = lineList.at(index).toLong();
            index++;
        }
        if (version >= 4.0)
        {
            recvTime = lineList.at(index).toLong();
            index++;
            for (int i = 0; i < 3; i++)
            {
                id.index[i] = lineList.at(index).toInt();
                index++;
            }
        }
        if (version >= 7.0)
        {
            id.index[3] = lineList.at(index).toInt();
            index++;
        }
        if (version >= 6.5)
        {
            cpuStart = lineList.at(index).toLong();
            index++;
        }
        if (version >= 6.6)
        {
            // PerfCount stuff... skip for now.
            if (hasPAPI)
                for (int j = 0; j < numPAPI; j++)
                    index++;
        }
        if (version >= 7.0 && lineList.size() > index)
        {
            arrayid = lineList.at(index).toInt();
            index++;

            // Special case now that CkReductionMgr has the wrong array id for some reason
            int chare = entries->value(entry)->chare;
            if (chare == reductionChare || chare == traceChare || chare == ckArrayChare)
                arrayid = 0;

            if (arrayid > 0)
            {
                if (!arrays->contains(arrayid))
                    arrays->insert(arrayid,
                                   new ChareArray(arrayid,
                                                  entries->value(entry)->chare));

                id.array = arrayid;
                id.chare = entries->value(entry)->chare;
                arrays->value(arrayid)->indices->insert(id);
            }
            else
            {
                if (!groups->contains(entries->value(entry)->chare))
                    groups->insert(entries->value(entry)->chare,
                                   new ChareGroup(entries->value(entry)->chare));
                groups->value(entries->value(entry)->chare)->pes.insert(my_pe);
            }
        }

        // recvMsg or addContribution we'll close off
        if (!charm_stack->isEmpty()
            && (charm_stack->top()->entry == recvMsg
                || charm_stack->top()->entry == addContribution)
            && (entry == recvMsg || entry == addContribution))
        {
            CharmEvt * front = charm_stack->pop();
            CharmEvt * back = new CharmEvt(front->entry, time, my_pe,
                                           front->chare, front->arrayid,
                                           false);
            charm_events->at(my_pe)->append(back);
        }

        CharmEvt * evt = new CharmEvt(entry, time, my_pe,
                                      entries->value(entry)->chare, arrayid,
                                      true);
        id.chare = evt->chare;
        evt->index = id;
        chares->value(evt->chare)->indices->insert(evt->index);
        charm_events->at(my_pe)->append(evt);

        seen_chares.insert(chares->value(entries->value(entry)->chare)->name
                            + "::" + entries->value(entry)->name);

        last = evt;

        charm_stack->push(evt);

        if (verbose)
        {
            std::cout << "BEGIN!" << " on pe " << my_pe << " at " << time;
            std::cout << " with event " << event << " my array " << arrayid;
            std::cout << " for " << chares->value(entries->value(entry)->chare)->name.toStdString().c_str();
            std::cout << "::" << entries->value(entry)->name.toStdString().c_str();
            std::cout << " with index " << id.toVerboseString().toStdString().c_str() << std::endl;
        }

        CharmMsg * msg = NULL;
        if (pe > my_pe) // We get send later
        {
            msg = new CharmMsg(mtype, msglen, pe, entry, event, my_pe);
            if (!unmatched_recvs->at(pe)->contains(event))
            {
                unmatched_recvs->at(pe)->insert(event, new QList<CharmMsg *>());
            }
            unmatched_recvs->at(pe)->value(event)->append(msg);
            messages->append(msg);

        } else { // Send already exists

            QList<CharmMsg *> * candidates = sends->at(pe)->value(event);
            CharmMsg * send_candidate = NULL;
            if (candidates) // May be missing some send events due to runtime collection
            {
                for (QList<CharmMsg *>::Iterator candidate = candidates->begin();
                     candidate != candidates->end(); ++candidate)
                {
                    if ((*candidate)->entry == entry)
                    {
                        send_candidate = *candidate;
                        break;
                    }
                }
            }
            if (send_candidate)
            {
                // Copy info as needed from the candidate
                msg = new CharmMsg(mtype, msglen, pe, entry, event, my_pe);
                messages->append(msg);
                msg->send_evt = send_candidate->send_evt;
                msg->sendtime = send_candidate->sendtime;
                send_candidate->send_evt->charmmsgs->append(msg);
            }
        }
        if (msg)
        {
            // +1 to make sort properly
            // Note only the enter needs the message, as that's where we look for it
            evt = new CharmEvt(RECV_FXN, time, my_pe,
                               entries->value(entry)->chare, arrayid,
                               true);
            evt->index = id;
            charm_events->at(my_pe)->append(evt);

            CharmEvt * recv_end = new CharmEvt(RECV_FXN, time+1, my_pe,
                                               entries->value(entry)->chare,
                                               arrayid, false);
            recv_end->index = id;
            charm_events->at(my_pe)->append(recv_end);

            msg->recvtime = time;
            msg->recv_evt = evt;
            evt->charmmsgs->append(msg);

            if (verbose)
            {
                std::cout << "RECV!" << " on pe " << my_pe << " from " << pe;
                std::cout << " with event " << event << " my array " << arrayid;
                std::cout << " for " << chares->value(entries->value(entry)->chare)->name.toStdString().c_str();
                std::cout << "::" << entries->value(entry)->name.toStdString().c_str();
                std::cout << " with index " << id.toVerboseString().toStdString().c_str() << std::endl;
            }
        }
        else
        {
            if (verbose)
            {
                std::cout << "NO MSG FOR RECV!!!" << " on pe " << my_pe << " was expecting message from ";
                std::cout << pe << " with event " << event;
                std::cout << " for " << chares->value(entries->value(entry)->chare)->name.toStdString().c_str();
                std::cout << "::" << entries->value(entry)->name.toStdString().c_str();
                std::cout << " with index " << id.toVerboseString().toStdString().c_str() << std::endl;
            }
        }

        if (time > traceEnd)
            traceEnd = time;
    }
    else if (rectype == END_PROCESSING)
    {
        // End of function
        mtype = lineList.at(1).toInt();
        entry = lineList.at(2).toInt();
        time = lineList.at(3).toLong();
        event = lineList.at(4).toInt();
        pe = lineList.at(5).toInt();
        index = 6;
        msglen = -1;
        if (version >= 2.0)
        {
            msglen = lineList.at(index).toLong();
            index++;
        }
        if (version >= 6.5)
        {
            cpuEnd = lineList.at(index).toLong();
            index++;
        }
        if (version >= 6.6)
        {
            // PerfCount stuff... skip for now.
            if (hasPAPI)
                for (int j = 0; j < numPAPI; j++)
                    index++;
        }
        if (version >= 7.0 && lineList.size() > index)
        {
            arrayid = lineList.at(index).toInt();
            index++;

            if (arrayid > 0 && !arrays->contains(arrayid))
            {
                arrays->insert(arrayid,
                               new ChareArray(arrayid,
                                              entries->value(entry)->chare));
            }
        }

        // Handle unfinished inside requests
        CharmEvt * front = NULL;
        bool found = false;
        // Handle unfinished inside requests
        while (!charm_stack->isEmpty() && !found)
        {
            front = charm_stack->pop();
            if (front->chare != entries->value(entry)->chare
                || front->entry != entry)
            {
                CharmEvt * back = new CharmEvt(front->entry, time, my_pe,
                                               front->chare, front->arrayid,
                                               false);
                charm_events->at(my_pe)->append(back);
            }
            else
            {
                found = true;
            }
        }

        CharmEvt * evt = new CharmEvt(entry, time, my_pe,
                                      entries->value(entry)->chare,
                                      arrayid, false);
        if (last) // Last may not exist since we can begin recording at a function end
            evt->index = last->index;
        evt->index.chare = entries->value(entry)->chare;
        charm_events->at(my_pe)->append(evt);

        // Close off others
        if (entry == addContribution || entry == recvMsg)
        {
            while (!charm_stack->isEmpty()
                   && (charm_stack->top()->entry == addContribution
                       || charm_stack->top()->entry == recvMsg))
            {
                front = charm_stack->pop();
                if (front->chare != entries->value(entry)->chare
                    || front->entry != entry)
                {
                    CharmEvt * back = new CharmEvt(front->entry, time, my_pe,
                                                   front->chare, front->arrayid,
                                                   false);
                    charm_events->at(my_pe)->append(back);
                }
            }
        }

        if (verbose)
        {
            std::cout << "END!" << " on pe " << my_pe;
            std::cout << " with event " << event << " my array " << arrayid;
            std::cout << " for " << chares->value(entries->value(entry)->chare)->name.toStdString().c_str();
            std::cout << "::" << entries->value(entry)->name.toStdString().c_str();
            std::cout << " with index " << evt->index.toVerboseString().toStdString().c_str() << std::endl;
        }

        last = NULL;

        if (time > traceEnd)
            traceEnd = time;
    }
    else if (rectype == BEGIN_IDLE)
    {
        // Beginning of Idleness
        time = lineList.at(1).toLong();
        pe = lineList.at(2).toInt();

        CharmEvt * evt = new CharmEvt(IDLE_FXN, time, pe,
                                      0, 0, true);

        charm_events->at(pe)->append(evt);

        if (verbose)
        {
            std::cout << "BEGIN IDLE" << " on pe " << my_pe << " at " << time << std::endl;
        }

        last = NULL;

        if (time > traceEnd)
            traceEnd = time;
    }
    else if (rectype == END_IDLE)
    {
        // End of Idleness
        time = lineList.at(1).toLong();
        pe = lineList.at(2).toInt();

        CharmEvt * evt = new CharmEvt(IDLE_FXN, time, pe,
                                      0, 0, false);

        charm_events->at(pe)->append(evt);

        if (verbose)
        {
            std::cout << "END IDLE" << " on pe " << my_pe << " at " << time << std::endl;
        }

        last = NULL;

        if (time > traceEnd)
            traceEnd = time;
    }
    else if (rectype == END_TRACE)
    {
        time = lineList.at(1).toLong();
        if (time > traceEnd)
            traceEnd = time;
    }
    else if (rectype == MESSAGE_RECV && verbose) // just in case we ever find one
    {     
        std::cout << "Message Recv!" << std::endl;
    }
    else if ((rectype < 14 || rectype > 19) && rectype != 6 && rectype != 7
             && rectype != 11 && rectype != 12 && verbose)
    {
        std::cout << "Event of type " << rectype << " spotted!" << std::endl;
    }


}


bool CharmImporter::matchingMessages(CharmMsg * send, CharmMsg * recv)
{
    // Event is the unique ID of each message
    if (send->entry == recv->entry && send->event == recv->event
            && send->send_pe == recv->send_pe)
        return true;
    return false;
}


const int CharmImporter::SEND_FXN;
const int CharmImporter::RECV_FXN;
const int CharmImporter::IDLE_FXN;


void CharmImporter::processDefinitions()
{
    functiongroups->insert(0, "MPI");
    functiongroups->insert(1, "Entry");
    functiongroups->insert(2, "Idle");

    functions->insert(SEND_FXN, new Function("Send", 0));
    functions->insert(RECV_FXN, new Function("Recv", 0));
    functions->insert(IDLE_FXN, new Function("Idle", 2));
    for (QMap<int, Entry *>::Iterator entry = entries->begin();
         entry != entries->end(); ++entry)
    {
        int id = entry.key();
        Entry * e = entry.value();

        functions->insert(id, new Function(chares->value(entries->value(id)->chare)->name
                                           + "::" + e->name, 1,
                                           e->name));
        if (entries->value(id)->chare == main)
            functions->value(id)->isMain = true;
        if (e->name.startsWith("contribute_wrap"))
            contribute = id;
    }
    entries->insert(SEND_FXN, new Entry(0, "Send", 0));
    entries->insert(RECV_FXN, new Entry(0, "Recv", 0));
    entries->insert(IDLE_FXN, new Entry(0, "Idle", 0));
}


// STS is never gzipped
void CharmImporter::readSts(QString dataFileName)
{
    std::ifstream stsfile(dataFileName.toStdString().c_str());
    std::string line;

    while (std::getline(stsfile, line))
    {
        QStringList lineList = QString::fromStdString(line).split(" ");
        if (lineList.at(0) == "ENTRY" && lineList.at(1) == "CHARE")
        {
            int len = lineList.length();
            QString name = lineList.at(3);
            for (int i = 4; i < len - 2; i++)
                name += " " + lineList.at(i);
            entries->insert(lineList.at(2).toInt(),
                            new Entry(lineList.at(len-2).toInt(), name,
                                      lineList.at(len-1).toInt()));

            if (name.startsWith("addContribution(CkReductionMsg"))
                addContribution = lineList.at(2).toInt();
            else if (name.startsWith("RecvMsg(CkReductionMsg"))
                recvMsg = lineList.at(2).toInt();
        }
        else if (lineList.at(0) == "CHARE")
        {
            chares->insert(lineList.at(1).toInt(),
                           new Chare(lineList.at(2)));

            if (reductionChare < 0 && QString::compare(lineList.at(2), "CkReductionMgr") == 0)
                reductionChare = lineList.at(1).toInt();
            else if (ckArrayChare < 0 && QString::compare(lineList.at(2), "CkArray") == 0)
                 ckArrayChare = lineList.at(1).toInt();
            else if (main < 0 && QString::compare(lineList.at(2), "main", Qt::CaseInsensitive) == 0)
                 main = lineList.at(1).toInt();
            else if (traceChare < 0 && QString::compare(lineList.at(2), "TraceProjectionsBOC") == 0)
                 traceChare = lineList.at(1).toInt();
        }
        else if (lineList.at(0) == "VERSION")
        {
            version = lineList.at(1).toFloat();
        }
        else if (lineList.at(0) == "PROCESSORS")
        {
            processes = lineList.at(1).toInt();
        }
        else if (lineList.at(0) == "TOTAL_PAPI_EVENTS")
        {
            hasPAPI = true;
            numPAPI = lineList.at(1).toInt();
        }
    }

    stsfile.close();
}


