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
#include <ispd/metrics/machine_metrics.hpp>
#include <ispd/configuration/machine.hpp>

extern double g_NodeSimulationTime;

namespace ispd {
namespace services {

struct machine_state {
  ispd::configuration::MachineConfiguration conf; ///< Machine's configuration.
  ispd::metrics::MachineMetrics m_Metrics; ///< Machine's metrics.
  std::vector<double> cores_free_time; ///< Machine's queueing model information
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
    if (msg->task.m_Dest == lp->gid) {
      /// Fetch the processing size and calculates the processing time.
      const double proc_size = msg->task.m_ProcSize;
      const double proc_time = s->conf.timeToProcess(proc_size, msg->task.m_CommSize, msg->task.m_Offload);

      unsigned core_index;
      const double least_free_time = least_core_time(s->cores_free_time, core_index);
      const double waiting_delay = ROSS_MAX(0.0, least_free_time - tw_now(lp));
      const double departure_delay = waiting_delay + proc_time;

      /// Update the machine's metrics.
      s->m_Metrics.m_ProcMflops += proc_size;
      s->m_Metrics.m_ProcTime += proc_time;
      s->m_Metrics.m_ProcTasks++;
      s->m_Metrics.m_ProcWaitingTime += waiting_delay;
      s->m_Metrics.m_EnergyConsumption += proc_time * s->conf.getWattagePerCore();

      /// Update the machine's queueing model information.
      s->cores_free_time[core_index] = tw_now(lp) + departure_delay;

      tw_event *const e = tw_event_new(msg->previous_service_id, g_tw_lookahead + departure_delay, lp);
      ispd_message *const m = static_cast<ispd_message *>(tw_event_data(e));

      *m = *msg;
      m->type = message_type::ARRIVAL;
      m->task = msg->task;             /// Copy the task's information.
      m->task.m_CommSize = 0.000976562; /// 1 Kib (representing the results).
      m->task_processed = 1;           /// Indicate that the message is carrying a processed task.
      m->downward_direction = 0;       /// The task's results will be sent back to the master.
      m->route_offset = msg->route_offset - 2;
      m->previous_service_id = lp->gid;
      m->service_id = lp->gid;
      
      /// Save information (for reverse computation).
      msg->saved_core_index = core_index;
      msg->saved_core_next_available_time = least_free_time;

      tw_event_send(e);
    }
    /// Otherwise, this indicates that the task's destination IS NOT this machine and, therefore,
    /// the task should only be forwarded to its next destination. 
    else {
      /// Fetch the route between the task's origin and task's destination.
      const ispd::routing::Route *route = ispd::routing_table::getRoute(msg->task.m_Origin, msg->task.m_Dest);

      /// Update machine's metrics.
      s->m_Metrics.m_ForwardedTasks++;

      /// @Todo: This zero-delay timestamped message could affect the conservative synchronization.
      ///        This should be changed after.
      tw_event *const e = tw_event_new(route->get(msg->route_offset), g_tw_lookahead, lp);
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
    if (msg->task.m_Dest == lp->gid) {
      const double proc_size = msg->task.m_ProcSize;
      const double proc_time = s->conf.timeToProcess(proc_size, msg->task.m_CommSize, msg->task.m_Offload);

      const double least_free_time = msg->saved_core_next_available_time;
      const double waiting_delay = ROSS_MAX(0.0, least_free_time - tw_now(lp));

      /// Reverse the machine's metrics.
      s->m_Metrics.m_ProcMflops -= proc_size;
      s->m_Metrics.m_ProcTime -= proc_time;
      s->m_Metrics.m_ProcTasks--;
      s->m_Metrics.m_ProcWaitingTime -= waiting_delay;
      s->m_Metrics.m_EnergyConsumption -= proc_time * s->conf.getWattagePerCore();

      /// Reverse the machine's queueing model information.
      s->cores_free_time[msg->saved_core_index] = least_free_time;
    } else {
      /// Reverse machine's metrics.
      s->m_Metrics.m_ForwardedTasks--;
    }

#ifdef DEBUG_ON
  const auto end = std::chrono::high_resolution_clock::now();
  const auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
  const auto timeTaken = static_cast<double>(duration.count());

  ispd::node_metrics::notifyMetric(ispd::metrics::NodeMetricsFlag::NODE_MACHINE_REVERSE_TIME, timeTaken);
#endif // DEBUG_ON
  }

