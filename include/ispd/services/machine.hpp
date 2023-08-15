#ifndef ISPD_SERVICE_MACHINE_HPP
#define ISPD_SERVICE_MACHINE_HPP

#include <ross.h>
#include <vector>
#include <chrono>
#include <limits>
#include <algorithm>
#include <numeric>
#include <ispd/message/message.hpp>
#include <ispd/routing/routing.hpp>
#include <ispd/model/builder.hpp>
#include <ispd/metrics/metrics.hpp>
#include <ispd/metrics/user_metrics.hpp>

extern double g_NodeSimulationTime;

namespace ispd {
namespace services {

class MachineConfiguration {
  double m_PowerPerCore;
  double m_Load;
  unsigned m_CoreCount;

public:
  MachineConfiguration(const double power, const double load, const unsigned coreCount)
    : m_PowerPerCore(power / coreCount), m_Load(load), m_CoreCount(coreCount) {}

  inline double timeToProcess(const double processingSize) {
    return processingSize / ((1.0 - m_Load) * m_PowerPerCore);
  }

  inline double getPower() const { return m_PowerPerCore * m_CoreCount; }
  inline double getPowerPerCore() const { return m_PowerPerCore; }
  inline double getLoad() const { return m_Load; }
};

struct machine_metrics {
  double proc_mflops;
  double proc_time;
  unsigned proc_tasks;
  unsigned forwarded_packets;
  double waiting_time;
};

struct machine_state {
  /// \brief Machine's Configuration.
  MachineConfiguration conf;

  /// \brief Machine's Metrics.
  machine_metrics metrics;

  /// \brief Machine's Queueing Model Information.
  std::vector<double> cores_free_time;
};

struct machine {

  static double least_core_time(const std::vector<double> &cores_free_time, unsigned &core_index) {
    double candidate = std::numeric_limits<double>::max();
    unsigned candidate_index;

    for (unsigned i = 0; i < cores_free_time.size(); i++) {
      if (candidate > cores_free_time[i]) {
        candidate = cores_free_time[i];
        candidate_index = i;
      }
    }

    /// Update the core index with the least free time.
    core_index = candidate_index;

    /// Return the least core's free time.
    return candidate;
  }

  static void init(machine_state *s, tw_lp *lp) {
    /// Fetch the service initializer from this logical process.
    const auto &service_initializer = ispd::this_model::getServiceInitializer(lp->gid);

    /// Call the service initializer for this logical process.
    service_initializer(s);
         
     /// Initialize machine's metrics.
     s->metrics.proc_mflops = 0;
     s->metrics.proc_tasks = 0;
     s->metrics.forwarded_packets = 0;
     s->metrics.waiting_time = 0;

    /// Print a debug message.
    ispd_debug("Machine %lu has been initialized.", lp->gid);
  }

  static void forward(machine_state *s, tw_bf *bf, ispd_message *msg, tw_lp *lp) {
    ispd_debug("[Forward] Machine %lu received a message at %lf of type (%d) and route offset (%u).", lp->gid, tw_now(lp), msg->type, msg->route_offset);

#ifdef DEBUG_ON
  const auto start = std::chrono::high_resolution_clock::now();
#endif // DEBUG_ON

    /// Checks if the task's destination is this machine. If so, the task is processed
    /// and the task's results is sent back to the master by the same route it came along.
    if (msg->task.dest == lp->gid) {
      /// Fetch the processing size and calculates the processing time.
      const double proc_size = msg->task.proc_size;
      const double proc_time = s->conf.timeToProcess(proc_size);

      unsigned core_index;
      const double least_free_time = least_core_time(s->cores_free_time, core_index);
      const double waiting_delay = ROSS_MAX(0.0, least_free_time - tw_now(lp));
      const double departure_delay = waiting_delay + proc_time;

      /// Update the machine's metrics.
      s->metrics.proc_mflops += proc_size;
      s->metrics.proc_time += proc_time;
      s->metrics.proc_tasks++;
      s->metrics.waiting_time += waiting_delay;

      /// Update the machine's queueing model information.
      s->cores_free_time[core_index] = tw_now(lp) + departure_delay;

      tw_event *const e = tw_event_new(msg->previous_service_id, departure_delay, lp);
      ispd_message *const m = static_cast<ispd_message *>(tw_event_data(e));

      *m = *msg;
      m->type = message_type::ARRIVAL;
      m->task = msg->task;             /// Copy the task's information.
      m->task.comm_size = 0.000976562; /// 1 Kib (representing the results).
      m->task_processed = 1;           /// Indicate that the message is carrying a processed task.
      m->downward_direction = 0;       /// The task's results will be sent back to the master.
      m->route_offset = msg->route_offset - 2;
      m->previous_service_id = lp->gid;
      
      /// Save information (for reverse computation).
      msg->saved_core_index = core_index;
      msg->saved_core_next_available_time = least_free_time;

      tw_event_send(e);
    }
    /// Otherwise, this indicates that the task's destination IS NOT this machine and, therefore,
    /// the task should only be forwarded to its next destination. 
    else {
      /// Fetch the route between the task's origin and task's destination.
      const ispd::routing::Route *route = ispd::routing_table::getRoute(msg->task.origin, msg->task.dest);

      /// Update machine's metrics.
      s->metrics.forwarded_packets++;

      /// @Todo: This zero-delay timestamped message could affect the conservative synchronization.
      ///        This should be changed after.
      tw_event *const e = tw_event_new(route->get(msg->route_offset), 0.0, lp);
      ispd_message *const m = static_cast<ispd_message *>(tw_event_data(e));

      m->type = message_type::ARRIVAL;
      m->task = msg->task; /// Copy the tasks's information.
      m->task_processed = msg->task_processed;
      m->downward_direction = msg->downward_direction;
      m->route_offset = msg->downward_direction ? (msg->route_offset + 1) : (msg->route_offset - 1);
      m->previous_service_id = lp->gid;

      tw_event_send(e);
    }
#ifdef DEBUG_ON
  const auto end = std::chrono::high_resolution_clock::now();
  const auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
  const auto timeTaken = static_cast<double>(duration.count());

  ispd::node_metrics::notifyMetric(ispd::metrics::NodeMetricsFlag::NODE_MACHINE_FORWARD_TIME, timeTaken);
#endif // DEBUG_ON
  }

