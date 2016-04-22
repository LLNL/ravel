#include "charmimporter.h"
#include "eventrecord.h"
#include "commrecord.h"
#include "rpartition.h"
#include "commevent.h"
#include "message.h"
#include "p2pevent.h"
#include "event.h"
#include "entitygroup.h"
#include <iostream>
#include <QStringList>
#include <QDir>
#include <QFileInfo>
#include <QStack>
#include <zlib.h>
#include <sstream>
#include <fstream>
#include "trace.h"
#include "entity.h"
#include "entitygroup.h"
#include "function.h"
#include "message.h"
#include "p2pevent.h"
#include "commevent.h"
#include "event.h"
#include "importoptions.h"
#include "primaryentitygroup.h"
#include "metrics.h"

#include "ravelutils.h"

CharmImporter::CharmImporter()
    : chares(new QMap<int, Chare*>()),
      entries(new QMap<int, Entry*>()),
      options(NULL),
      version(0),
      processes(0),
      hasPAPI(false),
      numPAPI(0),
      main(-1),
      forravel(-1),
      traceChare(-1),
      addContribution(new QSet<int>()),
      recvMsg(new QSet<int>()),
      reductionEntries(new QSet<int>()),
      traceEnd(0),
      num_application_entities(0),
      reduction_count(0),
      trace(NULL),
      unmatched_recvs(NULL),
      sends(NULL),
      charm_stack(new QStack<CharmEvt *>()),
      charm_events(NULL),
      entity_events(NULL),
      pe_events(NULL),
      charm_p2ps(NULL),
      pe_p2ps(NULL),
      idle_to_next(new QMap<Event *, int>()),
      messages(new QVector<CharmMsg *>()),
      primaries(new QMap<int, PrimaryEntityGroup *>()),
      entitygroups(new QMap<int, EntityGroup *>()),
      functiongroups(new QMap<int, QString>()),
      functions(new QMap<int, Function *>()),
      arrays(new QMap<int, ChareArray *>()),
      groups(new QMap<int, ChareGroup *>()),
      atomics(new QMap<int, int>()),
      reductions(new QMap<int, QMap<int, int> *>()),
      chare_to_entity(new QMap<ChareIndex, int>()),
      last(QStack<CharmEvt *>()),
      last_evt(NULL),
      last_entry(NULL),
      add_order(0),
      idles(QList<Event *>()),
      seen_chares(QSet<QString>()),
      application_chares(QSet<int>()),
      application_group_chares(QMap<int, int>())
{
}
const QString CharmImporter::idle_metric = "Idle A";
const QString CharmImporter::runtime_metric = "Unaccounted Runtime";

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
    delete messages;

    for (QMap<int, ChareArray *>::Iterator itr = arrays->begin();
         itr != arrays->end(); ++itr)
    {
        delete itr.value();
    }
    delete arrays;

    for (QMap<int, ChareGroup *>::Iterator itr = groups->begin();
         itr != groups->end(); ++itr)
    {
        delete itr.value();
    }
    delete groups;

    for (QMap<int, QMap<int, int> *>::Iterator itr = reductions->begin();
         itr != reductions->end(); ++itr)
    {
        delete itr.value();
    }
    delete reductions;

    delete charm_stack;
    delete idle_to_next;
    delete addContribution;
    delete recvMsg;
    delete atomics;
    delete chare_to_entity;
    delete reductionEntries;
}

