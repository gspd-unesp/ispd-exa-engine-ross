#ifndef ISPD_SERVICE_MASTER_HPP
#define ISPD_SERVICE_MASTER_HPP

#include <vector>
#include <ross.h>
#include <ispd/debug/debug.hpp>
#include <ispd/routing/routing.hpp>
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

  /// \brief Master's metrics.
  master_metrics metrics;
};

struct master {

  static void init(master_state *s, tw_lp *lp) {
    /// @Todo: Initialize the master configuration dynamically
    ///        using a model builder.

    /// @Temporary:
    s->slaves.reserve(1);
    s->slaves[0] = 2;
    s->scheduler = new ispd::scheduler::round_robin;
    /// @Temporary: End
    
    /// Initialize the metrics.
    s->metrics.completed_tasks = 0;

    /// Send a generate message to itself.
    tw_event *const e = tw_event_new(lp->gid, tw_rand_exponential(lp->rng, 0.1), lp);
    ispd_message *const m = static_cast<ispd_message *>(tw_event_data(e));

    m->type = message_type::GENERATE;

    tw_event_send(e);
  }

  static void forward(master_state *s, tw_bf *bf, ispd_message *msg, tw_lp *lp) {
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

  }

  static void finish(master_state *s, tw_lp *lp) {
    DEBUG({
        std::printf(
            "Master Metrics (%lu)\n"
            " - Completed Tasks: %u tasks (%lu).\n"
            "\n",
            lp->gid, s->metrics.completed_tasks, lp->gid
        );
    });
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
    
    /// @Temporary: This should be set by the workload generator.
    m->task.proc_size = 250.0;
    m->task.comm_size = 80.0;
    /// @Temporary: End

    m->task.origin = lp->gid;
    m->task.dest = scheduled_slave_id;
    m->route_offset = 1;
    m->previous_service_id = lp->gid;
    m->downward_direction = 1;
    m->task_processed = 0;

    tw_event_send(e);
  }

  static void arrival(master_state *s, tw_bf *bf, ispd_message *msg, tw_lp *lp) {
    /// Update master's metrics.
    s->metrics.completed_tasks++;
  }

};

}; // namespace services
}; // namespace ispd
#endif // ISPD_SERVICE_MASTER_HPP
