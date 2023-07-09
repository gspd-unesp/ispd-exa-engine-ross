#ifndef ISPD_SERVICE_MASTER_HPP
#define ISPD_SERVICE_MASTER_HPP

#include <vector>
#include <ross.h>
#include <ispd/debug/debug.hpp>
#include <ispd/model/builder.hpp>
#include <ispd/routing/routing.hpp>
#include <ispd/workload/workload.hpp>
#include <ispd/scheduler/scheduler.hpp>
#include <ispd/scheduler/round_robin.hpp>

namespace ispd {
namespace services {

struct master_metrics {
  /// \brief Amount of cmpleted tasks scheduled by the master.
  unsigned completed_tasks;
};

struct master_state {
  /// \brief Master's slaves.
  std::vector<tw_lpid> slaves;

  /// \brief Master's scheduler.
  ispd::scheduler::scheduler *scheduler;

  /// \brief Master's workload generator.
  ispd::workload::workload *workload;

  /// \brief Master's metrics.
  master_metrics metrics;
};

struct master {

  static void init(master_state *s, tw_lp *lp) {
    /// Fetch the service initializer from this logical process.
    const auto &service_initializer = ispd::model::builder::get_service_initializer(lp->gid);

    /// Call the service initializer for this logical process.
    service_initializer(s);
   
    /// Initialize the scheduler.
    s->scheduler->init_scheduler();

    /// Initialize the metrics.
    s->metrics.completed_tasks = 0;

    /// Send a generate message to itself.
    tw_event *const e = tw_event_new(lp->gid, tw_rand_exponential(lp->rng, 0.1), lp);
    ispd_message *const m = static_cast<ispd_message *>(tw_event_data(e));

    m->type = message_type::GENERATE;

    tw_event_send(e);
  }

  static void forward(master_state *s, tw_bf *bf, ispd_message *msg, tw_lp *lp) {
    DEBUG({
        std::printf("Master with GID %lu has received a message to be processed.\n", lp->gid);
    });

    switch (msg->type) {
      case message_type::GENERATE:
        generate(s, bf, msg, lp);
        break;
      case message_type::ARRIVAL:
        arrival(s, bf, msg, lp);
        break;
      default:
        std::cerr << "Unknown message type " << static_cast<int>(msg->type) << " at Master LP forward handler." << std::endl;
        abort();
        break;
    }
  }

  static void reverse(master_state *s, tw_bf *bf, ispd_message *msg, tw_lp *lp) {
    DEBUG({
        std::printf("Master with GID %lu has received a message to be reversed processed.\n", lp->gid);
    });

    switch (msg->type) {
      case message_type::GENERATE:
        generate_rc(s, bf, msg, lp);
        break;
      case message_type::ARRIVAL:
        arrival_rc(s, bf, msg, lp);
        break;
      default:
        std::cerr << "Unknown message type " << static_cast<int>(msg->type) << " at Master LP reverse handler." << std::endl;
        abort();
        break;
    }
  }

  static void finish(master_state *s, tw_lp *lp) {
      std::printf(
          "Master Metrics (%lu)\n"
          " - Completed Tasks: %u tasks (%lu).\n"
          "\n",
          lp->gid, s->metrics.completed_tasks, lp->gid
      );
  }

private:
  static void generate(master_state *s, tw_bf *bf, ispd_message *msg, tw_lp *lp) {
    /// Use the master's scheduling policy to the schedule the next slave.
    const tw_lpid scheduled_slave_id = s->scheduler->forward_schedule(s->slaves, bf, msg, lp);

    /// Fetch the route that connects this master with the scheduled slave.
    const ispd::routing::route *route = g_routing_table.get_route(lp->gid, scheduled_slave_id);

    /// @Todo: This zero-delay timestamped message, could affect the conservative synchronization.
    ///        This should be changed later.
    tw_event *const e = tw_event_new(route->get(0), 0.0, lp);
    ispd_message *const m = static_cast<ispd_message *>(tw_event_data(e));

    m->type = message_type::ARRIVAL;

    /// Use the master's workload generator for generate the task's
    /// processing and communication sizes.
    s->workload->workload_generate(m->task.proc_size, m->task.comm_size);

    m->task.origin = lp->gid;
    m->task.dest = scheduled_slave_id;
    m->route_offset = 1;
    m->previous_service_id = lp->gid;
    m->downward_direction = 1;
    m->task_processed = 0;

    tw_event_send(e);

    /// Checks if the there are more remaining tasks to be generated. If so, a generate message
    /// is sent to the master by itself to generate a new task.
    if (s->workload->count > 0) {
      /// Send a generate message to itself.
      tw_event *const e = tw_event_new(lp->gid, tw_rand_exponential(lp->rng, 0.1), lp);
      ispd_message *const m = static_cast<ispd_message *>(tw_event_data(e));

      m->type = message_type::GENERATE;

     tw_event_send(e);    
    }
  }

  static void generate_rc(master_state *s, tw_bf *bf, ispd_message *msg, tw_lp *lp) {
    /// Reverse the workload generator.
    s->workload->workload_generate_rc();

    /// Checks if after reversing the workload generator, there are remaining tasks to be generated.
    /// If so, the random number generator is reversed since it is used to generate the interarrival
    /// time of the tasks.
    if (s->workload->count > 0)
      tw_rand_reverse_unif(lp->rng);
  }

  static void arrival(master_state *s, tw_bf *bf, ispd_message *msg, tw_lp *lp) {
    /// Update the master's metrics.
    s->metrics.completed_tasks++;
  }

  static void arrival_rc(master_state *s, tw_bf *bf, ispd_message *msg, tw_lp *lp) {
    /// Reverse the master's metrics.
    s->metrics.completed_tasks--;
  }


};

}; // namespace services
}; // namespace ispd
#endif // ISPD_SERVICE_MASTER_HPP
