#ifndef ISPD_SERVICE_MASTER_HPP
#define ISPD_SERVICE_MASTER_HPP

#include <ross.h>
#include <vector>
#include <memory>
#include <chrono>
#include <ispd/debug/debug.hpp>
#include <ispd/model/builder.hpp>
#include <ispd/routing/routing.hpp>
#include <ispd/metrics/metrics.hpp>
#include <ispd/workload/workload.hpp>
#include <ispd/scheduler/scheduler.hpp>
#include <ispd/scheduler/round_robin.hpp>

namespace ispd {
namespace services {

struct master_metrics {
  /// \brief Amount of cmpleted tasks scheduled by the master.
  unsigned completed_tasks;
  
  /// \brief Sum of all turnaround times of completed tasks.
  double total_turnaround_time;
};

struct master_state {
  /// \brief Master's slaves.
  std::vector<tw_lpid> slaves;

  /// \brief Master's scheduler.
  ispd::scheduler::scheduler *scheduler;

  /// \brief Master's workload generator.
  ispd::workload::Workload *workload;

  /// \brief Master's metrics.
  master_metrics metrics;
};

struct master {

  static void init(master_state *s, tw_lp *lp) {
    /// Fetch the service initializer from this logical process.
    const auto &service_initializer = ispd::this_model::getServiceInitializer(lp->gid);

    /// Call the service initializer for this logical process.
    service_initializer(s);
   
    /// Initialize the scheduler.
    s->scheduler->init_scheduler();

    const uint32_t registered_routes_count = ispd::routing_table::countRoutes(lp->gid);

    /// Early sanity check if the routes has been registered correctly. If not,
    /// the program is immediately aborted.
    if (registered_routes_count != s->slaves.size())
      ispd_error("There are %u registered routes starting from master with GID %lu but there are %lu slaves.", registered_routes_count, lp->gid, s->slaves.size());

    /// Initialize the metrics.
    s->metrics.completed_tasks = 0;
    s->metrics.total_turnaround_time = 0;

    /// Checks if the specified workload has remaining tasks. If so, a generate message
    /// will be sent to the master itself to start generating the workload. Otherwise,
    /// no workload is generate at all, since at initialization it has been identified
    /// that the specified workload has no tasks.
    if (s->workload->getRemainingTasks() > 0) {
      double offset;

      s->workload->generateInterarrival(lp->rng, offset);

      /// Send a generate message to itself.
      tw_event *const e = tw_event_new(lp->gid, offset, lp);
      ispd_message *const m = static_cast<ispd_message *>(tw_event_data(e));

      m->type = message_type::GENERATE;

      tw_event_send(e);
    }

    /// Print a debug message.
    ispd_debug("Master %lu has been initialized.", lp->gid);
  }