void CharmImporter::importCharmLog(QString dataFileName, ImportOptions * _options)
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
    *reductionEntries += *addContribution;
    *reductionEntries += *recvMsg;

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
    // events based on these arrays. I should also make entitygroups and stuff
    int num_entities = makeEntities();

    // Now that we have num_entities, create the rawtrace to hold the conversion
    trace = new Trace(num_entities, processes);
    trace->num_application_entities = num_application_entities;
    trace->units = 9;
    trace->functions = functions;
    trace->functionGroups = functiongroups;
    trace->entitygroups = entitygroups;
    trace->primaries = primaries;
    trace->collective_definitions = new QMap<int, OTFCollective *>();
    trace->collectives = new QMap<unsigned long long, CollectiveRecord *>();
    trace->collectiveMap = new QVector<QMap<unsigned long long, CollectiveRecord *> *>(num_entities);
    entity_events = new QVector<QVector<CharmEvt *> *>(num_entities);
    charm_p2ps = new QVector<QVector<P2PEvent *> *>(num_entities);
    for (int i = 0; i < num_entities; i++) {
        (*(trace->collectiveMap))[i] = new QMap<unsigned long long, CollectiveRecord *>();
        (*(trace->events))[i] = new QVector<Event *>();
        (*(entity_events))[i] = new QVector<CharmEvt *>();
        (*(charm_p2ps))[i] = new QVector<P2PEvent *>();
    }

    // Change per-PE stuff to per-chare stuff
    makeEntityEvents();

    // Idle calcs
    chargeIdleness();

    // Build partitions
    QElapsedTimer partTimer;
    partTimer.start();
    buildPartitions();
    trace->totalTime += partTimer.nsecsElapsed();
    RavelUtils::gu_printTime(trace->totalTime, "Initial Partitions Time:");

    // Rename for vis/debugging
    for (int i = 0; i < trace->partitions->size(); ++i)
    {
        trace->partitions->at(i)->debug_name = i;
    }

    // Output a list of what chares have been seen
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


    for (QVector<QVector<P2PEvent *> *>::Iterator itr
         = charm_p2ps->begin(); itr != charm_p2ps->end(); ++itr)
    {
        delete *itr;
    }
    delete charm_p2ps;

    for (QVector<QVector<CharmEvt *> *>::Iterator itr
         = entity_events->begin(); itr != entity_events->end(); ++itr)
    {
        // Evts deleted in charm_events
        delete *itr;
    }
    delete entity_events;


    for (int i = 0; i < processes; i++)
    {
        delete (*pe_events)[i];
        delete (*pe_p2ps)[i];
    }

    delete pe_events;
    delete pe_p2ps;
}