  static void reverse(machine_state *s, tw_bf *bf, ispd_message *msg, tw_lp *lp) {
    ispd_debug("[Reverse] Machine %lu received a message at %lf of type (%d).", lp->gid, tw_now(lp), msg->type);

#ifdef DEBUG_ON
  const auto start = std::chrono::high_resolution_clock::now();
#endif // DEBUG_ON

    /// Check if the task's destination is this machine.
    if (msg->task.dest == lp->gid) {
      const double proc_size = msg->task.proc_size;
      const double proc_time = s->conf.timeToProcess(proc_size);

      const double least_free_time = msg->saved_core_next_available_time;
      const double waiting_delay = ROSS_MAX(0.0, least_free_time - tw_now(lp));

      /// Reverse the machine's metrics.
      s->metrics.proc_mflops -= proc_size;
      s->metrics.proc_time -= proc_time;
      s->metrics.proc_tasks--;
      s->metrics.waiting_time -= waiting_delay;

      /// Reverse the machine's queueing model information.
      s->cores_free_time[msg->saved_core_index] = least_free_time;
    } else {
      /// Reverse machine's metrics.
      s->metrics.forwarded_packets--;
    }

#ifdef DEBUG_ON
  const auto end = std::chrono::high_resolution_clock::now();
  const auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
  const auto timeTaken = static_cast<double>(duration.count());

  ispd::node_metrics::notifyMetric(ispd::metrics::NodeMetricsFlag::NODE_MACHINE_REVERSE_TIME, timeTaken);
#endif // DEBUG_ON
  }

  static void commit(machine_state *s, tw_bf *bf, ispd_message *msg, tw_lp *lp) {
    if (msg->task.dest == lp->gid) {
      /// Fetch the processing size and calculates the processing time.
      const double proc_size = msg->task.proc_size;
      const double proc_time = s->conf.timeToProcess(proc_size);

      std::cout << "Processing Time: " << proc_time << std::endl;

      const double least_free_time = msg->saved_core_next_available_time;
      const double waiting_delay = ROSS_MAX(0.0, least_free_time - tw_now(lp));

      /// Update the user's metrics.
      ispd::metrics::UserMetrics& userMetrics = ispd::this_model::getUserById(msg->task.owner).getMetrics();

      userMetrics.m_ProcTime += proc_time;
      userMetrics.m_ProcWaitingTime += waiting_delay;
      userMetrics.m_CompletedTasks++;
    }
  }

  static void finish(machine_state *s, tw_lp *lp) {
    const double lastActivityTime = *std::max_element(s->cores_free_time.cbegin(), s->cores_free_time.cend());
    const double totalCpuTime = std::accumulate(s->cores_free_time.cbegin(), s->cores_free_time.cend(), 0.0);
    const double idleness = (totalCpuTime - s->metrics.proc_time) / totalCpuTime;

    /// Report to the node`s metrics collector this machine`s metrics.
    ispd::node_metrics::notifyMetric(ispd::metrics::NodeMetricsFlag::NODE_SIMULATION_TIME, lastActivityTime);
    ispd::node_metrics::notifyMetric(ispd::metrics::NodeMetricsFlag::NODE_TOTAL_PROCESSED_MFLOPS, s->metrics.proc_mflops);
    ispd::node_metrics::notifyMetric(ispd::metrics::NodeMetricsFlag::NODE_TOTAL_PROCESSING_WAITING_TIME, s->metrics.waiting_time);
    ispd::node_metrics::notifyMetric(ispd::metrics::NodeMetricsFlag::NODE_TOTAL_MACHINE_SERVICES);
    ispd::node_metrics::notifyMetric(ispd::metrics::NodeMetricsFlag::NODE_TOTAL_COMPUTATIONAL_POWER, s->conf.getPower());
    ispd::node_metrics::notifyMetric(ispd::metrics::NodeMetricsFlag::NODE_TOTAL_CPU_CORES, static_cast<unsigned>(s->cores_free_time.size()));
    ispd::node_metrics::notifyMetric(ispd::metrics::NodeMetricsFlag::NODE_TOTAL_PROCESSING_TIME, s->metrics.proc_time);

    std::printf(
        "Machine Metrics (%lu)\n"
        " - Last Activity Time..: %lf seconds (%lu).\n"
        " - Processed MFLOPS....: %lf MFLOPS (%lu).\n"
        " - Processed Tasks.....: %u tasks (%lu).\n"
        " - Forwarded Packets...: %u packets (%lu).\n"
        " - Waiting Time........: %lf seconds (%lu).\n"
        " - Avg. Processing Time: %lf seconds (%lu).\n"
        " - Idleness............: %lf%% (%lu).\n"
        "\n",
        lp->gid, 
        lastActivityTime, lp->gid,
        s->metrics.proc_mflops, lp->gid,
        s->metrics.proc_tasks, lp->gid,
        s->metrics.forwarded_packets, lp->gid,
        s->metrics.waiting_time, lp->gid,
        s->metrics.proc_time / s->metrics.proc_tasks, lp->gid,
        idleness * 100.0, lp->gid
    );
  }
};

}; // namespace services
}; // namespace ispd

#endif // ISPD_SERVICE_MACHINE_HPP
