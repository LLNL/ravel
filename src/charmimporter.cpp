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

#include "general_util.h"

CharmImporter::CharmImporter()
    : chares(new QMap<int, Chare*>()),
      entries(new QMap<int, Entry*>()),
      version(0),
      processes(0),
      hasPAPI(false),
      numPAPI(0),
      trace(NULL),
      unmatched_recvs(NULL),
      sends(NULL),
      charm_events(NULL),
      task_events(NULL),
      messages(new QVector<CharmMsg *>()),
      tasks(new QMap<int, Task *>()),
      taskgroups(new QMap<int, TaskGroup *>()),
      functiongroups(new QMap<int, QString>()),
      functions(new QMap<int, Function *>()),
      arrays(new QMap<int, ChareArray *>()),
      chare_to_task(QMap<ChareIndex, int>()),
      last(NULL),
      last_send(NULL),
      seen_chares(QSet<QString>())
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
}

void CharmImporter::importCharmLog(QString dataFileName, OTFImportOptions * _options)
{
    readSts(dataFileName);

    unmatched_recvs = new QVector<QMap<int, QList<CharmMsg *> *> *>(processes);
    sends = new QVector<QMap<int, QList<CharmMsg *> *> *>(processes);
    charm_events = new QVector<QVector<CharmEvt *> *>(processes);
    for (int i = 0; i < processes; i++) {
        (*unmatched_recvs)[i] = new QMap<int, QList<CharmMsg *> *>();
        (*sends)[i] = new QMap<int, QList<CharmMsg *> *>();
        (*charm_events)[i] = new QVector<CharmEvt *>();
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
        readLog(path + "/" + basename + "." + QString::number(i) + suffix,
                gzflag, i);

    // At this point, I have a list of events per PE
    // Now I have to check to see what chares are actually arrays
    // and then convert these by-PE events into by-chare
    // events based on these arrays. I should also make taskgroups and stuff
    int num_tasks = makeTasks();

    // Now that we have num_tasks, create the rawtrace to hold the conversion
    trace = new Trace(num_tasks);
    trace->units = 9;
    trace->functions = functions;
    trace->functionGroups = functiongroups;
    trace->taskgroups = taskgroups;
    trace->tasks = tasks;
    trace->collective_definitions = new QMap<int, OTFCollective *>();
    trace->collectives = new QMap<unsigned long long, CollectiveRecord *>();
    //trace->counters = new QMap<unsigned int, Counter *>();
    //trace->counter_records = new QVector<QVector<CounterRecord *> *>(num_tasks);
    trace->collectiveMap = new QVector<QMap<unsigned long long, CollectiveRecord *> *>(num_tasks);
    task_events = new QVector<QVector<CharmEvt *> *>(num_tasks);
    for (int i = 0; i < num_tasks; i++) {
        (*(trace->collectiveMap))[i] = new QMap<unsigned long long, CollectiveRecord *>();
        (*(trace->events))[i] = new QVector<Event *>();
        (*(trace->roots))[i] = new QVector<Event *>();
        (*(task_events))[i] = new QVector<CharmEvt *>();
    }

    // Change per-PE stuff to per-chare stuff
    charify();

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
            if (chares->value((*event)->chare)->indices->size() <= 1)
            {
                continue;
            }

            int taskid = chare_to_task.value((*event)->index);
            (*event)->task = taskid;
            task_events->at(taskid)->append(*event);

            std::cout << "Adding ";
            if ((*event)->enter)
            {
                std::cout << " enter ";
                if ((*event)->charmmsgs->size() > 0)
                {
                    std::cout << "  this event has a msg!" << std::endl;
                }
                else
                {
                    std::cout << "  NO MSG" << std::endl;
                }
            }
            else
            {
                std::cout << " leave ";
            }
            std::cout << entries->value((*event)->entry)->name.toStdString().c_str();
            std::cout << " on index " << (*event)->index.toString().toStdString().c_str();
            std::cout << " with taskid " << taskid << std::endl;
        }
    }

    // Sort chare events in time
    for (QVector<QVector<CharmEvt *> *>::Iterator event_list
         = task_events->begin(); event_list != task_events->end();
         ++event_list)
    {
        std::cout << "Sorting !!!! " << std::endl;
        qSort((*event_list)->begin(), (*event_list)->end(), dereferencedLessThan<CharmEvt>);
        for (QVector<CharmEvt *>::Iterator ev = (*event_list)->begin();
             ev != (*event_list)->end(); ++ev)
        {
            if ((*ev)->enter)
            {
                std::cout << "Enter ";
            }
            else
            {
                std::cout << "Leave ";
            }
            std::cout << entries->value((*ev)->entry)->name.toStdString().c_str();
            std::cout << " on index " << (*ev)->index.toString().toStdString().c_str();
            std::cout << " with taskid " << (*ev)->task << std::endl;
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

void CharmImporter::buildPartitions()
{
    QStack<CharmEvt *> stack = QStack<CharmEvt *>();
    int depth, phase;
    CharmEvt * bgn = NULL;
    CommEvent * prev = NULL;
    unsigned long long endtime = 0;
    int counter = 0;

    // Go through each separately.
    for (QVector<QVector<CharmEvt *> *>::Iterator event_list = task_events->begin();
         event_list != task_events->end(); ++event_list)
    {
        stack.clear();
        depth = 0;
        phase = 0;
        prev = NULL;
        std::cout << "Now handling " << counter << std::endl;
        counter++;
        for (QVector<CharmEvt *>::Iterator evt = (*event_list)->begin();
             evt != (*event_list)->end(); ++evt)
        {
            if ((*evt)->enter)
            {
                stack.push(*evt);
                std::cout << "Enter " << entries->value((*evt)->entry)->name.toStdString().c_str() << std::endl;
                std::cout << "     Stack size: " << stack.size() << std::endl;
                depth++;
            }
            else
            {
                std::cout << "Leave " << entries->value((*evt)->entry)->name.toStdString().c_str() << std::endl;
                std::cout << "     Stack size: " << stack.size() << std::endl;
                bgn = stack.pop();
                Event * e = NULL;
                if (trace->functions->value(bgn->entry)->group == 0) // Send or Recv
                {
                    QVector<Message *> * msgs = new QVector<Message *>();
                    for (QList<CharmMsg *>::Iterator cmsg = bgn->charmmsgs->begin();
                         cmsg != bgn->charmmsgs->end(); ++cmsg)
                    {
                        std::cout << "Finding trace mesages for cmsg" << std::endl;
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
                                (*cmsg)->tracemsg->sender->comm_prev = prev;
                                if (prev)
                                    prev->comm_next = (*cmsg)->tracemsg->sender;
                                prev = (*cmsg)->tracemsg->sender;

                                makeSingletonPartition((*cmsg)->tracemsg->sender);

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

                            (*cmsg)->tracemsg->receiver->comm_prev = prev;
                            if (prev)
                                prev->comm_next = (*cmsg)->tracemsg->receiver;
                            prev = (*cmsg)->tracemsg->receiver;

                            makeSingletonPartition((*cmsg)->tracemsg->receiver);

                            e = (*cmsg)->tracemsg->receiver;
                        }
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
                    (*(trace->roots))[(*evt)->task]->append(e);

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

                (*(trace->events))[(*evt)->task]->append(e);
            } // Handle this Event
        } // Loop Event
    } // Loop Event Lists
}

int CharmImporter::makeTasks()
{
    int taskid = 0;
    for (QMap<int, Chare *>::Iterator chare = chares->begin();
         chare != chares->end(); ++chare)
    {
        if (chare.value()->indices->size() > 1)
        {
            taskgroups->insert(chare.key(), new TaskGroup(chare.key(), chare.value()->name));
            /*QList<const ChareIndex *> indices_list = QList<const ChareIndex *>();
            for (QSet<ChareIndex>::Iterator id = chare.value()->indices->begin();
                 id != chare.value()->indices->end(); ++ id)
                indices_list.append(&(*id));
                */
            QList<ChareIndex> indices_list = chare.value()->indices->toList();
            qSort(indices_list.begin(), indices_list.end());
            for (QList<ChareIndex>::Iterator index = indices_list.begin();
                 index != indices_list.end(); ++index)
            {
                Task * task = new Task(taskid, chare.value()->name + "_" + (*index).toString());
                taskgroups->value(chare.key())->tasks->append(taskid);
                tasks->insert(taskid, task);
                chare_to_task.insert(*index, taskid);
                taskid++;
            }
        }
    }
    return taskid;
}


void CharmImporter::parseLine(QString line, int my_pe)
{
    int index, mtype, entry, event, pe, arrayid;
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
        }

        CharmEvt * evt = new CharmEvt(SEND_FXN, time, pe,
                                      entries->value(entry)->chare,
                                      true);
        // We are inside a begin_processing, so the create event we have
        // here naturally has the index of the begin_processing which is
        // what last should hold. However, the send may not be meaningful
        // should its recv not exist or go to a not-kept chare, so this needs
        // to be processed later.
        if (last)
            evt->index = last->index;

        charm_events->at(pe)->append(evt);

        /*seen_chares.insert(chares->value(entries->value(entry)->chare)->name
                            + "::" + entries->value(entry)->name);

                            */
        CharmEvt * send_end = new CharmEvt(SEND_FXN, time+1, pe,
                                           entries->value(entry)->chare,
                                           false);
        charm_events->at(pe)->append(send_end);
        if (last)
            send_end->index = last->index;

        if (rectype == CREATION)
        {
            //std::cout << "Send on " << pe << " of " << entries->value(entry)->name.toStdString().c_str() << std::endl;
            //std::cout << "     Event: " << event << ", msg_type: " << mtype << std::endl;

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
                        std::cout << "Found message for send on entry " << entries->value(entry)->name.toStdString().c_str();
                        std::cout << " at " << time << " and pe " << pe << std::endl;
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
                std::cout << "Creating message from send on entry " << entries->value(entry)->name.toStdString().c_str();
                std::cout << " at " << time << " and pe " << pe << " with event " << event;
                std::cout << " for " << chares->value(entries->value(entry)->chare)->name.toStdString().c_str();
                std::cout << "::" << entries->value(entry)->name.toStdString().c_str() << std::endl;
            }
        }
        else
        {
            /*for (int i = 0; i < numpes; i++)
            {
                CharmMsg * msg = new CharmMsg(mtype, msglen, pe, entry, event, my_pe);
                msg->sendtime = time;
                sends->append(msg);
                unmatched_sends->at(pe)->append(msg);
            }*/
            std::cout << "Bcast on " << pe << " of " << chares->value(entries->value(entry)->chare)->name.toStdString().c_str();
            std::cout << "::" << entries->value(entry)->name.toStdString().c_str() << std::endl;
            std::cout << "     Event: " << event << ", msg_type: " << mtype << std::endl;

        }
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
        }

        CharmEvt * evt = new CharmEvt(entry, time, pe,
                                      entries->value(entry)->chare,
                                      true);
        id.chare = evt->chare;
        evt->index = id;
        chares->value(evt->chare)->indices->insert(evt->index);
        charm_events->at(pe)->append(evt);

        seen_chares.insert(chares->value(entries->value(entry)->chare)->name
                            + "::" + entries->value(entry)->name);

        last = evt;

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

            std::cout << "Creating message from recv on entry " << entries->value(entry)->name.toStdString().c_str();
            std::cout << " at " << time << " and pe " << pe << " with event " << event;
            std::cout << " for " << chares->value(entries->value(entry)->chare)->name.toStdString().c_str();
            std::cout << "::" << entries->value(entry)->name.toStdString().c_str() << std::endl;

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

                std::cout << "Found message for recv on entry " << entries->value(entry)->name.toStdString().c_str();
                std::cout << " at " << time << " and pe " << pe << " with event " << event;
                std::cout << " for " << chares->value(entries->value(entry)->chare)->name.toStdString().c_str();
                std::cout << "::" << entries->value(entry)->name.toStdString().c_str() << std::endl;
            }
        }
        if (msg)
        {
            // +1 to make sort properly
            // Note only the enter needs the message, as that's where we look for it
            evt = new CharmEvt(RECV_FXN, time+1, pe,
                               entries->value(entry)->chare,
                               true);
            evt->index = id;
            charm_events->at(pe)->append(evt);

            CharmEvt * recv_end = new CharmEvt(RECV_FXN, time+2, pe,
                                               entries->value(entry)->chare,
                                               false);
            recv_end->index = id;
            charm_events->at(pe)->append(recv_end);

            msg->recvtime = time;
            evt->charmmsgs->append(msg);
        }
        else
        {
            std::cout << "NO RECV MSG!!!" << " on pe " << my_pe << " was expecting message from ";
            std::cout << pe << " with event " << event;
            std::cout << " for " << chares->value(entries->value(entry)->chare)->name.toStdString().c_str();
            std::cout << "::" << entries->value(entry)->name.toStdString().c_str() << std::endl;
        }

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
        }

        CharmEvt * evt = new CharmEvt(entry, time, pe,
                                      entries->value(entry)->chare, false);
        evt->index = last->index;
        evt->index.chare = entries->value(entry)->chare;
        charm_events->at(pe)->append(evt);

    }
    else if (rectype == MESSAGE_RECV) // just in case we ever find one
    {
        std::cout << "Message Recv!" << std::endl;
    }
    else if ((rectype < 14 || rectype > 19) && rectype != 6 && rectype != 7)
    {
        std::cout << "Event of type " << rectype << " spotted!" << std::endl;
    }


}


bool CharmImporter::matchingMessages(CharmMsg * send, CharmMsg * recv)
{
    // Event is the unique ID of each message
    std::cout << "Matching message " << send->entry << " vs " << recv->entry;
    std::cout << "  and  " << send->event << " vs " << recv->event;
    std::cout << "  and  " << send->send_pe << " vs " << recv->send_pe << std::endl;
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