// Process each log file (per PE)
void CharmImporter::readLog(QString logFileName, bool gzipped, int pe)
{
    last.clear();
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
// Will fill the pe_events and the trace->roots
// After this is done we will take the trace events and
// turn them into partitions.
void CharmImporter::makeEntityEvents()
{

    // Setup idle metrics.
    trace->metrics->append("Idle");
    (*(trace->metric_units))["Idle"] = RavelUtils::getUnits(trace->units);
    trace->metrics->append("Idle Blame");
    (*(trace->metric_units))["Idle Blame"] = RavelUtils::getUnits(trace->units);

    trace->metrics->append(idle_metric);
    (*(trace->metric_units))[idle_metric] = RavelUtils::getUnits(trace->units);
    trace->metrics->append(runtime_metric);
    (*(trace->metric_units))[runtime_metric] = RavelUtils::getUnits(trace->units);



    // Now make the entity event hierarchy with roots and such
    QStack<CharmEvt *>  * stack = new QStack<CharmEvt *>();
    int depth, phase;
    CharmEvt * bgn = NULL;
    int counter = 0;
    int atomic = -1;

    bool have_arrays = false;
    if (arrays->size() > 0)
        have_arrays = true;

    if (verbose)
    {
        for (QMap<ChareIndex, int>::Iterator map = chare_to_entity->begin();
             map != chare_to_entity->end(); ++map)
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
    // putting them into charm_p2ps and setting their entities appropriately
    for (QVector<QVector<CharmEvt *> *>::Iterator event_list = charm_events->begin();
         event_list != charm_events->end(); ++event_list)
    {
        stack->clear();
        last_evt = NULL;
        last_entry = NULL;
        depth = 0;
        phase = 0;
        counter++;
        for (QVector<CharmEvt *>::Iterator evt = (*event_list)->begin();
             evt != (*event_list)->end(); ++evt)
        {
            if ((*evt)->enter)
            {
                // Enter is the only place with the true array id so we must
                // get the entity id here.
                if (have_arrays)
                {      
                    if (((*evt)->arrayid == 0 && (!application_chares.contains((*evt)->index.chare)
                                                 || ((*evt)->index.chare == main && (*evt)->pe != 0)))
                             || (*evt)->index.chare == -1)
                    {
                        (*evt)->entity = -1;
                    }
                    else if (!stack->isEmpty()
                             && (*evt)->index.chare == main
                             && (*evt)->entry == SEND_FXN
                             && stack->top()->chare != main)
                    {
                        // This is the case where we attributed to main incorrectly since
                        // a send records its send destination chare rather than its called
                        // chare.
                        (*evt)->entity = num_application_entities + (*evt)->pe;
                    }
                    else // Should get main if nothing else works
                    {
                        (*evt)->entity = chare_to_entity->value((*evt)->index);
                    }
                }
                else
                {
                    if (chares->value((*evt)->index.chare)->indices->size() <= 1
                        && !application_chares.contains((*evt)->index.chare))
                        (*evt)->entity = -1;
                    else
                        (*evt)->entity = chare_to_entity->value((*evt)->index);
                }

                if ((*evt)->entity == -1) // runtime chare
                {
                    (*evt)->entity = num_application_entities + (*evt)->pe;
                }

                if (atomics->contains((*evt)->entry))
                {
                    atomic = atomics->value((*evt)->entry);

                    // Check to see if we have a previous entry on the same
                    // entity. That may indicate a when clause.
                    P2PEvent * last_p2p = NULL;
                    if (charm_p2ps->at((*evt)->entity)->size() > 0)
                        last_p2p = charm_p2ps->at((*evt)->entity)->last();
                    Event * prev_evt = NULL;
                    if (pe_events->at((*evt)->pe)->size() > 0)
                        prev_evt = pe_events->at((*evt)->pe)->last();
                    if (last_p2p && prev_evt && prev_evt->entity == (*evt)->entity)
                    {
                        // If these didn't have atomics set in them. We have
                        // to go through charm_p2ps because we need P2PEvents
                        int index = charm_p2ps->at((*evt)->entity)->size() - 1;
                        while (last_p2p->caller == prev_evt
                               && last_p2p->atomic < 0)
                        {
                            last_p2p->atomic = atomic;
                            index--;
                            if (index < 0)
                                break;
                            last_p2p = charm_p2ps->at((*evt)->entity)->at(index);
                        }
                    }

                }

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
                    depth = makeEntityEventsPop(stack, bgn, (*evt)->time, phase, depth, atomic);

                // Unset atomic
                if (atomics->contains(bgn->entry))
                {
                    atomic = -1;
                }
            } // Handle this Event
        } // Loop Event

        // Unfinished begins
        while (!stack->isEmpty())
            depth = makeEntityEventsPop(stack, stack->pop(), traceEnd, phase, depth, atomic);
    } // Loop Event Lists


    // Sort chare events in time
    unsigned long long num_events = 0;
    for (QVector<QVector<P2PEvent *> *>::Iterator event_list
         = charm_p2ps->begin(); event_list != charm_p2ps->end();
         ++event_list)
    {
        qSort((*event_list)->begin(), (*event_list)->end(), dereferencedLessThan<P2PEvent>);
        num_events += (*event_list)->size();
    }
    std::cout << "Total P2P events: " << num_events << std::endl;

    delete stack;
}


// When we pop an event from the event stack, we are closing some sort of function.
// We keep track of extra information, such as message relations, when we do tihs.
int CharmImporter::makeEntityEventsPop(QStack<CharmEvt *> * stack, CharmEvt * bgn,
                                     long endtime, int phase, int depth, int atomic)
{
    Event * e = NULL;

    if (trace->functions->value(bgn->entry)->group == 0) // Send or Recv
    {
        // We need to go and wire up all the mesages appropriately
        // Then we need to make sure these get sorted in the right
        // place for charm messages so we can make partitions out of them appropriately.
        // Note this has to happen after the fact so we can sort appropriately
        if (bgn->entity < 0)
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
                                                             bgn->entity,
                                                             bgn->pe,
                                                             phase,
                                                             msgs);
                    (*cmsg)->tracemsg->sender->is_recv = false;
                    (*cmsg)->tracemsg->sender->add_order = add_order;
                    add_order++;
                    (*cmsg)->send_evt->trace_evt = (*cmsg)->tracemsg->sender;
                    charm_p2ps->at(bgn->entity)->append((*cmsg)->tracemsg->sender);

                    e = (*cmsg)->tracemsg->sender;

                    pe_p2ps->at(bgn->pe)->append((*cmsg)->tracemsg->sender);
                    (*cmsg)->tracemsg->sender->metrics->addMetric("Idle", 0, 0);
                    (*cmsg)->tracemsg->sender->metrics->addMetric("Idle Blame", 0, 0);
                    (*cmsg)->tracemsg->sender->atomic = atomic;
                    (*cmsg)->tracemsg->sender->matching = bgn->associated_array;

                    if (last_evt)
                    {
                        (*cmsg)->tracemsg->sender->pe_prev = last_evt;
                        last_evt->pe_next = (*cmsg)->tracemsg->sender;
                    }
                    last_evt = (*cmsg)->tracemsg->sender;

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
                                                           bgn->entity,
                                                           bgn->pe,
                                                           phase,
                                                           msgs);

                (*cmsg)->tracemsg->receiver->is_recv = true;
                (*cmsg)->tracemsg->receiver->add_order = add_order;
                add_order++;
                charm_p2ps->at(bgn->entity)->append((*cmsg)->tracemsg->receiver);

                e = (*cmsg)->tracemsg->receiver;
                (*cmsg)->tracemsg->receiver->metrics->addMetric("Idle", 0, 0);
                (*cmsg)->tracemsg->receiver->metrics->addMetric("Idle Blame", 0, 0);
                (*cmsg)->tracemsg->receiver->atomic = atomic;
                (*cmsg)->tracemsg->receiver->matching = bgn->associated_array;

                pe_p2ps->at(bgn->pe)->append((*cmsg)->tracemsg->receiver);

                if (last_evt)
                {
                    (*cmsg)->tracemsg->receiver->pe_prev = last_evt;
                    last_evt->pe_next = (*cmsg)->tracemsg->receiver;
                }
                last_evt = (*cmsg)->tracemsg->receiver;
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
                      bgn->entity, bgn->pe);
        if (bgn->entry == IDLE_FXN)
        {
            // Index of the next comm event after this IDLE
            idle_to_next->insert(e, pe_p2ps->at(bgn->pe)->size());
        }
        else
        {
            if (last_entry)
            {
                if (last_entry->function == IDLE_FXN)
                {
                    e->metrics->addMetric(idle_metric, last_entry->exit - last_entry->enter);
                }
                e->metrics->addMetric(runtime_metric, e->enter - last_entry->exit);
            }
        }
        last_entry = e;
    }

    if (trace->functions->value(bgn->entry)->group == 2) // IDLE
        idles.append(e);

    depth--;
    e->depth = depth;
    if (depth == 0)
    {
        (*(trace->roots))[bgn->pe]->append(e);
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

    (*pe_events)[bgn->pe]->append(e);
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
            if (idle_pos >= pe_p2ps->at(idle_evt->pe)->size())
            {
                flag = false;
                break;
            }
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

                // First check if the sender was actually caused by something happening
                // after the idle. In this case we don't count it at all since it
                // wasn't contributing to the idle.
                if (sender->caller && sender->caller->enter > idle_evt->exit - 1)
                {
                    flag = false;
                }
                else
                {
                    idle_diff = sender->enter - idle_evt->enter;
                    if (sender->caller)
                    {
                        idle_diff = sender->caller->exit - idle_evt->enter;
                    }

                    if (idle_diff > 0)
                    {
                        // Let the recv collect that as well in a different metric
                        //comm_evt->metrics->setMetric("Idle", idle_diff, 0);
                        comm_evt->metrics->setMetric("Idle", idle_evt->exit - idle_evt->enter, 0);
                    }
                    else
                    {
                        flag = false;
                    }
                }
            }
            idle_pos++;
        }
    }
}

