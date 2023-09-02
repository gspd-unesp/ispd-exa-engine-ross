

#ifndef ISPD_SERVICE_VMM_HPP
#define ISPD_SERVICE_VMM_HPP
#define VM_FLAG 0
#define TASK_FLAG 1
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
#include <ispd/allocator/first_fit_decreasing.hpp>
#include <ispd/scheduler/scheduler.hpp>
#include <ispd/routing/routing.hpp>
namespace ispd {

namespace services {
/// The virtual machine monitor has basic information about the virtual machines
/// it's responsible
struct slave_vms_info {
  tw_lpid id;
  double avaliable_memory;
  double avaliable_disk;
  unsigned num_cores;
};

struct VMM_metrics {
  unsigned tasks_proc;
  unsigned vm_alloc;
  unsigned vms_rejected;
  double total_turnaround_time;
};

struct VMM_state {

  std::vector<struct slave_vms_info> vms;
  std::vector<tw_lpid> allocated_vms;
  std::vector<tw_lpid> machines;
  /// links a virtual machine with its owner.
  std::unordered_map<tw_lpid, tw_lpid> *owner;
  ispd::allocator::allocator *allocator;
  ispd::workload::Workload *workload;
  ispd::scheduler::scheduler *scheduler;

  VMM_metrics metrics;
};

struct VMM {

  static void init(VMM_state *s, tw_lp *lp) {
    const auto &service_initializer =
        ispd::this_model::getServiceInitializer(lp->gid);

    service_initializer(s);
  s->owner = new std::unordered_map<tw_lpid, tw_lpid>();
    s->scheduler->init_scheduler();
    s->allocator->init();

    /// checks if allocator is an instance of first_fit_decreasing and sorts the vms for that'
    if (ispd::allocator::firt_fit_decreasing *derived_ptr =
            dynamic_cast<ispd::allocator::firt_fit_decreasing *>(
                s->allocator)) {
      std::sort(s->vms.begin(), s->vms.end(), sorting_criteria);
    }
    s->metrics.tasks_proc = 0;
    s->metrics.vm_alloc = 0;
    s->metrics.vms_rejected = 0;
    /// Send a generate message to itself.
    double offset = 0.0;
    s->workload->generateInterarrival(lp->rng, offset);

    tw_event *const e = tw_event_new(lp->gid, offset, lp);
    ispd_message *const m = static_cast<ispd_message *>(tw_event_data(e));

    m->type = message_type::GENERATE;
    tw_event_send(e);
    ispd_debug("VMM %lu has been initialized.", lp->gid);
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
    default:
      std::cerr << "Unknown message type " << static_cast<int>(msg->type)
                << " at VMM LP reverse handler." << std::endl;
      abort();
      break;
    }
  }

  static void finish(VMM_state *s, tw_lp *lp) {

    ispd::node_metrics::notifyMetric(
        ispd::metrics::NodeMetricsFlag::NODE_TOTAL_ALLOCATED_VMS,
        s->metrics.vm_alloc);
    //  ispd::node_metrics::notifyMetric(ispd::metrics::NodeMetricsFlag::NODE_TOTAL_REJECTED_VMS, s->metrics.vms_rejected);
    std::printf("allocated vms: %u \n tasks processed: %u \n", s->metrics.vm_alloc,
                s->metrics.tasks_proc);
  }

private:
  static void generate(VMM_state *s, tw_bf *bf, ispd_message *msg, tw_lp *lp) {
    ispd_debug("There are %u tasks", s->workload->getRemainingTasks());
    if (s->workload->getRemainingVms() > 0)
      allocate(s, bf, msg, lp);
    else
      schedule(s, bf, msg, lp);
  }

  static void allocate(VMM_state *s, tw_bf *bf, ispd_message *msg, tw_lp *lp) {
    ispd_debug(
        "VMM %lu will generate an allocation process at %lf, remaining %u.",
        lp->gid, tw_now(lp), s->workload->getRemainingVms());

    const tw_lpid machine_chosen =
        s->allocator->forward_allocation(s->machines, bf, msg, lp);
    ispd_debug("%lu ", machine_chosen);

    const ispd::routing::Route *route =
        ispd::routing_table::getRoute(lp->gid, machine_chosen);
    tw_event *const e = tw_event_new(route->get(0), 0.0, lp);
    ispd_message *const m = static_cast<ispd_message *>(tw_event_data(e));

    m->type = message_type::ARRIVAL;

    s->workload->generateWorkload(lp->rng, m->task.proc_size,
                                  m->task.comm_size, VM_FLAG);

    m->task.origin = lp->gid;
    m->task.dest = machine_chosen;
    m->task.submit_time = tw_now(lp);
    m->task.owner = s->workload->getOwner();

    m->route_offset = 1;
    m->previous_service_id = lp->gid;
    m->downward_direction = 1;
    m->task_processed = 0;

    m->is_vm = 1;
    m->fit = 0;
    m->vm_disk_space = s->vms[0].avaliable_disk;
    m->vm_num_cores = s->vms[0].num_cores;
    m->vm_memory_space = s->vms[0].avaliable_memory;
    m->vm_sent = s->vms[0].id;

    tw_event_send(e);
  }

