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
      version(0),
      processes(0),
      hasPAPI(false),
      numPAPI(0),
      traceEnd(0),
      trace(NULL),
      unmatched_recvs(NULL),
      sends(NULL),
      charm_stack(new QStack<CharmEvt *>()),
      charm_events(NULL),
      task_events(NULL),
      pe_events(NULL),
      //roots(NULL),
      charm_p2ps(NULL),
      messages(new QVector<CharmMsg *>()),
      primaries(new QMap<int, PrimaryTaskGroup *>()),
      taskgroups(new QMap<int, TaskGroup *>()),
      functiongroups(new QMap<int, QString>()),
      functions(new QMap<int, Function *>()),
      arrays(new QMap<int, ChareArray *>()),
      chare_to_task(QMap<ChareIndex, int>()),
      last(NULL),
      last_send(NULL),
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
}

void CharmImporter::importCharmLog(QString dataFileName, OTFImportOptions * _options)
{
    readSts(dataFileName);

    unmatched_recvs = new QVector<QMap<int, QList<CharmMsg *> *> *>(processes);
    sends = new QVector<QMap<int, QList<CharmMsg *> *> *>(processes);
    charm_events = new QVector<QVector<CharmEvt *> *>(processes);
    pe_events = new QVector<QVector<Event *> *>(processes);
    //roots = new QVector<QVector<Event *> *>(processes);
    for (int i = 0; i < processes; i++) {
        (*unmatched_recvs)[i] = new QMap<int, QList<CharmMsg *> *>();
        (*sends)[i] = new QMap<int, QList<CharmMsg *> *>();
        (*charm_events)[i] = new QVector<CharmEvt *>();
        (*pe_events)[i] = new QVector<Event *>();
        //(*roots)[i] = new QVector<Event *>();
    }

    processDefinitions();

    // Read individual log files
    QFileInfo file_info = QFileInfo(dataFileName);
    QDir directory = file_info.dir();
    QString basename = file_info.baseName();
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
    //charify();
    makeTaskEvents();

    // Build partitions
    buildPartitions();

    // Keep a list of what chares have been seen
    QList<QString> tmp = seen_chares.toList();
    qSort(tmp.begin(), tmp.end());
    for (QList<QString>::Iterator ch = tmp.begin(); ch != tmp.end(); ++ch)
    {
        std::cout << "Seen chare: " << (*ch).toStdString().c_str() << std::endl;
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

    for (QMap<ChareIndex, int>::Iterator map = chare_to_task.begin();
         map != chare_to_task.end(); ++map)
    {
        std::cout << map.key().toVerboseString().toStdString().c_str() << " <---> " << map.value() << std::endl;
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
                std::cout << "    Enter stack " << " on pe " << (*evt)->pe << " my array " << (*evt)->arrayid;
                std::cout << " for " << chares->value(entries->value((*evt)->entry)->chare)->name.toStdString().c_str();
                std::cout << "::" << entries->value((*evt)->entry)->name.toStdString().c_str();
                std::cout << " with index " << (*evt)->index.toVerboseString().toStdString().c_str() << std::endl;

                // Enter is the only place with the true array id so we must
                // get the task id here.
                if (have_arrays)
                {
                    if ((*evt)->arrayid == 0 && !application_chares.contains((*evt)->chare))
                    {
                        (*evt)->task = -1;
                        std::cout << "           Setting task to neg 1" << std::endl;
                    }
                    else // Should get main if nothing else works
                    {
                        (*evt)->task = chare_to_task.value((*evt)->index);
                        std::cout << "           Setting task to " << chare_to_task.value((*evt)->index) << std::endl;
                    }
                }
                else
                {
                    if (chares->value((*evt)->chare)->indices->size() <= 1)
                        (*evt)->task = -1;
                    else
                        (*evt)->task = chare_to_task.value((*evt)->index);
                }

                std::cout << "                    Task is now " << (*evt)->task << std::endl;

                stack->push(*evt);
                depth++;
            } // Handle enter stuff
            else
            {
                bgn = stack->pop();
                depth = makeTaskEventsPop(stack, bgn, (*evt)->time, phase, depth);
                /*Event * e = NULL;

                std::cout << "    Pop stack " << " on pe " << (*evt)->pe << " my array " << (*evt)->arrayid;
                std::cout << " for " << chares->value(entries->value((*evt)->entry)->chare)->name.toStdString().c_str();
                std::cout << "::" << entries->value((*evt)->entry)->name.toStdString().c_str();
                std::cout << " with index " << (*evt)->index.toString().toStdString().c_str() << std::endl;

                if (trace->functions->value(bgn->entry)->group == 0) // Send or Recv
                {
                    // We need to go and wire up all the mesages appropriately
                    // Then we need to make sure these get sorted in the right
                    // place for charm messages so we can make partitions out of them appropriately.
                    // Note this has to happen after the fact so we can sort appropriately
                    if (bgn->task < 0)
                    {
                        continue;
                    }
                    QVector<Message *> * msgs = new QVector<Message *>();
                    for (QList<CharmMsg *>::Iterator cmsg = bgn->charmmsgs->begin();
                         cmsg != bgn->charmmsgs->end(); ++cmsg)
                    {
                        if ((*cmsg)->tracemsg)
                        {
                            msgs->append((*cmsg)->tracemsg);
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
                                                                         (*evt)->time-1,
                                                                         bgn->entry,
                                                                         bgn->task,
                                                                         bgn->pe,
                                                                         phase,
                                                                         msgs);
                                (*cmsg)->send_evt->trace_evt = (*cmsg)->tracemsg->sender;
                                charm_p2ps->at(bgn->task)->append((*cmsg)->tracemsg->sender);

                                e = (*cmsg)->tracemsg->sender;

                            }
                            else
                            {
                                (*cmsg)->tracemsg->sender = (*cmsg)->send_evt->trace_evt;
                                // We have already set e here.
                            }

                        }
                        else if (bgn->entry == RECV_FXN)
                        {
                            (*cmsg)->tracemsg->receiver = new P2PEvent(bgn->time-1,
                                                                       (*evt)->time-2,
                                                                       bgn->entry,
                                                                       bgn->task,
                                                                       bgn->pe,
                                                                       phase,
                                                                       msgs);

                            (*cmsg)->tracemsg->receiver->is_recv = true;
                            charm_p2ps->at(bgn->task)->append((*cmsg)->tracemsg->receiver);

                            e = (*cmsg)->tracemsg->receiver;
                        }
                    }
                    if (!e) // No messages were recorded with this, skip for now and don't include
                    {
                        continue;
                    }
                }
                else // Non-comm event
                {
                    e = new Event(bgn->time, (*evt)->time, bgn->entry,
                                  bgn->task, bgn->pe);
                }

                depth--;
                e->depth = depth;
                if (depth == 0)
                    (*(trace->roots))[(*evt)->pe]->append(e);

                if (e->exit > endtime)
                    endtime = e->exit;
                if (!stack.isEmpty())
                {
                    stack.top()->children->append(e);
                }
                for (QList<Event *>::Iterator child = bgn->children->begin();
                     child != bgn->children->end(); ++child)
                {
                    e->callees->append(*child);
                    (*child)->caller = e;
                }

                (*(trace->pe_events))[(*evt)->pe]->append(e);
                */
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

    std::cout << "    Pop stack " << " on pe " << bgn->pe << " my array " << bgn->arrayid;
    std::cout << " for " << chares->value(entries->value(bgn->entry)->chare)->name.toStdString().c_str();
    std::cout << "::" << entries->value(bgn->entry)->name.toStdString().c_str();
    std::cout << " with index " << bgn->index.toString().toStdString().c_str() << std::endl;

    if (trace->functions->value(bgn->entry)->group == 0) // Send or Recv
    {
        // We need to go and wire up all the mesages appropriately
        // Then we need to make sure these get sorted in the right
        // place for charm messages so we can make partitions out of them appropriately.
        // Note this has to happen after the fact so we can sort appropriately
        if (bgn->task < 0)
        {
            return depth;
        }
        QVector<Message *> * msgs = new QVector<Message *>();
        for (QList<CharmMsg *>::Iterator cmsg = bgn->charmmsgs->begin();
             cmsg != bgn->charmmsgs->end(); ++cmsg)
        {
            if ((*cmsg)->tracemsg)
            {
                msgs->append((*cmsg)->tracemsg);
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
                                                             endtime-1,
                                                             bgn->entry,
                                                             bgn->task,
                                                             bgn->pe,
                                                             phase,
                                                             msgs);
                    (*cmsg)->send_evt->trace_evt = (*cmsg)->tracemsg->sender;
                    charm_p2ps->at(bgn->task)->append((*cmsg)->tracemsg->sender);
                    std::cout << "                      Adding" << std::endl;

                    e = (*cmsg)->tracemsg->sender;

                }
                else
                {
                    (*cmsg)->tracemsg->sender = (*cmsg)->send_evt->trace_evt;
                    // We have already set e here.
                }

            }
            else if (bgn->entry == RECV_FXN)
            {
                (*cmsg)->tracemsg->receiver = new P2PEvent(bgn->time-1,
                                                           endtime-2,
                                                           bgn->entry,
                                                           bgn->task,
                                                           bgn->pe,
                                                           phase,
                                                           msgs);

                (*cmsg)->tracemsg->receiver->is_recv = true;
                charm_p2ps->at(bgn->task)->append((*cmsg)->tracemsg->receiver);
                std::cout << "                      Adding" << std::endl;

                e = (*cmsg)->tracemsg->receiver;
            }
        }
        if (!e) // No messages were recorded with this, skip for now and don't include
        {
            return depth;
        }
    }
    else // Non-comm event
    {
        e = new Event(bgn->time, endtime, bgn->entry,
                      bgn->task, bgn->pe);
    }

    depth--;
    e->depth = depth;
    if (depth == 0)
        (*(trace->roots))[bgn->pe]->append(e);

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

// Convert per-PE charm Events into per-chare events
void CharmImporter::charify()
{
    // Convert PE events into Chare events
    for (QVector<QVector<CharmEvt *> *>::Iterator event_list = charm_events->begin();
         event_list != charm_events->end(); ++event_list)
    {
        for (QVector<CharmEvt *>::Iterator event = (*event_list)->begin();
             event != (*event_list)->end(); ++event)
        {
            // Skip if the chare doesn't map to a task event
            if ((*event)->arrayid == 0 // we have array ids
                && chares->value((*event)->chare)->indices->size() <= 1) // we don't
            {
                continue;
            }

            int taskid = chare_to_task.value((*event)->index);
            (*event)->task = taskid;
            task_events->at(taskid)->append(*event);
        }
    }

    // Sort chare events in time
    for (QVector<QVector<CharmEvt *> *>::Iterator event_list
         = task_events->begin(); event_list != task_events->end();
         ++event_list)
    {
        qSort((*event_list)->begin(), (*event_list)->end(), dereferencedLessThan<CharmEvt>);
    }
}

void CharmImporter::buildPartitions()
{
    P2PEvent * prev = NULL;
    for (QVector<QVector<P2PEvent *> *>::Iterator p2plist = charm_p2ps->begin();
         p2plist != charm_p2ps->end(); ++p2plist)
    {
        prev = NULL;
        for (QVector<P2PEvent *>::Iterator p2p = (*p2plist)->begin();
             p2p != (*p2plist)->end(); ++p2p)
        {
            if (prev)
                prev->comm_next = *p2p;
            (*p2p)->comm_prev = prev;
            prev = *p2p;

            makeSingletonPartition(*p2p);
            trace->events->at((*p2p)->task)->append(*p2p);
        }
    }
}


void CharmImporter::makeSingletonPartition(CommEvent * evt)
{
    Partition * p = new Partition();
    p->addEvent(evt);
    evt->partition = p;
    p->new_partition = p;
    trace->partitions->append(p);
}


int CharmImporter::makeTasks()
{
    // Setup main task
    primaries->insert(0, new PrimaryTaskGroup(0, "main"));
    Task * mainTask = new Task(0, "main", primaries->value(0));
    primaries->value(0)->tasks->append(mainTask);
    chare_to_task.insert(ChareIndex(main, 0, 0, 0, 0), 0);
    application_chares.insert(main);

    int taskid = 1;
    if (arrays->size() > 0)
    {
        for (QMap<int, ChareArray *>::Iterator array = arrays->begin();
             array != arrays->end(); ++array)
        {
            std::cout << "Adding " << chares->value(array.value()->chare)->name.toStdString().c_str() << " with num indices " << array.value()->indices->size() << std::endl;
            primaries->insert(array.key(),
                              new PrimaryTaskGroup(array.key(),
                                                   chares->value(array.value()->chare)->name
                                                   + "_" + QString::number(array.key())));
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
                primaries->last()->tasks->append(task);
                chare_to_task.insert(*index, taskid);
                std::cout << "Index " << (*index).toString().toStdString().c_str() << " maps to task " << chare_to_task.value(*index) << std::endl;
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
                    primaries->last()->tasks->append(task);
                    std::cout << "Index " << (*index).toString().toStdString().c_str() << " maps to task " << taskid << std::endl;
                    chare_to_task.insert(*index, taskid);
                    taskid++;
                }

                application_chares.insert(chare.key());
            }
        }
    }
    return taskid;
}


