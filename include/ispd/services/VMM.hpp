

#ifndef ISPD_SERVICE_VMM_HPP
#define ISPD_SERVICE_VMM_HPP

#include <ross.h>
#include <vector>
#include <algorithm>
#include <ispd/message/message.hpp>
#include <ispd/model/builder.hpp>
#include<ispd/services/services.hpp>
#include <ispd/allocator/allocator.hpp>
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

struct allocated_vms {
  tw_lpid id;
  tw_lpid owner;
};
struct VMM_metrics {
  unsigned tasks_proc;
  unsigned vm_alloc;
  unsigned vms_rejected;
};

struct VMM_state {

  std::vector<struct slave_vms_info> vms;
  std::vector<tw_lpid> allocated_vms;
  std::vector<tw_lpid> machines;

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


    s->scheduler->init_scheduler();
    s->allocator->init();

    s->metrics.tasks_proc = 0;
    s->metrics.vm_alloc = 0;
    /// Send a generate message to itself.
    double offset = 0.0;
    s->workload->generateInterarrival(lp->rng, offset);

    tw_event *const e = tw_event_new(lp->gid, offset , lp);
    ispd_message *const m = static_cast<ispd_message *>(tw_event_data(e));

    m->type = message_type::GENERATE;
    tw_event_send(e);
    ispd_debug("VMM %lu has been initialized.", lp->gid);
  }

  static void forward(VMM_state *s, tw_bf *bf, ispd_message *msg, tw_lp *lp) {
    ispd_debug("Message came from %lu of type %lu", msg->previous_service_id, msg->type);
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
    std::printf("allocated vms: %u \n tasks processed: %u", s->metrics.vm_alloc,
                s->metrics.tasks_proc);
  }

private:
  static void generate(VMM_state *s, tw_bf *bf, ispd_message *msg, tw_lp *lp) {
    if (s->workload->getRemainingVms() > 0)
      allocate(s, bf, msg, lp);
    /*      else
            schedule(s,bf,msg,lp);*/
  }

  static void allocate(VMM_state *s, tw_bf *bf, ispd_message *msg, tw_lp *lp) {
    ispd_debug(
        "VMM %lu will generate an allocatio process at %lf, remaining %u.",
        lp->gid, tw_now(lp), s->workload->getRemainingVms());

    const tw_lpid machine_chosen = s->allocator->forward_allocation(s->machines, bf, msg, lp);
    ispd_debug("%lu ", machine_chosen);

    const ispd::routing::Route *route =
        ispd::routing_table::getRoute(lp->gid, machine_chosen);
    tw_event *const e = tw_event_new(route->get(0), 0.0, lp);
    ispd_message *const m = static_cast<ispd_message *>(tw_event_data(e));

    m->type = message_type::ARRIVAL;

    s->workload->generateWorkload(lp->rng, m->task.proc_size,
                                  m->task.comm_size);

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

  static void arrival(VMM_state *s, tw_bf *bf, ispd_message *msg, tw_lp *lp) {
    ispd_debug("Arrived a message in vmm of vm %lu and fit %lu ", msg->is_vm, msg->fit);

    if (msg->is_vm) {
      /// erases the vm in the list of vms and put it on the list of allocated
      /// vms
      if (msg->fit) {
        struct allocated_vms new_vm_allocated;
        new_vm_allocated.id = msg->vm_sent;
        new_vm_allocated.owner = msg->allocated_in;
        s->vms.erase(s->vms.begin());

        s->metrics.vm_alloc++;

      }
      /// put the vm in the tail
      else {
        std::iter_swap(s->vms.begin(), s->vms.end() - 1);
      }
    /// send a message to continue the allocation
    tw_event *const e = tw_event_new(lp->gid, 0.0, lp);
    ispd_message *const m = static_cast<ispd_message *>(tw_event_data(e));
    m->type = message_type::GENERATE;
    tw_event_send(e);

    }
    /// arrival of an ordinary task
    else {
    }
  }

  static void generate_rc(VMM_state *s, tw_bf *bf, ispd_message *msg,
                          tw_lp *lp) {
    /// the last processed message was a vm
    if (msg->is_vm)
      allocate_rc(s, bf, msg, lp);
    /*      else
            schedule_rc(s,bf,msg,lp);*/
  }

  static void allocate_rc(VMM_state *s, tw_bf *bf, ispd_message *msg,
                          tw_lp *lp) {

    s->allocator->reverse_allocation(s->machines, bf, msg, lp);

    s->workload->reverseGenerateWorkload(lp->rng);

    /// Checks if after reversing the workload generator, there are remaining
    /// vms to be generated. If so, the random number generator is reversed
    /// since it is used to generate the interarrival time of the vm.
    if (s->workload->getRemainingTasks() > 0)
      s->workload->reverseGenerateInterarrival(lp->rng);
  }

  static void arrival_rc(VMM_state *s, tw_bf *bf, ispd_message *msg, tw_lp *lp){
    if (msg->is_vm)
    {

      if(msg->fit)
      {
        struct slave_vms_info vm;
        vm.id = msg->vm_sent;
        vm.avaliable_memory = msg->vm_memory_space;
        vm.avaliable_disk = msg->vm_disk_space;
        vm.num_cores = msg->vm_num_cores;

        s->vms.insert(s->vms.begin(), vm);

        s->allocated_vms.erase(s->allocated_vms.begin());

        s->metrics.vm_alloc --;
      }

    }

  }

  };

}; // namespace services
}; // namespace ispd

#endif // ISPD_EXA_ENGINE_ROSS_VMM_HPP
