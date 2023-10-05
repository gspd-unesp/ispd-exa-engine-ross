

#ifndef ISPD_VIRTUAL_MACHINE_MONITOR_HPP
#define ISPD_VIRTUAL_MACHINE_MONITOR_HPP
#include <utility>
#include <ross.h>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <ispd/message/message.hpp>
#include <ispd/model/builder.hpp>
#include <ispd/metrics/metrics.hpp>
#include<ispd/services/services.hpp>
#include <ispd/allocator/allocator.hpp>
#include <ispd/cloud_scheduler/cloud_scheduler.hpp>
#include <ispd/routing/routing.hpp>


namespace ispd::services{


/// the virtual machine monitor has information about the virtual machines.
struct slave_vms_info{
  tw_lpid id;
  double memory;
  double disk;
  double num_cores;
};

struct VMM_metrics
{
  unsigned task_proc;
  unsigned vms_alloc;
  unsigned vms_rejected;
  double total_turnaround_time;
};

struct VMM_state
{
  std::vector<struct slave_vms_info> vms;
  std::vector<tw_lpid> machines;

  tw_lpid allocated_vms[10000];

  /// links a virtual machine with its owner.
  std::unordered_map<tw_lpid, tw_lpid> *owner;
  ispd::allocator::Allocator *allocator;
  ispd::workload::Workload *workload;
  ispd::cloud_scheduler::CloudScheduler *scheduler;

  unsigned total_vms_to_allocate;
  unsigned total_vms;

  VMM_metrics metrics;

};

struct VMM{

  static void init(VMM_state *s, tw_lp *lp) {
    const auto &service_initializer =
        ispd::this_model::getServiceInitializer(lp->gid);

    service_initializer(s);
    s->owner = new std::unordered_map<tw_lpid, tw_lpid>();
    s->scheduler->initScheduler(s->total_vms_to_allocate);
    s->allocator->initAllocator();

    s->metrics.task_proc = 0;
    s->metrics.vms_alloc = 0;
    s->metrics.vms_rejected = 0;

    s->total_vms = s->total_vms_to_allocate;
    /// Send a generate message to itself.
    double offset = 0.0;

    tw_event *const e = tw_event_new(lp->gid, offset, lp);
    ispd_message *const m = static_cast<ispd_message *>(tw_event_data(e));

    m->type = message_type::GENERATE;
    tw_event_send(e);
    ispd_debug("VMM %lu has been initialized with %lu vms to allocate.", lp->gid, s->total_vms);
  }


  static void forward(VMM_state *s, tw_bf *bf, ispd_message *msg, tw_lp *lp) {
    ispd_debug("Message came from %lu of type %lu", msg->previous_service_id,
               msg->type);

    switch (msg->type) {
    case message_type::GENERATE:
      generate(s, bf, msg, lp);
      break;
    case message_type::ARRIVAL:
      arrival(s, bf, msg, lp);
      break;
    default:
      std::cerr << "Unknown message type " << static_cast<int>(msg->type)
                << " at VMM LP forward handler." << std::endl;
      abort();
      break;
    }
  }


  static void reverse(VMM_state *s, tw_bf *bf, ispd_message *msg, tw_lp *lp) {

    switch (msg->type) {
    case message_type::GENERATE:
      generate_rc(s, bf, msg, lp);
      break;
    case message_type::ARRIVAL:
      arrival_rc(s, bf, msg, lp);
      break;
    default:
      std::cerr << "Unknown message type " << static_cast<int>(msg->type)
                << " at VMM LP reverse handler." << std::endl;
      ispd_debug("Came from LP %lu", msg->previous_service_id);
      abort();
      break;
    }
  }

  static void finish(VMM_state *s, tw_lp *lp) {

 //   ispd::node_metrics::notifyMetric(ispd::metrics::NodeMetricsFlag::NODE_TOTAL_ALLOCATED_VMS,s->metrics.vms_alloc);
   // ispd::node_metrics::notifyMetric(ispd::metrics::NodeMetricsFlag::NODE_TOTAL_REJECTED_VMS, s->metrics.vms_rejected);
   // ispd::node_metrics::notifyMetric(ispd::metrics::NodeMetricsFlag::NODE_TOTAL_COMPLETED_TASKS,s->metrics.task_proc);
    std::printf(
        "Virtual Machine Monitor metrics (%lu)\n"
        " - Total Vms allocated......: %u (%lu)\n"
        " - Total Vms rejected.......: %u (%lu) \n"
        " - Total tasks processed....: %u (%lu) \n",
        lp->gid, s->metrics.vms_alloc,lp->gid,  s->metrics.vms_rejected, lp->gid,
        s->metrics.task_proc, lp->gid
    );

  }


private:
  static void generate(VMM_state *s, tw_bf *bf, ispd_message *msg, tw_lp *lp) {
    ispd_debug("There are %u tasks and %u vms", s->workload->getRemainingTasks(), s->total_vms_to_allocate);
    if ( s->total_vms_to_allocate > 0)
      allocate(s, bf, msg, lp);
    else
      schedule(s, bf, msg, lp);
  }