// Partitions the Charm++ events
void CharmImporter::buildPartitions()
{
    QList<P2PEvent *> * events = new QList<P2PEvent *>();

    P2PEvent * prev = NULL;
    P2PEvent * true_prev = NULL;

    // List of functions/entries we will break partitions on
    QList<QString> breakables = QList<QString>();
    if (!options->partitionByFunction && options->breakFunctions.length() > 0)
        breakables = options->breakFunctions.split(",");

    int count = 0;
    int entityid = 0;
    for (QVector<QVector<P2PEvent *> *>::Iterator p2plist = charm_p2ps->begin();
         p2plist != charm_p2ps->end(); ++p2plist)
    {
        // first make sure everything is in the absolutely right order.
        // Using the simple sorting, we could be interleaving things unnecessarily.
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

        // Now break these timelines into partitions -- week accumulate along the
        // timeline into a partition until one of the four stopping mechanisms
        // below is seen.
        for (QVector<P2PEvent *>::Iterator p2p = (*p2plist)->begin();
             p2p != (*p2plist)->end(); ++p2p)
        {
            // End previous by making it into a partition if:
            // 1) We have changed functions (not including addContribution/recvMsg)
            // 2) We are changing from app->runtime or runtime->app
            // 3) This is one of the breakable functions
            // 4) We are starting a a multicast? (probably a sign?)
            if (!events->isEmpty())
            {
                // 1)
                if ((*p2p)->caller == NULL
                    || ((prev && (*p2p)->caller != prev->caller)
                        // If they are from addContribution and recvMsg of the same chare
                        && !((*p2p)->matching == prev->matching
                             && entries->value((*p2p)->caller->function)->chare
                                == entries->value(prev->caller->function)->chare
                             && reductionEntries->contains((*p2p)->caller->function)
                             && reductionEntries->contains(prev->caller->function)
                            )
                        && (prev->atomic == -1 || (*p2p)->atomic != prev->atomic)
                       )
                    )//|| trace->functions->value((*p2p)->caller->function)->isMain) // 1)
                {
                    makePartition(events);
                    events->clear();
                }

                // 2)
                else if (prev && entityid < num_application_entities)
                {
                    // First figure out if the previous comm event was app only or not.
                    bool prev_app = true, my_app = true;
                    if (prev->is_recv)
                    {
                        if (prev->messages->first()->sender->entity >= num_application_entities)
                        {
                            prev_app = false;
                        }
                    }
                    else
                    {
                        if (prev->messages->first()->receiver->entity >= num_application_entities)
                        {
                            prev_app = false;
                        }
                    }

                    // Check if we are application only
                    if ((*p2p)->is_recv)
                    {
                        if ((*p2p)->messages->first()->sender->entity >= num_application_entities)
                        {
                            my_app = false;
                        }
                    }
                    else
                    {
                        if ((*p2p)->messages->first()->receiver->entity >= num_application_entities)
                        {
                            my_app = false;
                        }
                    }

                    // These should be the same or split.
                    if (my_app != prev_app)
                    {
                        makePartition(events);
                        events->clear();
                    }
                }

                // 3)
                if (!events->isEmpty() && !options->partitionByFunction)
                {
                    QString caller = trace->functions->value((*p2p)->caller->function)->shortname;
                    for (QList<QString>::Iterator fxn = breakables.begin();
                         fxn != breakables.end(); ++fxn)
                    {
                        if (caller.startsWith(*fxn, Qt::CaseInsensitive))
                        {
                            makePartition(events);
                            events->clear();
                            break;
                        }
                    }
                }
            }

            // comm_next / comm_prev only set within the same caller
            // as that's what we know is a true ordering constraint
            // This is true for charm++, may not be true for
            // general task-based runtimes
            if (prev && prev->caller != NULL
                && ((prev->caller == (*p2p)->caller)
                    || ((*p2p)->matching == prev->matching
                        && entries->value((*p2p)->caller->function)->chare
                           == entries->value(prev->caller->function)->chare
                        && reductionEntries->contains((*p2p)->caller->function)
                        && reductionEntries->contains(prev->caller->function)
                       )
                   )
               )
            {
                prev->comm_next = *p2p;
                (*p2p)->comm_prev = prev;
            }
            prev = *p2p;

            (*p2p)->true_prev = true_prev;
            if (true_prev)
            {
                true_prev->true_next = *p2p;
            }
            true_prev = *p2p;

            events->append(*p2p);
            trace->events->at((*p2p)->entity)->append(*p2p);
        }

        // Clear remaining in events;
        if (!events->isEmpty())
        {
            makePartition(events);
            events->clear();
        }

        entityid++;
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
        if ((*evt)->atomic >= 0)
        {
            if ((*evt)->atomic < p->min_atomic)
            {
                p->min_atomic = (*evt)->atomic;
            }
            if ((*evt)->atomic > p->max_atomic)
            {
                p->max_atomic = (*evt)->atomic;
            }
        }
    }

    if (events->first()->entity >= num_application_entities)
    {
        p->runtime = true;
    }
    p->new_partition = p;
    trace->partitions->append(p);
}