  static void commit(machine_state *s, tw_bf *bf, ispd_message *msg, tw_lp *lp) {
    if (msg->task.m_Dest == lp->gid) {
      /// Fetch the processing size and calculates the processing time.
      const double proc_size = msg->task.m_ProcSize;
      const double proc_time = s->conf.timeToProcess(proc_size, msg->task.m_CommSize, msg->task.m_Offload);

      const double least_free_time = msg->saved_core_next_available_time;
      const double waiting_delay = ROSS_MAX(0.0, least_free_time - tw_now(lp));

      /// Calculates the energy consumption by processing this task.
      const double energyConsumption = proc_time * (s->conf.getWattageIdle() + s->conf.getWattagePerCore());

      /// Update the user's metrics.
      ispd::metrics::UserMetrics& userMetrics = ispd::this_model::getUserById(msg->task.m_Owner).getMetrics();

      userMetrics.m_ProcTime += proc_time;
      userMetrics.m_ProcWaitingTime += waiting_delay;
      userMetrics.m_CompletedTasks++;
      userMetrics.m_EnergyConsumption += energyConsumption;
    }
  }

  static void finish(machine_state *s, tw_lp *lp) {
    const double lastActivityTime = *std::max_element(s->cores_free_time.cbegin(), s->cores_free_time.cend());
    const double totalCpuTime = std::accumulate(s->cores_free_time.cbegin(), s->cores_free_time.cend(), 0.0);
    const double idleness = (totalCpuTime - s->m_Metrics.m_ProcTime) / totalCpuTime;

    /// Finish the machine's metrics.
    s->m_Metrics.m_Idleness = idleness;

    /// Report to the node`s metrics collector this machine`s metrics.
    ispd::node_metrics::notifyMetric(ispd::metrics::NodeMetricsFlag::NODE_SIMULATION_TIME, lastActivityTime);
    ispd::node_metrics::notifyMetric(ispd::metrics::NodeMetricsFlag::NODE_TOTAL_PROCESSED_MFLOPS, s->m_Metrics.m_ProcMflops);
    ispd::node_metrics::notifyMetric(ispd::metrics::NodeMetricsFlag::NODE_TOTAL_PROCESSING_WAITING_TIME, s->m_Metrics.m_ProcWaitingTime);
    ispd::node_metrics::notifyMetric(ispd::metrics::NodeMetricsFlag::NODE_TOTAL_MACHINE_SERVICES);
    ispd::node_metrics::notifyMetric(ispd::metrics::NodeMetricsFlag::NODE_TOTAL_COMPUTATIONAL_POWER, s->conf.getPower() + s->conf.getGpuPower());
    ispd::node_metrics::notifyMetric(ispd::metrics::NodeMetricsFlag::NODE_TOTAL_CPU_CORES, s->conf.getCoreCount());
    ispd::node_metrics::notifyMetric(ispd::metrics::NodeMetricsFlag::NODE_TOTAL_GPU_CORES, s->conf.getGpuCoreCount());
    ispd::node_metrics::notifyMetric(ispd::metrics::NodeMetricsFlag::NODE_TOTAL_PROCESSING_TIME, s->m_Metrics.m_ProcTime);
    ispd::node_metrics::notifyMetric(ispd::metrics::NodeMetricsFlag::NODE_TOTAL_NON_IDLE_ENERGY_CONSUMPTION, s->m_Metrics.m_EnergyConsumption);
    ispd::node_metrics::notifyMetric(ispd::metrics::NodeMetricsFlag::NODE_TOTAL_POWER_IDLE, s->conf.getWattageIdle());

    /// Report to the node's metrics reports file this machine's metrics.
    ispd::node_metrics::notifyReport(s->m_Metrics, s->conf, lp->gid);

    std::printf(
        "Machine Metrics (%lu)\n"
        " - Last Activity Time..: %lf seconds (%lu).\n"
        " - Processed MFLOPS....: %lf MFLOPS (%lu).\n"
        " - Processed Tasks.....: %u tasks (%lu).\n"
        " - Forwarded Packets...: %u packets (%lu).\n"
        " - Waiting Time........: %lf seconds (%lu).\n"
        " - Avg. Processing Time: %lf seconds (%lu).\n"
        " - Idleness............: %lf%% (%lu).\n"
        " - Non Idle Energy Cons: %lf J (%lu).\n"
        "\n",
        lp->gid, 
        lastActivityTime, lp->gid,
        s->m_Metrics.m_ProcMflops, lp->gid,
        s->m_Metrics.m_ProcTasks, lp->gid,
        s->m_Metrics.m_ForwardedTasks, lp->gid,
        s->m_Metrics.m_ProcWaitingTime, lp->gid,
        s->m_Metrics.m_ProcTime / s->m_Metrics.m_ProcTasks, lp->gid,
        s->m_Metrics.m_Idleness * 100.0, lp->gid,
        s->m_Metrics.m_EnergyConsumption, lp->gid
    );
  }
};

}; // namespace services
}; // namespace ispd

#endif // ISPD_SERVICE_MACHINE_HPP