  static void allocate(VMM_state *s, tw_bf *bf, ispd_message *msg, tw_lp *lp) {
    ispd_debug(
        "VMM %lu will generate an allocation process at %lf, remaining %u.",
        lp->gid, tw_now(lp), s->total_vms_to_allocate);



    const tw_lpid machine_chosen =
        s->allocator->forwardAllocation(s->machines, bf, msg, lp);

    const ispd::routing::Route *route =
        ispd::routing_table::getRoute(lp->gid, machine_chosen);
    tw_event *const e = tw_event_new(route->get(0), 0.0, lp);
    ispd_message *const m = static_cast<ispd_message *>(tw_event_data(e));

    m->type = message_type::ARRIVAL;

    s->workload->generateWorkload(lp->rng, m->task.m_ProcSize,
                                  m->task.m_CommSize);
    s->total_vms_to_allocate--;

    m->task.m_Origin = lp->gid;
    m->task.m_Dest = machine_chosen;
    m->task.m_SubmitTime = tw_now(lp);
    m->task.m_Owner = s->workload->getOwner();

    m->route_offset = 1;
    m->previous_service_id = lp->gid;
    m->downward_direction = 1;
    m->task_processed = 0;

    m->is_vm = 1;
    m->vm_fit = 0;
    m->vm_disk_space = s->vms[0].disk;
    m->vm_num_cores = s->vms[0].num_cores;
    m->vm_memory_space = s->vms[0].memory;
    m->vm_id = s->vms[0].id;

    s->vms.erase(s->vms.begin());

    tw_event_send(e);


    /// send message to itself to continue allocation
    if (s->total_vms_to_allocate > 0) {
      double offset;

      s->workload->generateInterarrival(lp->rng, offset);
      tw_event *const e2 = tw_event_new(lp->gid, offset, lp);


      ispd_message *const m2 = static_cast<ispd_message *>(tw_event_data(e2));

      m2->type = message_type::GENERATE;

      tw_event_send(e2);
    }

  }


  static void schedule(VMM_state *s, tw_bf *bf, ispd_message *msg, tw_lp *lp) {
    tw_lpid vm_id =
        s->scheduler->forwardSchedule(s->allocated_vms, bf, msg, lp);


      ispd_info("vm id : %lu", vm_id);
    auto verify = s->owner->find(vm_id);
    tw_lpid dest;
    if (verify != s->owner->end())
      dest = verify->second;
    else
      ispd_error("There is no machine responsible for vm %u", vm_id);
    const ispd::routing::Route *route =
        ispd::routing_table::getRoute(lp->gid, dest);

    tw_event *const e = tw_event_new(route->get(0), 0.0, lp);
    ispd_message *const m = static_cast<ispd_message *>(tw_event_data(e));

    m->type = message_type::ARRIVAL;

    s->workload->generateWorkload(lp->rng, m->task.m_ProcSize,
                                  m->task.m_CommSize);

    m->task.m_Origin = lp->gid;
    m->task.m_Dest = dest;
    m->vm_id = vm_id;
    m->is_vm = 0;
    m->task.m_SubmitTime = tw_now(lp);
    m->task.m_Owner = s->workload->getOwner();


    m->route_offset = 1;
    m->previous_service_id = lp->gid;
    m->downward_direction = 1;
    m->task_processed = 0;

    tw_event_send(e);
    if (s->workload->getRemainingTasks() > 0) {
      double offset;

      s->workload->generateInterarrival(lp->rng, offset);

      tw_event *const e = tw_event_new(lp->gid, offset, lp);
      ispd_message *const m = static_cast<ispd_message *>(tw_event_data(e));

      m->type = message_type::GENERATE;

      tw_event_send(e);
    }
  }

