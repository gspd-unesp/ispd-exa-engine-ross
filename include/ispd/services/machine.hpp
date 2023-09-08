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

struct machine_prices {
  double cpu_individual_cost;
  double memory_indivual_cost ;
  double storage_individual_cost ;
};

struct machine_state {
  ispd::configuration::MachineConfiguration conf; ///< Machine's configuration.
  ispd::metrics::MachineMetrics m_Metrics;        ///< Machine's metrics.
  std::vector<double> cores_free_time; ///< Machine's queueing model information
  machine_prices prices;
  std::vector<tw_lpid> vms; /// vms allocated in this machine
};

struct machine {

  static double least_core_time(const std::vector<double> &cores_free_time,
                                unsigned &core_index) {
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
    const auto &service_initializer =
        ispd::this_model::getServiceInitializer(lp->gid);

    /// Call the service initializer for this logical process.
    service_initializer(s);

    s->prices.cpu_individual_cost = 10;
    s->prices.storage_individual_cost = 10;
    s->prices.memory_indivual_cost = 10;

    s->m_Metrics.m_Total_cpu_cost  = 0;
    s->m_Metrics.m_Total_storage_cost = 0;
    s->m_Metrics.m_Total_memory_cost = 0;

    /// Print a debug message.
    ispd_debug("Machine %lu has been initialized.", lp->gid);
  }

  static void forward(machine_state *s, tw_bf *bf, ispd_message *msg,
                      tw_lp *lp) {
    ispd_debug("[Forward] Machine %lu received a message at %lf of type (%d) "
               "and route offset (%u).",
               lp->gid, tw_now(lp), msg->type, msg->route_offset);

    if (msg->is_vm) {
      forward_vm(s, bf, msg, lp);
      return;
    }

#ifdef DEBUG_ON
    const auto start = std::chrono::high_resolution_clock::now();
#endif // DEBUG_ON

    /// Checks if the task's destination is this machine. If so, the task is
    /// processed and the task's results is sent back to the master by the same
    /// route it came along.
    if (msg->task.dest == lp->gid) {

      /// the message is for the vm allocated in this machine
      if(msg->vm_sent > 0 &&  msg->is_vm == 0)
      {
        sent_to_vm(s,bf,msg,lp);
        return;
      }
      /// Fetch the processing size and calculates the processing time.
      const double proc_size = msg->task.proc_size;
      const double proc_time = s->conf.timeToProcess(proc_size);

      unsigned core_index;
      const double least_free_time =
          least_core_time(s->cores_free_time, core_index);
      const double waiting_delay = ROSS_MAX(0.0, least_free_time - tw_now(lp));
      const double departure_delay = waiting_delay + proc_time;

      /// Update the machine's metrics.
      s->m_Metrics.m_ProcMflops += proc_size;
      s->m_Metrics.m_ProcTime += proc_time;
      s->m_Metrics.m_ProcTasks++;
      s->m_Metrics.m_ProcWaitingTime += waiting_delay;
      s->m_Metrics.m_EnergyConsumption +=
          proc_time * s->conf.getWattagePerCore();

      /// Update the machine's queueing model information.
      s->cores_free_time[core_index] = tw_now(lp) + departure_delay;

      tw_event *const e =
          tw_event_new(msg->previous_service_id, departure_delay, lp);

      ispd_message *const m = static_cast<ispd_message *>(tw_event_data(e));

      *m = *msg;
      m->type = message_type::ARRIVAL;
      m->task = msg->task;             /// Copy the task's information.
      m->task.comm_size = 0.000976562; /// 1 Kib (representing the results).
      m->task_processed =
          1; /// Indicate that the message is carrying a processed task.
      m->downward_direction =
          0; /// The task's results will be sent back to the master.
      m->route_offset = msg->route_offset - 2;
      m->previous_service_id = lp->gid;

      /// Save information (for reverse computation).
      msg->saved_core_index = core_index;
      msg->saved_core_next_available_time = least_free_time;

      tw_event_send(e);
    }
    /// Otherwise, this indicates that the task's destination IS NOT this
    /// machine and, therefore, the task should only be forwarded to its next
    /// destination.
    else {
      /// Fetch the route between the task's origin and task's destination.
      const ispd::routing::Route *route =
          ispd::routing_table::getRoute(msg->task.origin, msg->task.dest);

      /// Update machine's metrics.
      s->m_Metrics.m_ForwardedTasks++;

      /// @Todo: This zero-delay timestamped message could affect the
      /// conservative synchronization.
      ///        This should be changed after.
      tw_event *const e = tw_event_new(route->get(msg->route_offset), 0.0, lp);
      ispd_message *const m = static_cast<ispd_message *>(tw_event_data(e));

      m->type = message_type::ARRIVAL;
      m->task = msg->task; /// Copy the tasks's information.
      m->task_processed = msg->task_processed;
      m->downward_direction = msg->downward_direction;
      m->route_offset = msg->downward_direction ? (msg->route_offset + 1)
                                                : (msg->route_offset - 1);
      m->previous_service_id = lp->gid;

      tw_event_send(e);
    }
#ifdef DEBUG_ON
    const auto end = std::chrono::high_resolution_clock::now();
    const auto duration =
        std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    const auto timeTaken = static_cast<double>(duration.count());

    ispd::node_metrics::notifyMetric(
        ispd::metrics::NodeMetricsFlag::NODE_MACHINE_FORWARD_TIME, timeTaken);
#endif // DEBUG_ON
  }