void CharmImporter::parseLine(QString line, int my_pe)
{
    int index, mtype, entry, event, pe;
    int arrayid = 0;
    ChareIndex id = ChareIndex(-1, 0,0,0,0);
    long time, msglen, sendTime, recvTime, numpes, cpuStart, cpuEnd;

    //std::cout << line.toStdString().c_str() << std::endl;
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

            if (arrayid > 0 && !arrays->contains(arrayid))
            {
                arrays->insert(arrayid,
                               new ChareArray(arrayid,
                                              entries->value(entry)->chare));
            }
        }

        if (last)
            arrayid = last->index.array;

        CharmEvt * evt = new CharmEvt(SEND_FXN, time, pe,
                                      entries->value(entry)->chare, arrayid,
                                      true);
        // We are inside a begin_processing, so the create event we have
        // here naturally has the index of the begin_processing which is
        // what last should hold. However, the send may not be meaningful
        // should its recv not exist or go to a not-kept chare, so this needs
        // to be processed later.
        if (last)
            evt->index = last->index;
        else
            evt->index = ChareIndex(main, 0, 0, 0, 0);

        charm_events->at(pe)->append(evt);

        /*seen_chares.insert(chares->value(entries->value(entry)->chare)->name
                            + "::" + entries->value(entry)->name);

                            */
        CharmEvt * send_end = new CharmEvt(SEND_FXN, time+1, pe,
                                           entries->value(entry)->chare, arrayid,
                                           false);
        charm_events->at(pe)->append(send_end);
        if (last)
            send_end->index = last->index;
        else
            evt->index = ChareIndex(main, 0, 0, 0, 0);

        if (rectype == CREATION)
        {
            // Look for our message -- note this send may actually be a
            // broadcast. If that's the case, we weant to find all matches,
            // process them, and then remove them from the list, and finally
            // insert ourself so that future recvs to this broadcast can find us.
            // If we find nothig, then we just insert ourself.
            CharmMsg * msg = NULL;
            QList<CharmMsg *> * candidates = unmatched_recvs->at(pe)->value(event);
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
                msg = new CharmMsg(mtype, msglen, pe, entry, event, my_pe);
                if (!sends->at(pe)->contains(event))
                {
                    sends->at(pe)->insert(event, new QList<CharmMsg *>());
                }
                sends->at(pe)->value(event)->append(msg);
                //messages->append(msg); <-- only append from recv side
                msg->sendtime = time;
                msg->send_evt = evt;
            }
            std::cout << "CREATE!" << " on pe " << pe << " at " << time;
            std::cout << " with event " << event << " my array " << arrayid << " and last index array " << send_end->index.array;
            std::cout << " for " << chares->value(entries->value(entry)->chare)->name.toStdString().c_str();
            std::cout << "::" << entries->value(entry)->name.toStdString().c_str();
            if (last)
                std::cout << " with index " << last->index.toString().toStdString().c_str() << std::endl;
            else
                std::cout << std::endl;
        }
        else // True CREATION_BCAST / CREATION_MULTICAST
        {
            std::cout << "BCAST/MULTICAST!" << " on pe " << pe;
            std::cout << " with event " << event;
            std::cout << " for " << chares->value(entries->value(entry)->chare)->name.toStdString().c_str();
            std::cout << "::" << entries->value(entry)->name.toStdString().c_str();
            if (last)
                std::cout << " with index " << last->index.toString().toStdString().c_str() << std::endl;
            else
                std::cout << std::endl;
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

        std::cout << "BEGIN!" << " on pe " << my_pe << " at " << time;
        std::cout << " with event " << event << " my array " << arrayid;
        std::cout << " for " << chares->value(entries->value(entry)->chare)->name.toStdString().c_str();
        std::cout << "::" << entries->value(entry)->name.toStdString().c_str();
        std::cout << " with index " << id.toString().toStdString().c_str() << std::endl;

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
            evt = new CharmEvt(RECV_FXN, time+1, my_pe,
                               entries->value(entry)->chare, arrayid,
                               true);
            evt->index = id;
            charm_events->at(my_pe)->append(evt);

            CharmEvt * recv_end = new CharmEvt(RECV_FXN, time+2, my_pe,
                                               entries->value(entry)->chare,
                                               arrayid, false);
            recv_end->index = id;
            charm_events->at(my_pe)->append(recv_end);

            msg->recvtime = time;
            msg->recv_evt = evt;
            evt->charmmsgs->append(msg);

            std::cout << "RECV!" << " on pe " << my_pe << " from " << pe;
            std::cout << " with event " << event << " my array " << arrayid;
            std::cout << " for " << chares->value(entries->value(entry)->chare)->name.toStdString().c_str();
            std::cout << "::" << entries->value(entry)->name.toStdString().c_str();
            std::cout << " with index " << id.toString().toStdString().c_str() << std::endl;
        }
        else
        {
            std::cout << "NO RECV MSG!!!" << " on pe " << my_pe << " was expecting message from ";
            std::cout << pe << " with event " << event;
            std::cout << " for " << chares->value(entries->value(entry)->chare)->name.toStdString().c_str();
            std::cout << "::" << entries->value(entry)->name.toStdString().c_str();
            std::cout << " with index " << id.toString().toStdString().c_str() << std::endl;
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

        CharmEvt * evt = new CharmEvt(entry, time, my_pe,
                                      entries->value(entry)->chare,
                                      arrayid, false);
        evt->index = last->index;
        evt->index.chare = entries->value(entry)->chare;
        charm_events->at(my_pe)->append(evt);

        std::cout << "END!" << " on pe " << my_pe;
        std::cout << " with event " << event << " my array " << arrayid;
        std::cout << " for " << chares->value(entries->value(entry)->chare)->name.toStdString().c_str();
        std::cout << "::" << entries->value(entry)->name.toStdString().c_str();
        std::cout << " with index " << last->index.toString().toStdString().c_str() << std::endl;

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
    else if (rectype == MESSAGE_RECV) // just in case we ever find one
    {
        std::cout << "Message Recv!" << std::endl;
    }
    else if ((rectype < 14 || rectype > 19) && rectype != 6 && rectype != 7
             && rectype != 11 && rectype != 12)
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

void CharmImporter::processDefinitions()
{
    functiongroups->insert(0, "MPI");
    functiongroups->insert(1, "Entry");

    functions->insert(SEND_FXN, new Function("Send", 0));
    functions->insert(RECV_FXN, new Function("Recv", 0));
    for (QMap<int, Entry *>::Iterator entry = entries->begin();
         entry != entries->end(); ++entry)
    {
        int id = entry.key();
        Entry * e = entry.value();

        functions->insert(id, new Function(e->name, 1));
    }
    entries->insert(SEND_FXN, new Entry(0, "Send", 0));
    entries->insert(RECV_FXN, new Entry(0, "Recv", 0));
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
        }
        else if (lineList.at(0) == "CHARE")
        {
            chares->insert(lineList.at(1).toInt(),
                           new Chare(lineList.at(2)));
            if (QString::compare(lineList.at(2), "main", Qt::CaseInsensitive) == 0)
                main = lineList.at(1).toInt();
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