  static void arrival(VMM_state *s, tw_bf *bf, ispd_message *msg, tw_lp *lp) {
    ispd_debug("Arrived a message in vmm of vm %lu and fit %lu ", msg->is_vm,
               msg->vm_fit);

    if (msg->is_vm) {
      /// erases the vm in the list of vms and put it on the list of allocated
      /// vms

      if (msg->vm_fit) {

        s->allocated_vms[s->metrics.vms_alloc] = msg->vm_id;

        ispd_debug("Vm %lu is allocated on machine %lu", msg->vm_id, msg->allocated_in);
        s->owner->emplace(std::make_pair(msg->vm_id, msg->allocated_in));


        s->metrics.vms_alloc++;


      }
      /// rejects the vm
      else {
        s->metrics.vms_rejected++;

      }



      if (s->metrics.vms_alloc + s->metrics.vms_rejected == s->total_vms)
      {
        double offset;

        s->workload->generateInterarrival(lp->rng, offset);
        tw_event *const e2 = tw_event_new(lp->gid, offset, lp);
        ispd_message *const m2 = static_cast<ispd_message *>(tw_event_data(e2));

        m2->type = message_type::GENERATE;

        tw_event_send(e2);
      }
    }
    /// arrival of an ordinary task
    else {
      msg->task.m_EndTime = tw_now(lp);
      const double turnaround_time = msg->task.m_EndTime - msg->task.m_SubmitTime;
      s->metrics.task_proc++;
      s->metrics.total_turnaround_time += turnaround_time;
    }
  }



  static void generate_rc(VMM_state *s, tw_bf *bf, ispd_message *msg,
                          tw_lp *lp) {
    /// the last processed message was a vm
    if (msg->is_vm)
      allocate_rc(s, bf, msg, lp);
    else
      schedule_rc(s,bf,msg,lp);
  }

  static void  allocate_rc(VMM_state *s, tw_bf *bf, ispd_message *msg,
                          tw_lp *lp) {

    s->allocator->reverseAllocation(s->machines, bf, msg, lp);

    s->workload->reverseGenerateWorkload(lp->rng);
    s->total_vms_to_allocate++;

    /// Checks if after reversing the workload generator, there are remaining
    /// vms to be generated. If so, the random number generator is reversed
    /// since it is used to generate the interarrival time of the vm.
    if (s->workload->getRemainingTasks() > 0)
      s->workload->reverseGenerateInterarrival(lp->rng);
  }

  static void schedule_rc(VMM_state *s, tw_bf *bf, ispd_message *msg, tw_lp *lp)
  {
    /// Reverse the schedule.
    s->scheduler->reverseSchedule(s->allocated_vms, bf, msg, lp);

    /// Reverse the workload generator.
    s->workload->reverseGenerateWorkload(lp->rng);

    /// Checks if after reversing the workload generator, there are remaining tasks to be generated.
    /// If so, the random number generator is reversed since it is used to generate the interarrival
    /// time of the tasks.
    if (s->workload->getRemainingTasks() > 0)
      s->workload->reverseGenerateInterarrival(lp->rng);

  }

  static void arrival_rc(VMM_state *s, tw_bf *bf, ispd_message *msg,
                         tw_lp *lp) {
    if (msg->is_vm) {

      if (msg->vm_fit) {
        struct slave_vms_info vm;
        vm.id = msg->vm_id;
        vm.memory = msg->vm_memory_space;
        vm.disk = msg->vm_disk_space;
        vm.num_cores = msg->vm_num_cores;

        s->vms.insert(s->vms.begin(), vm);

        uint index_to_remove;
        for (uint i = 0; i < s->metrics.vms_alloc; i ++)
        {
            if (s->allocated_vms[i] == msg->vm_id){
                index_to_remove = i;
                break;
            }
         }

        for (uint i = index_to_remove; i < s->metrics.vms_alloc - index_to_remove; i++)
        {
            s->allocated_vms[i] = s->allocated_vms[i+1];
        }
        s->metrics.vms_alloc--;
      }



    } else {
      /// Calculate the task`s turnaround time.
      const double turnaround_time = msg->task.m_EndTime - msg->task.m_SubmitTime;

      /// Reverse the master's metrics.
      s->metrics.task_proc--;
      s->metrics.total_turnaround_time -= turnaround_time;
    }
  }

};
};
#endif