  static void reverse(machine_state *s, tw_bf *bf, ispd_message *msg,
                      tw_lp *lp) {
    ispd_debug("[Reverse] Machine %lu received a message at %lf of type (%d).",
               lp->gid, tw_now(lp), msg->type);

    if (msg->is_vm) {
      reverse_vm(s, bf, msg, lp);
      return;
    }
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
      s->m_Metrics.m_ProcMflops -= proc_size;
      s->m_Metrics.m_ProcTime -= proc_time;
      s->m_Metrics.m_ProcTasks--;
      s->m_Metrics.m_ProcWaitingTime -= waiting_delay;
      s->m_Metrics.m_EnergyConsumption -=
          proc_time * s->conf.getWattagePerCore();

      /// Reverse the machine's queueing model information.
      s->cores_free_time[msg->saved_core_index] = least_free_time;
    } else {
      /// Reverse machine's metrics.
      s->m_Metrics.m_ForwardedTasks--;
    }

#ifdef DEBUG_ON
    const auto end = std::chrono::high_resolution_clock::now();
    const auto duration =
        std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    const auto timeTaken = static_cast<double>(duration.count());

    ispd::node_metrics::notifyMetric(
        ispd::metrics::NodeMetricsFlag::NODE_MACHINE_REVERSE_TIME, timeTaken);
#endif // DEBUG_ON
  }

  static void commit(machine_state *s, tw_bf *bf, ispd_message *msg,
                     tw_lp *lp) {
    if (msg->task.dest == lp->gid) {
      /// Fetch the processing size and calculates the processing time.
      const double proc_size = msg->task.proc_size;
      const double proc_time = s->conf.timeToProcess(proc_size);

      const double least_free_time = msg->saved_core_next_available_time;
      const double waiting_delay = ROSS_MAX(0.0, least_free_time - tw_now(lp));

      /// Calculates the energy consumption by processing this task.
      const double energyConsumption =
          proc_time * (s->conf.getWattageIdle() + s->conf.getWattagePerCore());

      /// Update the user's metrics.
      ispd::metrics::UserMetrics &userMetrics =
          ispd::this_model::getUserById(msg->task.owner).getMetrics();

      userMetrics.m_ProcTime += proc_time;
      userMetrics.m_ProcWaitingTime += waiting_delay;
      userMetrics.m_CompletedTasks++;
      userMetrics.m_EnergyConsumption += energyConsumption;
    }
  }

  static void finish(machine_state *s, tw_lp *lp) {
    const double lastActivityTime = *std::max_element(
        s->cores_free_time.cbegin(), s->cores_free_time.cend());
    const double totalCpuTime = std::accumulate(s->cores_free_time.cbegin(),
                                                s->cores_free_time.cend(), 0.0);
    const double idleness =
        (totalCpuTime - s->m_Metrics.m_ProcTime) / totalCpuTime;

    /// Report to the node`s metrics collector this machine`s metrics.
    ispd::node_metrics::notifyMetric(
        ispd::metrics::NodeMetricsFlag::NODE_SIMULATION_TIME, lastActivityTime);
    ispd::node_metrics::notifyMetric(
        ispd::metrics::NodeMetricsFlag::NODE_TOTAL_PROCESSED_MFLOPS,
        s->m_Metrics.m_ProcMflops);
    ispd::node_metrics::notifyMetric(
        ispd::metrics::NodeMetricsFlag::NODE_TOTAL_PROCESSING_WAITING_TIME,
        s->m_Metrics.m_ProcWaitingTime);
    ispd::node_metrics::notifyMetric(
        ispd::metrics::NodeMetricsFlag::NODE_TOTAL_MACHINE_SERVICES);
    ispd::node_metrics::notifyMetric(
        ispd::metrics::NodeMetricsFlag::NODE_TOTAL_COMPUTATIONAL_POWER,
        s->conf.getPower());
    ispd::node_metrics::notifyMetric(
        ispd::metrics::NodeMetricsFlag::NODE_TOTAL_CPU_CORES,
        static_cast<unsigned>(s->cores_free_time.size()));
    ispd::node_metrics::notifyMetric(
        ispd::metrics::NodeMetricsFlag::NODE_TOTAL_PROCESSING_TIME,
        s->m_Metrics.m_ProcTime);
    ispd::node_metrics::notifyMetric(
        ispd::metrics::NodeMetricsFlag::NODE_TOTAL_NON_IDLE_ENERGY_CONSUMPTION,
        s->m_Metrics.m_EnergyConsumption);
    ispd::node_metrics::notifyMetric(
        ispd::metrics::NodeMetricsFlag::NODE_TOTAL_POWER_IDLE,
        s->conf.getWattageIdle());

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
        " - Allocated vms.......: %u vms (%lu).\n"
        " - Total cpu cost......: %lf (%lu). \n"
        " - Total memory cost...: %lf (%lu). \n"
        " - Total storage cost..: %lf (%lu). \n"
        "\n",
        lp->gid, lastActivityTime, lp->gid, s->m_Metrics.m_ProcMflops, lp->gid,
        s->m_Metrics.m_ProcTasks, lp->gid, s->m_Metrics.m_ForwardedTasks,
        lp->gid, s->m_Metrics.m_ProcWaitingTime, lp->gid,
        s->m_Metrics.m_ProcTime / s->m_Metrics.m_ProcTasks, lp->gid,
        idleness * 100.0, lp->gid, s->m_Metrics.m_EnergyConsumption, lp->gid
        , s->m_Metrics.m_allocated_vms, lp->gid
        , s->m_Metrics.m_Total_cpu_cost, lp->gid,
        s->m_Metrics.m_Total_memory_cost, lp->gid,
        s->m_Metrics.m_Total_storage_cost, lp->gid);

  }