// Turn the chares we have seen into Ravel entities.
// Each specific chare is grouped by its specific chare array
int CharmImporter::makeEntities()
{
    // Setup main entity
    primaries->insert(0, new PrimaryEntityGroup(0, "main"));
    Entity * mainEntity = new Entity(0, "main", primaries->value(0));
    primaries->value(0)->entities->append(mainEntity);
    chare_to_entity->insert(ChareIndex(main, 0, 0, 0, 0), 0);
    application_chares.insert(main);

    int entityid = 1;
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
                              new PrimaryEntityGroup(array.key(),
                                                   ptg_name));
            QList<ChareIndex> indices_list = array.value()->indices->toList();
            qSort(indices_list.begin(), indices_list.end());

            for (QList<ChareIndex>::Iterator index = indices_list.begin();
                 index != indices_list.end(); ++index)
            {
                Entity * entity = new Entity(entityid,
                                       (*index).toString(),
                                       primaries->value(array.key()));
                primaries->value(array.key())->entities->append(entity);
                chare_to_entity->insert(*index, entityid);
                entityid++;
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
                                  new PrimaryEntityGroup(chare.key(),
                                                       chare.value()->name));
                QList<ChareIndex> indices_list = chare.value()->indices->toList();
                qSort(indices_list.begin(), indices_list.end());
                for (QList<ChareIndex>::Iterator index = indices_list.begin();
                     index != indices_list.end(); ++index)
                {
                    Entity * entity = new Entity(entityid,
                                           (*index).toString(),
                                           primaries->value(chare.key()));
                    primaries->value(chare.key())->entities->append(entity);
                    chare_to_entity->insert(*index, entityid);
                    entityid++;
                }

                application_chares.insert(chare.key());
            }
        }
    }

    num_application_entities = entityid;
    primaries->insert(chares->size(),
                      new PrimaryEntityGroup(chares->size(),
                                           "pe 0"));
    for (int i = 0; i < processes; i++)
    {
        Entity * entity = new Entity(entityid,
                               "pe " + QString::number(i),
                               primaries->value(chares->size()));
        primaries->value(chares->size())->entities->append(entity);
        entityid++;
    }
    return entityid;
}


