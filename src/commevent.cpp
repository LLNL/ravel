#include "commevent.h"
#include <otf2/OTF2_AttributeList.h>
#include <otf2/OTF2_GeneralDefinitions.h>
#include <iostream>
#include "metrics.h"
#include "rpartition.h"

CommEvent::CommEvent(unsigned long long _enter, unsigned long long _exit,
                     int _function, int _task, int _pe, int _phase)
    : Event(_enter, _exit, _function, _task, _pe),
      metrics(new Metrics()),
      //partition(NULL),
      comm_next(NULL),
      comm_prev(NULL),
      true_next(NULL),
      true_prev(NULL),
      pe_next(NULL),
      pe_prev(NULL),
      //atomic(-1),
      matching(-1),
      last_stride(NULL),
      next_stride(NULL),
      last_recvs(NULL),
      last_step(-1),
      stride_parents(new QSet<CommEvent *>()),
      stride_children(new QSet<CommEvent *>()),
      stride(-1),
      step(-1),
      phase(_phase),
      gvid("")
{
}

CommEvent::~CommEvent()
{
    delete metrics;

    if (last_recvs)
        delete last_recvs;
    if (stride_children)
        delete stride_children;
    if (stride_parents)
        delete stride_parents;
}

bool CommEvent::hasMetric(QString name)
{
    if (metrics->hasMetric(name))
        return true;
    else
        return partition->metrics->hasMetric(name);
}

double CommEvent::getMetric(QString name, bool aggregate)
{
    if (metrics->hasMetric(name))
        return metrics->getMetric(name, aggregate);

    return partition->metrics->getMetric(name, aggregate);
}

void CommEvent::calculate_differential_metric(QString metric_name,
                                              QString base_name, bool aggregates)
{
    long long max_parent = metrics->getMetric(base_name, true);
    long long max_agg_parent = 0;
    if (aggregates && comm_prev)
        max_agg_parent = (comm_prev->metrics->getMetric(base_name));

    if (aggregates)
        metrics->addMetric(metric_name,
                           std::max(0.,
                                    getMetric(base_name)- max_parent),
                           std::max(0.,
                                    getMetric(base_name, true)- max_agg_parent));
    else
        metrics->addMetric(metric_name,
                           std::max(0.,
                                    getMetric(base_name)- max_parent));
}

void CommEvent::writeOTF2Leave(OTF2_EvtWriter * writer, QMap<QString, int> * attributeMap)
{
    // For coalesced steps, don't write attributes
    if (step < 0)
    {
        // Finally write enter event
        OTF2_EvtWriter_Leave(writer,
                             NULL,
                             exit,
                             function);
        return;
    }

    OTF2_AttributeList * attribute_list = OTF2_AttributeList_New();

    // Phase and Step
    OTF2_AttributeValue phase_value;
    phase_value.uint32 = phase;
    OTF2_AttributeList_AddAttribute(attribute_list,
                                    attributeMap->value("phase"),
                                    OTF2_TYPE_UINT64,
                                    phase_value);

    OTF2_AttributeValue step_value;
    step_value.uint32 = step;
    OTF2_AttributeList_AddAttribute(attribute_list,
                                    attributeMap->value("step"),
                                    OTF2_TYPE_UINT64,
                                    step_value);

    // Write metrics
    for (QMap<QString, int>::Iterator attr = attributeMap->begin();
         attr != attributeMap->end(); ++attr)
    {
        if (!metrics->hasMetric(attr.key()))
            continue;

        OTF2_AttributeValue attr_value;
        attr_value.uint64 = metrics->getMetric(attr.key());
        OTF2_AttributeList_AddAttribute(attribute_list,
                                        attributeMap->value(attr.key()),
                                        OTF2_TYPE_UINT64,
                                        attr_value);

        OTF2_AttributeValue agg_value;
        agg_value.uint64 = metrics->getMetric(attr.key(), true);
        OTF2_AttributeList_AddAttribute(attribute_list,
                                        attributeMap->value(attr.key() + "_agg"),
                                        OTF2_TYPE_UINT64,
                                        agg_value);
    }

    // Finally write enter event
    OTF2_EvtWriter_Leave(writer,
                         attribute_list,
                         exit,
                         function);
}