private:
  static void forward_vm(machine_state *s, tw_bf *bf, ispd_message *msg,
                         tw_lp *lp) {
    ispd_debug("Machine [%lu] received an vm %lu to allocate", lp->gid,
               msg->vm_sent);
    if (msg->task.dest == lp->gid) {

      bool fit = false;
      double vm_memory = msg->vm_memory_space;
      double vm_storage = msg->vm_disk_space;
      unsigned vm_cores = msg->vm_num_cores;

      if (vm_memory <= s->conf.getAvaliableMemory() &&
          vm_storage <= s->conf.getAvaliableDisk() &&
          vm_cores <= s->conf.getNumCores()) {
        fit = true;

        s->m_Metrics.m_allocated_vms++;
        s->m_Metrics.m_Total_cpu_cost += s->prices.cpu_individual_cost * vm_cores;
        s->m_Metrics.m_Total_memory_cost +=
            s->prices.memory_indivual_cost * vm_memory;
        s->m_Metrics.m_Total_storage_cost+=
            s->prices.storage_individual_cost * vm_storage;

        s->conf.setAvaliableMemory(s->conf.getAvaliableMemory() - vm_memory);
        s->conf.setAvaliableDisk(s->conf.getAvaliableDisk() - vm_storage);
        s->conf.setNumCores(s->conf.getNumCores() - vm_cores);

        s->vms.push_back(msg->vm_sent);
      }
      const double proc_size = msg->task.proc_size;
      const double proc_time = s->conf.timeToProcess(proc_size);

      unsigned core_index;
      const double least_free_time =
          least_core_time(s->cores_free_time, core_index);
      const double waiting_delay = ROSS_MAX(0.0, least_free_time - tw_now(lp));
      const double departure_delay = waiting_delay + proc_time;

      /// Update the machine's metrics.
      s->m_Metrics.m_ProcMflops += proc_size;
      s->m_Metrics.m_ProcTime += proc_time;
      s->m_Metrics.m_ProcWaitingTime += waiting_delay;
      s->m_Metrics.m_EnergyConsumption +=
          proc_time * s->conf.getWattagePerCore();

      /// Update the machine's queueing model information.
      s->cores_free_time[core_index] = tw_now(lp) + departure_delay;

      /// sends an ack directly to the VMM without passing the link
      tw_event *const e = tw_event_new(msg->task.origin, departure_delay, lp);
      ispd_debug("Sending an ack message from %lu to %lu", lp->gid,
                 msg->task.origin);
      ispd_message *const m = static_cast<ispd_message *>(tw_event_data(e));
      m->type = message_type::ARRIVAL;
      m->task = msg->task; /// Copy the tasks's information.
      m->task_processed = msg->task_processed;
      m->downward_direction = msg->downward_direction;
      m->route_offset = msg->downward_direction ? (msg->route_offset + 1)
                                                : (msg->route_offset - 1);
      m->previous_service_id = lp->gid;
      m->fit = fit;
      m->is_vm = msg->is_vm;
      m->allocated_in = lp->gid;
      m->vm_sent = msg->vm_sent;
      m->vm_disk_space = msg->vm_disk_space;
      m->vm_num_cores = msg->vm_num_cores;
      m->vm_memory_space = msg->vm_memory_space;
      tw_event_send(e);

    } else {
      /// Fetch the route between the task's origin and task's destination.
      const ispd::routing::Route *route =
          ispd::routing_table::getRoute(msg->task.origin, msg->task.dest);

      /// Update machine's metrics.
      s->m_Metrics.m_ForwardedTasks++;

      /// @Todo: This zero-delay timestamped message could affect the
      /// conservative synchronization.
      ///        This should be changed after.
      tw_event *const e = tw_event_new(route->get(msg->route_offset), 0.0, lp);
      ispd_message *const m = static_cast<ispd_message *>(tw_event_data(e));

      m->type = message_type::ARRIVAL;
      m->task = msg->task; /// Copy the tasks's information.
      m->task_processed = msg->task_processed;
      m->downward_direction = msg->downward_direction;
      m->route_offset = msg->downward_direction ? (msg->route_offset + 1)
                                                : (msg->route_offset - 1);
      m->previous_service_id = lp->gid;
      m->vm_sent = msg->vm_sent;
      m->vm_disk_space = msg->vm_disk_space;
      m->vm_num_cores = msg->vm_num_cores;
      m->vm_memory_space = msg->vm_memory_space;

      tw_event_send(e);
    }
  }

  static void reverse_vm(machine_state *s, tw_bf *bf, ispd_message *msg,
                         tw_lp *lp) {

    if (msg->task.dest == lp->gid) {

      bool fit = false;
      double vm_memory = msg->vm_memory_space;
      double vm_storage = msg->vm_disk_space;
      unsigned vm_cores = msg->vm_num_cores;

      if (vm_memory < s->conf.getAvaliableMemory() ||
          vm_storage < s->conf.getAvaliableDisk() ||
          vm_cores < s->conf.getNumCores()) {
        fit = true;

        s->m_Metrics.m_allocated_vms--;
        s->m_Metrics.m_Total_cpu_cost -= s->prices.cpu_individual_cost * vm_cores;
        s->m_Metrics.m_Total_memory_cost -=
            s->prices.memory_indivual_cost * vm_memory;
        s->m_Metrics.m_Total_storage_cost-=
            s->prices.storage_individual_cost * vm_storage;

        s->conf.setAvaliableMemory(s->conf.getAvaliableMemory() - vm_memory);
        s->conf.setAvaliableDisk(s->conf.getAvaliableDisk() - vm_storage);
        s->conf.setNumCores(s->conf.getNumCores() - vm_cores);

        s->vms.pop_back();

      }
      const double proc_size = msg->task.proc_size;
      const double proc_time = s->conf.timeToProcess(proc_size);

      unsigned core_index;
      const double least_free_time =
          least_core_time(s->cores_free_time, core_index);
      const double waiting_delay = ROSS_MAX(0.0, least_free_time - tw_now(lp));
      const double departure_delay = waiting_delay + proc_time;

      /// Update the machine's metrics.
      s->m_Metrics.m_ProcMflops -= proc_size;
      s->m_Metrics.m_ProcTime -= proc_time;
      s->m_Metrics.m_ProcWaitingTime -= waiting_delay;
      s->m_Metrics.m_EnergyConsumption -=
          proc_time * s->conf.getWattagePerCore();

      /// Update the machine's queueing model information.
      s->cores_free_time[msg->saved_core_index] = least_free_time;

    } else
      s->m_Metrics.m_ForwardedTasks--;
  }


  /**
   * Forward the message to the virtual machine hosted by this machine.
   */
  static void sent_to_vm(machine_state *s, tw_bf *bf, ispd_message *msg, tw_lp *lp)
  {
    auto iter = std::find(s->vms.begin(), s->vms.end(), msg->vm_sent);
    if (iter != s->vms.end()){
      /// iter exists thus it's safe to send the vm
      tw_event *const e = tw_event_new(msg->vm_sent, 0.0, lp);
      ispd_message *const m = static_cast<ispd_message *>(tw_event_data(e));
      *m = *msg;
      m->task = msg->task;
      m->task_processed = 0;

      ispd_debug("Task will be sent to %lu", msg->vm_sent);
      tw_event_send(e);

    }
    else
      ispd_error("Virtual machine %lu not found on machine %lu", msg->vm_sent, lp->gid);
  }
};

}; // namespace services
}; // namespace ispd

#endif // ISPD_SERVICE_MACHINE_HPP