// Read/store record from a line of a PE's log file
void CharmImporter::parseLine(QString line, int my_pe)
{
    int index, mtype, entry, event, pe, assoc = -1;
    int original_array, arrayid = 0;
    ChareIndex id = ChareIndex(-1, 0,0,0,0);
    long time, msglen, sendTime, recvTime, numpes, cpuStart, cpuEnd;

    QStringList lineList = line.split(" ");
    int rectype = lineList.at(0).toInt();
    if (rectype == CREATION || rectype == CREATION_BCAST || rectype == CREATION_MULTICAST)
    {
        // We don't handle messages that are not inside something.
        if (last.isEmpty())
            return;

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
            }
        }
        if (version >= 7.0 && lineList.size() > index)
        {
            arrayid = lineList.at(index).toInt();
            index++;
            original_array = arrayid;

            int chare = entries->value(entry)->chare;
            if (chare == traceChare || chare == forravel
                || application_group_chares.contains(chare))
            {
                arrayid = 0;
            }
            else if (chares->value(chare)->name.startsWith("Ck"))
            {

                if (!reductions->contains(arrayid))
                    reductions->insert(arrayid, new QMap<int, int>());
                if (!reductions->value(arrayid)->value(event))
                {
                    reductions->value(arrayid)->insert(event, reduction_count);
                    reduction_count++;
                }
                assoc = reductions->value(arrayid)->value(event);

                arrayid = 0;
            }
            if (arrayid > 0 && !arrays->contains(arrayid))
            {
                arrays->insert(arrayid,
                               new ChareArray(arrayid,
                                              entries->value(entry)->chare));
            }
        }

        if (!last.isEmpty())
        {
            arrayid = last.top()->index.array;
        }

        CharmEvt * evt = new CharmEvt(SEND_FXN, time, my_pe,
                                      entries->value(entry)->chare, arrayid,
                                      true);
        // We are inside a begin_processing, so the create event we have
        // here naturally has the index of the begin_processing which is
        // what last should hold. However, the send may not be meaningful
        // should its recv not exist or go to a not-kept chare, so this needs
        // to be processed later.
        if (!last.isEmpty())
        {
            evt->index = last.top()->index;
        }

        evt->associated_array = assoc;

        charm_events->at(my_pe)->append(evt);

        CharmEvt * send_end = new CharmEvt(SEND_FXN, time+1, my_pe,
                                           entries->value(entry)->chare, arrayid,
                                           false);
        charm_events->at(my_pe)->append(send_end);
        if (!last.isEmpty())
        {
            send_end->index = last.top()->index;
        }
        send_end->associated_array = assoc;

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
                msg->sendtime = time;
                msg->send_evt = evt;
            }
        }
        else if (verbose) // True CREATION_BCAST / CREATION_MULTICAST -- Not yet seen, therefore unhandled
        {
            std::cout << "BCAST/MULTICAST!" << " on pe " << pe;
            std::cout << " with event " << event;
            std::cout << " for " << chares->value(entries->value(entry)->chare)->name.toStdString().c_str();
            std::cout << "::" << entries->value(entry)->name.toStdString().c_str();
            if (!last.isEmpty())
                std::cout << " with index " << last.top()->index.toVerboseString().toStdString().c_str();
            std::cout << " at " << time << std::endl;
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
        if (entries->value(entry)->name.startsWith("start_compute"))
            return;

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
            original_array = arrayid;

            // Special case now that CkReductionMgr has the wrong array id for some reason
            int chare = entries->value(entry)->chare;
            if (chare == traceChare || chare == forravel
                || application_group_chares.contains(chare))
            {
                arrayid = 0;
            }
            else if (chares->value(chare)->name.startsWith("Ck"))
            {

                if (!reductions->contains(arrayid))
                    reductions->insert(arrayid, new QMap<int, int>());
                if (!reductions->value(arrayid)->value(event))
                {
                    reductions->value(arrayid)->insert(event, reduction_count);
                    reduction_count++;
                }
                assoc = reductions->value(arrayid)->value(event);

                arrayid = 0;
            }

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


        // Close off anything that is missing the end message, that's what
        // Projections does
        if (!charm_stack->isEmpty())
        {
            CharmEvt * front = charm_stack->pop();
            CharmEvt * back = new CharmEvt(front->entry, time, my_pe,
                                           front->chare, front->arrayid,
                                           false);
            charm_events->at(my_pe)->append(back);
            if (!last.isEmpty())
                last.pop();
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

        last.push(evt);

        charm_stack->push(evt);

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
            evt->associated_array = assoc;
            charm_events->at(my_pe)->append(evt);

            CharmEvt * recv_end = new CharmEvt(RECV_FXN, time+1, my_pe,
                                               entries->value(entry)->chare,
                                               arrayid, false);
            recv_end->index = id;
            recv_end->associated_array = assoc;
            charm_events->at(my_pe)->append(recv_end);

            msg->recvtime = time;
            msg->recv_evt = evt;
            evt->charmmsgs->append(msg);
        }
        else if (verbose)
        {
            std::cout << "NO MSG FOR RECV!!!" << " on pe " << my_pe << " was expecting message from ";
            std::cout << pe << " with event " << event;
            std::cout << " for " << chares->value(entries->value(entry)->chare)->name.toStdString().c_str();
            std::cout << "::" << entries->value(entry)->name.toStdString().c_str();
            std::cout << " with index " << id.toVerboseString().toStdString().c_str() << std::endl;
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
        if (!last.isEmpty()) // Last may not exist since we can begin recording at a function end
            evt->index = last.top()->index;
        evt->index.chare = entries->value(entry)->chare;
        charm_events->at(my_pe)->append(evt);

        if (!last.isEmpty())
            last.pop();
        if (!charm_stack->isEmpty())
            charm_stack->pop();

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

        if (!last.isEmpty())
            last.pop();

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

        if (!last.isEmpty())
            last.pop();

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

// Check if send and recv belong to the same message
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


// Populate list of functions from the Charm++ entries
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
    }
    entries->insert(SEND_FXN, new Entry(0, "Send", 0));
    entries->insert(RECV_FXN, new Entry(0, "Recv", 0));
    entries->insert(IDLE_FXN, new Entry(0, "Idle", 0));
}