  static void schedule(VMM_state *s, tw_bf *bf, ispd_message *msg, tw_lp *lp) {
    tw_lpid vm_id =
        s->scheduler->forward_schedule(s->allocated_vms, bf, msg, lp);

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

    s->workload->generateWorkload(lp->rng, m->task.proc_size,
                                  m->task.comm_size, TASK_FLAG);

    m->task.origin = lp->gid;
    m->task.dest = dest;
    m->vm_sent = vm_id;
    m->task.submit_time = tw_now(lp);
    m->task.owner = s->workload->getOwner();

    m->route_offset = 1;
    m->previous_service_id = lp->gid;
    m->downward_direction = 1;
    m->task_processed = 0;

    tw_event_send(e);
    /// Checks if the there are more remaining tasks to be generated. If so, a generate message is sent to the master by itself to generate a new task.
    if (s->workload->getRemainingTasks() > 0) {
      double offset;

      s->workload->generateInterarrival(lp->rng, offset);

      /// Send a generate message to itself.
      tw_event *const e = tw_event_new(lp->gid, offset, lp);
      ispd_message *const m = static_cast<ispd_message *>(tw_event_data(e));

      m->type = message_type::GENERATE;

      tw_event_send(e);
    }
  }
  static void arrival(VMM_state *s, tw_bf *bf, ispd_message *msg, tw_lp *lp) {
    ispd_debug("Arrived a message in vmm of vm %lu and fit %lu ", msg->is_vm,
               msg->fit);

    if (msg->is_vm) {
      /// erases the vm in the list of vms and put it on the list of allocated
      /// vms
      if (msg->fit) {
        s->allocated_vms.push_back(msg->vm_sent);
        s->vms.erase(s->vms.begin());


        ispd_debug("Vm %lu is allocated on machine %lu", msg->vm_sent, msg->allocated_in);
        s->owner->emplace(std::make_pair(msg->vm_sent, msg->allocated_in));


        s->metrics.vm_alloc++;

      }
      /// rejects the vm
      else {
        s->vms.erase(s->vms.begin());
        s->metrics.vms_rejected++;
      }
      /// send a message to continue the allocation
      tw_event *const e = tw_event_new(lp->gid, 0.0, lp);
      ispd_message *const m = static_cast<ispd_message *>(tw_event_data(e));
      m->type = message_type::GENERATE;
      tw_event_send(e);

    }
    /// arrival of an ordinary task
    else {
      msg->task.end_time = tw_now(lp);
      const double turnaround_time = msg->task.end_time - msg->task.submit_time;
      s->metrics.tasks_proc++;
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

    s->allocator->reverse_allocation(s->machines, bf, msg, lp);

    s->workload->reverseGenerateWorkload(lp->rng, VM_FLAG);

    /// Checks if after reversing the workload generator, there are remaining
    /// vms to be generated. If so, the random number generator is reversed
    /// since it is used to generate the interarrival time of the vm.
    if (s->workload->getRemainingTasks() > 0)
      s->workload->reverseGenerateInterarrival(lp->rng);
  }

  static void schedule_rc(VMM_state *s, tw_bf *bf, ispd_message *msg, tw_lp *lp)
  {
    /// Reverse the schedule.
    s->scheduler->reverse_schedule(s->allocated_vms, bf, msg, lp);

    /// Reverse the workload generator.
    s->workload->reverseGenerateWorkload(lp->rng, TASK_FLAG);

    /// Checks if after reversing the workload generator, there are remaining tasks to be generated.
    /// If so, the random number generator is reversed since it is used to generate the interarrival
    /// time of the tasks.
    if (s->workload->getRemainingTasks() > 0)
      s->workload->reverseGenerateInterarrival(lp->rng);

  }

  static void arrival_rc(VMM_state *s, tw_bf *bf, ispd_message *msg,
                         tw_lp *lp) {
    if (msg->is_vm) {

      if (msg->fit) {
        struct slave_vms_info vm;
        vm.id = msg->vm_sent;
        vm.avaliable_memory = msg->vm_memory_space;
        vm.avaliable_disk = msg->vm_disk_space;
        vm.num_cores = msg->vm_num_cores;

        s->vms.insert(s->vms.begin(), vm);

        s->allocated_vms.erase(s->allocated_vms.begin());

        s->metrics.vm_alloc--;
      }

    } else {
      /// Calculate the task`s turnaround time.
      const double turnaround_time = msg->task.end_time - msg->task.submit_time;

      /// Reverse the master's metrics.
      s->metrics.tasks_proc--;
      s->metrics.total_turnaround_time -= turnaround_time;
    }
  }

  /// sorting criteria for the First Fit Decreasing algorithm that requires
  /// the vms sorted in a decreasing order.
  static bool sorting_criteria(const slave_vms_info &a, const slave_vms_info &b)
  {
    const int multiplier = 100000;
    double A = multiplier * (a.num_cores + (int)a.avaliable_memory + (int) a.avaliable_disk);
    double B   = multiplier * (b.num_cores + (int)b.avaliable_memory + (int) b.avaliable_disk);

    return A > B;
  }


  };

}; // namespace services
}; // namespace ispd

#endif // ISPD_EXA_ENGINE_ROSS_VMM_HPP