  static void forward(master_state *s, tw_bf *bf, ispd_message *msg, tw_lp *lp) {
    ispd_debug("[Forward] Master %lu received a message at %lf of type (%d).", lp->gid, tw_now(lp), msg->type);

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
    ispd_debug("[Reverse] Master %lu received a message at %lf of type (%d).", lp->gid, tw_now(lp), msg->type);

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
    ispd::node_metrics::notifyMetric(ispd::metrics::NodeMetricsFlag::NODE_TOTAL_COMPLETED_TASKS, s->metrics.completed_tasks);
    ispd::node_metrics::notifyMetric(ispd::metrics::NodeMetricsFlag::NODE_TOTAL_MASTER_SERVICES);
    ispd::node_metrics::notifyMetric(ispd::metrics::NodeMetricsFlag::NODE_TOTAL_TURNAROUND_TIME, s->metrics.total_turnaround_time);

    const double avgTurnaroundTime = s->metrics.total_turnaround_time / s->metrics.completed_tasks;

    std::printf(
        "Master Metrics (%lu)\n"
        " - Completed Tasks.....: %u tasks (%lu).\n"
        " - Avg. Turnaround Time: %lf seconds (%lu).\n"
        "\n",
        lp->gid,
        s->metrics.completed_tasks, lp->gid,
        avgTurnaroundTime, lp->gid
    );
  }

private:
  static void generate(master_state *s, tw_bf *bf, ispd_message *msg, tw_lp *lp) {
    ispd_debug("Master %lu will generate a task at %lf, remaining %u.", lp->gid, tw_now(lp), s->workload->getRemainingTasks());

#ifdef DEBUG_ON
  const auto start = std::chrono::high_resolution_clock::now();
#endif // DEBUG_ON

    /// Use the master's scheduling policy to the schedule the next slave.
    const tw_lpid scheduled_slave_id = s->scheduler->forward_schedule(s->slaves, bf, msg, lp);

    /// Fetch the route that connects this master with the scheduled slave.
    const ispd::routing::Route *route = ispd::routing_table::getRoute(lp->gid, scheduled_slave_id);

    /// @Todo: This zero-delay timestamped message, could affect the conservative synchronization.
    ///        This should be changed later.
    tw_event *const e = tw_event_new(route->get(0), 0.0, lp);
    ispd_message *const m = static_cast<ispd_message *>(tw_event_data(e));

    m->type = message_type::ARRIVAL;

    /// Use the master's workload generator for generate the task's
    /// processing and communication sizes.
    s->workload->generateWorkload(lp->rng, m->task.proc_size, m->task.comm_size);

    /// Task information specification.
    m->task.origin = lp->gid;
    m->task.dest = scheduled_slave_id;
    m->task.submit_time = tw_now(lp);
    m->task.user = &s->workload->getUser();

    m->route_offset = 1;
    m->previous_service_id = lp->gid;
    m->downward_direction = 1;
    m->task_processed = 0;

    tw_event_send(e);

    /// Checks if the there are more remaining tasks to be generated. If so, a generate message
    /// is sent to the master by itself to generate a new task.
    if (s->workload->getRemainingTasks() > 0) {
      double offset;

      s->workload->generateInterarrival(lp->rng, offset);

      /// Send a generate message to itself.
      tw_event *const e = tw_event_new(lp->gid, offset, lp);
      ispd_message *const m = static_cast<ispd_message *>(tw_event_data(e));

      m->type = message_type::GENERATE;

     tw_event_send(e);    
    }

#ifdef DEBUG_ON
  const auto end = std::chrono::high_resolution_clock::now();
  const auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
  const auto timeTaken = static_cast<double>(duration.count());

  ispd::node_metrics::notifyMetric(ispd::metrics::NodeMetricsFlag::NODE_MASTER_FORWARD_TIME, timeTaken);
#endif // DEBUG_ON
  }

  static void generate_rc(master_state *s, tw_bf *bf, ispd_message *msg, tw_lp *lp) {
#ifdef DEBUG_ON
  const auto start = std::chrono::high_resolution_clock::now();
#endif // DEBUG_ON

    /// Reverse the schedule.
    s->scheduler->reverse_schedule(s->slaves, bf, msg, lp);

    /// Reverse the workload generator.
    s->workload->reverseGenerateWorkload(lp->rng);

    /// Checks if after reversing the workload generator, there are remaining tasks to be generated.
    /// If so, the random number generator is reversed since it is used to generate the interarrival
    /// time of the tasks.
    if (s->workload->getRemainingTasks() > 0)
      s->workload->reverseGenerateInterarrival(lp->rng);

#ifdef DEBUG_ON
  const auto end = std::chrono::high_resolution_clock::now();
  const auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
  const auto timeTaken = static_cast<double>(duration.count());

  ispd::node_metrics::notifyMetric(ispd::metrics::NodeMetricsFlag::NODE_MASTER_REVERSE_TIME, timeTaken);
#endif // DEBUG_ON
  }

  static void arrival(master_state *s, tw_bf *bf, ispd_message *msg, tw_lp *lp) {
    /// Calculate the end time of the task.
    msg->task.end_time = tw_now(lp);

    /// Calculate the task`s turnaround time.
    const double turnaround_time = msg->task.end_time - msg->task.submit_time;

    /// Update the master's metrics.
    s->metrics.completed_tasks++;
    s->metrics.total_turnaround_time += turnaround_time;
  }

  static void arrival_rc(master_state *s, tw_bf *bf, ispd_message *msg, tw_lp *lp) {
    /// Calculate the task`s turnaround time.
    const double turnaround_time = msg->task.end_time - msg->task.submit_time;

    /// Reverse the master's metrics.
    s->metrics.completed_tasks--;
    s->metrics.total_turnaround_time -= turnaround_time;
  }

};

}; // namespace services
}; // namespace ispd
#endif // ISPD_SERVICE_MASTER_HPP