// Read/store meta information from the STS file
// STS is never gzipped.
void CharmImporter::readSts(QString dataFileName)
{
    std::ifstream stsfile(dataFileName.toStdString().c_str());
    std::string line;

    while (std::getline(stsfile, line))
    {
        QStringList lineList = QString::fromStdString(line).split(" ");
        if (lineList.at(0) == "ENTRY" && lineList.at(1) == "CHARE")
        {
            // We have to do relative to end because some names may have spaces
            int len = lineList.length();
            QString name = lineList.at(3);
            for (int i = 4; i < len - 2; i++)
                name += " " + lineList.at(i);
            entries->insert(lineList.at(2).toInt(),
                            new Entry(lineList.at(len-2).toInt(), name,
                                      lineList.at(len-1).toInt()));

            if (name.startsWith("addContribution(CkReductionMsg"))
                addContribution->insert(lineList.at(2).toInt());
            else if (name.startsWith("RecvMsg(CkReductionMsg"))
                recvMsg->insert(lineList.at(2).toInt());
            else if (name.contains("atomic"))
            {
                atomics->insert(lineList.at(2).toInt(),
                                name.split("_atomic_").at(1).toInt());
            }
        }
        else if (lineList.at(0) == "CHARE")
        {
            chares->insert(lineList.at(1).toInt(),
                           new Chare(lineList.at(2)));

            if (main < 0 && QString::compare(lineList.at(2), "main", Qt::CaseInsensitive) == 0)
                main = lineList.at(1).toInt();
            else if (forravel < 0 && QString::compare(lineList.at(2), "ForRavel") == 0)
                forravel = lineList.at(1).toInt();
            else if (traceChare < 0 && QString::compare(lineList.at(2), "TraceProjectionsBOC") == 0)
                traceChare = lineList.at(1).toInt();
            else if (QString::compare(lineList.at(2), "DataManager") == 0)
                application_group_chares.insert(lineList.at(1).toInt(), -1);
            else if (QString::compare(lineList.at(2), "CompletionDetector") == 0)
            {
                application_group_chares.insert(lineList.at(1).toInt(), -1);
            }
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


