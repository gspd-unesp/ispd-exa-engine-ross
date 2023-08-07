#include <mpi.h>
#include <ross.h>
#include <algorithm>
#include <ispd/log/log.hpp>
#include <ispd/metrics/metrics.hpp>

namespace ispd::metrics {

template <>
void NodeMetricsCollector::notifyMetric(const NodeMetricsFlag flag, const double value) {
  switch (flag) {
    case NodeMetricsFlag::NODE_TOTAL_PROCESSED_MFLOPS:
      /// Updates total processed MFLOPS.
      m_NodeTotalProcessedMFlops += value;
      break;
    case NodeMetricsFlag::NODE_TOTAL_COMMUNICATED_MBITS:
      /// Updates total communicated MBits.
      m_NodeTotalCommunicatedMBits += value;
      break;
    case NodeMetricsFlag::NODE_TOTAL_PROCESSING_WAITING_TIME:
      /// Updates total processing waiting time.
      m_NodeTotalProcessingWaitingTime += value;
      break;
    case NodeMetricsFlag::NODE_TOTAL_COMMUNICATION_WAITING_TIME:
      /// Updates total communication waiting time.
      m_NodeTotalCommunicationWaitingTime += value;
      break;
    case NodeMetricsFlag::NODE_TOTAL_COMPUTATIONAL_POWER:
      /// Updates total computational power.
      m_NodeTotalComputationalPower += value;
    case NodeMetricsFlag::NODE_TOTAL_PROCESSING_TIME:
      /// Updates total processing time.
      m_NodeTotalProcessingTime += value;
      break;
     case NodeMetricsFlag::NODE_TOTAL_COMMUNICATION_TIME:
      /// Updates total communication time.
      m_NodeTotalCommunicationTime += value;
      break;
    case NodeMetricsFlag::NODE_TOTAL_TURNAROUND_TIME:
      /// Updates total turnaround time.
      m_NodeTotalTurnaroundTime += value;
      break;
    case NodeMetricsFlag::NODE_SIMULATION_TIME:
      /// Updates simulation time.
      m_NodeSimulationTime = std::max(m_NodeSimulationTime, value);
      break;
    default:
      ispd_error("Unknown node metrics flag (%d) or incorrect argument type.", flag);
  }
}

template <>
void NodeMetricsCollector::notifyMetric(const enum NodeMetricsFlag flag, const unsigned value) {
  switch (flag) {
    case NodeMetricsFlag::NODE_TOTAL_COMPLETED_TASKS:
      /// Updates total completed tasks.
      m_NodeTotalCompletedTasks += value;
      break;
    case NodeMetricsFlag::NODE_TOTAL_CPU_CORES:
      /// Updates the total CPU cores.
      m_NodeTotalCpuCores += value;
      break;
    default:
      ispd_error("Unknown node metrics flag (%d) or it may be the case the flag is correct but the argument is not of the required type.", flag);
  }
}

void NodeMetricsCollector::notifyMetric(const enum NodeMetricsFlag flag) {
  switch (flag) {
     case NodeMetricsFlag::NODE_TOTAL_MASTER_SERVICES:
      /// Updates the count of master services.
      m_NodeTotalMasterServices++;
      break;
     case NodeMetricsFlag::NODE_TOTAL_LINK_SERVICES:
      /// Updates the count of link services.
      m_NodeTotalLinkServices++;
      break;
     case NodeMetricsFlag::NODE_TOTAL_MACHINE_SERVICES:
      /// Updates the count of machine services.
      m_NodeTotalMachineServices++;
      break;
     case NodeMetricsFlag::NODE_TOTAL_SWITCH_SERVICES:
      /// Updates the count of switch services.
      m_NodeTotalSwitchServices++;
      break;
    default:
      ispd_error("Unknown node metrics flag (%d) or it may be the case the flag is correct but the argument is not of the required type.", flag);
    }
}

void NodeMetricsCollector::reportNodeMetrics() {
  /// An alias for the global metrics collector.
  auto gmc = ispd::global_metrics::g_GlobalMetricsCollector;

  /// Report to the master node the simulation time of this node.
  if (MPI_SUCCESS != MPI_Reduce(&m_NodeSimulationTime, &gmc->m_GlobalSimulationTime, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_ROSS))
    ispd_error("Global simulation time could not be reduced, exiting...");

  /// Report to the master node the processed MFlops of this node.
  if (MPI_SUCCESS != MPI_Reduce(&m_NodeTotalProcessedMFlops, &gmc->m_GlobalTotalProcessedMFlops, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_ROSS))
    ispd_error("Global total processed mflops could not be reduced, exiting...");

  /// Report to the master node the processed MFlops of this node.
  if (MPI_SUCCESS != MPI_Reduce(&m_NodeTotalCommunicatedMBits, &gmc->m_GlobalTotalCommunicatedMBits, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_ROSS))
    ispd_error("Global total communicated mbits could not be reduced, exiting...");

  /// Report to the master node the total processing waiting time.
  if (MPI_SUCCESS != MPI_Reduce(&m_NodeTotalProcessingWaitingTime, &gmc->m_GlobalTotalProcessingWaitingTime, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_ROSS))
    ispd_error("Global total processing waiting time could not be reduced, exiting...");

  /// Report to the master node the total communication waiting time.
  if (MPI_SUCCESS != MPI_Reduce(&m_NodeTotalCommunicationWaitingTime, &gmc->m_GlobalTotalCommunicationWaitingTime, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_ROSS))
    ispd_error("Global total communication waiting time could not be reduced, exiting...");

  /// Report to the master node the total master services count.
  if (MPI_SUCCESS != MPI_Reduce(&m_NodeTotalMasterServices, &gmc->m_GlobalTotalMasterServices, 1, MPI_UNSIGNED, MPI_SUM, 0, MPI_COMM_ROSS))
    ispd_error("Global total master services could not be reduced, exiting...");
 
  /// Report to the master node the total link services count.
  if (MPI_SUCCESS != MPI_Reduce(&m_NodeTotalLinkServices, &gmc->m_GlobalTotalLinkServices, 1, MPI_UNSIGNED, MPI_SUM, 0, MPI_COMM_ROSS))
    ispd_error("Global total link services could not be reduced, exiting...");
 
  /// Report to the master node the total machine services count.
  if (MPI_SUCCESS != MPI_Reduce(&m_NodeTotalMachineServices, &gmc->m_GlobalTotalMachineServices, 1, MPI_UNSIGNED, MPI_SUM, 0, MPI_COMM_ROSS))
    ispd_error("Global total machine services could not be reduced, exiting...");
 
  /// Report to the master node the total switch services count.
  if (MPI_SUCCESS != MPI_Reduce(&m_NodeTotalSwitchServices, &gmc->m_GlobalTotalSwitchServices, 1, MPI_UNSIGNED, MPI_SUM, 0, MPI_COMM_ROSS))
    ispd_error("Global total switch services could not be reduced, exiting...");
 
  /// Report to the master node the total completed tasks.
  if (MPI_SUCCESS != MPI_Reduce(&m_NodeTotalCompletedTasks, &gmc->m_GlobalTotalCompletedTasks, 1, MPI_UNSIGNED, MPI_SUM, 0, MPI_COMM_ROSS))
    ispd_error("Global total completed tasks could not be reduced, exiting...");

  /// Report to the master node the total computational power.
  if (MPI_SUCCESS != MPI_Reduce(&m_NodeTotalComputationalPower, &gmc->m_GlobalTotalComputationalPower, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_ROSS))
    ispd_error("Global total computational power could not be reduced, exiting...");

  /// Report to the master node the total CPU cores.
  if (MPI_SUCCESS != MPI_Reduce(&m_NodeTotalCpuCores, &gmc->m_GlobalTotalCpuCores, 1, MPI_UNSIGNED, MPI_SUM, 0, MPI_COMM_ROSS))
    ispd_error("Global total cpu cores could not be reduced, exiting...");

  /// Report to the master node the processing time.
  if (MPI_SUCCESS != MPI_Reduce(&m_NodeTotalProcessingTime, &gmc->m_GlobalTotalProcessingTime, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_ROSS))
    ispd_error("Global total processing time could not be reduced, exiting...");

  /// Report to the master node the communication size.
  if (MPI_SUCCESS != MPI_Reduce(&m_NodeTotalCommunicationTime, &gmc->m_GlobalTotalCommunicationTime, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_ROSS))
    ispd_error("Global total communication time could not be reduced, exiting...");

  /// Report to the master node the turnaround time.
  if (MPI_SUCCESS != MPI_Reduce(&m_NodeTotalTurnaroundTime, &gmc->m_GlobalTotalTurnaroundTime, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_ROSS))
    ispd_error("Global total turnaround time could not be reduced, exiting...");
}

void GlobalMetricsCollector::reportGlobalMetrics() {
  /// Check if the current node is not the master one. If so, the global metrics will
  /// not be reported, since only the master node will report the global metrics.
  if (g_tw_mynode)
    return;

  const double avgProcessingTime = m_GlobalTotalProcessingTime / m_GlobalTotalCompletedTasks;
  const double avgProcessingWaitingTime = m_GlobalTotalProcessingWaitingTime / m_GlobalTotalCompletedTasks;
  const double avgCommunicationTime = m_GlobalTotalCommunicationTime / m_GlobalTotalCompletedTasks;
  const double avgCommunicationWaitingTime = m_GlobalTotalCommunicationWaitingTime / m_GlobalTotalCompletedTasks;
  const double avgTotalTurnaroundTime = m_GlobalTotalTurnaroundTime / m_GlobalTotalCompletedTasks;
  const double maxComputationalPower = m_GlobalTotalProcessedMFlops / m_GlobalSimulationTime;

  /// The efficiency is calculated as: Rmax / Rpeak
  const double efficiency = maxComputationalPower / m_GlobalTotalComputationalPower;

  ispd_log(LOG_INFO, "");
  ispd_log(LOG_INFO, "Global Simulation Time...........: %lf seconds.", m_GlobalSimulationTime);
  ispd_log(LOG_INFO, "");
  ispd_log(LOG_INFO, "Total Metrics");
  ispd_log(LOG_INFO, " Total Processed MFLOPS..........: %lf MFLOPS.", m_GlobalTotalProcessedMFlops);
  ispd_log(LOG_INFO, " Total Communicated MBits........: %lf MBits.", m_GlobalTotalCommunicatedMBits);
  ispd_log(LOG_INFO, " Total Processing Waiting Time...: %lf seconds.", m_GlobalTotalProcessingWaitingTime);
  ispd_log(LOG_INFO, " Total Communication Waiting Time: %lf seconds.", m_GlobalTotalCommunicationWaitingTime);
  ispd_log(LOG_INFO, " Total Master Services...........: %u services.", m_GlobalTotalMasterServices);
  ispd_log(LOG_INFO, " Total Link Services.............: %u services.", m_GlobalTotalLinkServices);
  ispd_log(LOG_INFO, " Total Machine Services..........: %u services.", m_GlobalTotalMachineServices);
  ispd_log(LOG_INFO, " Total Switch Services...........: %u services.", m_GlobalTotalSwitchServices);
  ispd_log(LOG_INFO, " Total Completed Tasks...........: %u tasks.", m_GlobalTotalCompletedTasks);
  ispd_log(LOG_INFO, "");
  ispd_log(LOG_INFO, "Average Metrics");
  ispd_log(LOG_INFO, " Avg. Processing Time............: %lf seconds.", avgProcessingTime);
  ispd_log(LOG_INFO, " Avg. Processing Waiting Time....: %lf seconds.", avgProcessingWaitingTime);
  ispd_log(LOG_INFO, " Avg. Communication Time.........: %lf seconds.", avgCommunicationTime);
  ispd_log(LOG_INFO, " Avg. Communication Waiting Time.: %lf seconds.", avgCommunicationWaitingTime);
  ispd_log(LOG_INFO, " Avg. Turnaround Time............: %lf seconds.", avgTotalTurnaroundTime);
  ispd_log(LOG_INFO, "");
  ispd_log(LOG_INFO, "System Metrics");
  ispd_log(LOG_INFO, " Peak Computational Power........: %lf MFLOPS.", m_GlobalTotalComputationalPower);
  ispd_log(LOG_INFO, " Max. Computational Power........: %lf MFLOPS.", maxComputationalPower);
  ispd_log(LOG_INFO, " Avg. Turnaround Time............: %lf seconds.", avgTotalTurnaroundTime);
  ispd_log(LOG_INFO, " Efficiency......................: %lf%%.", efficiency * 100.0);
  ispd_log(LOG_INFO, " Total CPU Cores.................: %u cores.", m_GlobalTotalCpuCores);
  ispd_log(LOG_INFO, "");
}

}; // namespace ispd::metrics

namespace ispd::node_metrics {

  ispd::metrics::NodeMetricsCollector *g_NodeMetricsCollector = new ispd::metrics::NodeMetricsCollector();

  template <>
  void notifyMetric(const enum ispd::metrics::NodeMetricsFlag flag, const double value) {
    /// Forward the notification to the node metrics collector.
    g_NodeMetricsCollector->notifyMetric(flag, value);
  }

  template <>
  void notifyMetric(const enum ispd::metrics::NodeMetricsFlag flag, const unsigned value) {
    /// Forward the notification to the node metrics collector.
    g_NodeMetricsCollector->notifyMetric(flag, value);
  }

  void notifyMetric(const enum ispd::metrics::NodeMetricsFlag flag) {
    /// Forward the notification to the node metrics collector.
    g_NodeMetricsCollector->notifyMetric(flag);
  }

  void reportNodeMetrics() {
    /// Forward the report to the node metrics collector.
    g_NodeMetricsCollector->reportNodeMetrics();
  }

}; // namespace ispd::node_metrics

namespace ispd::global_metrics {

  ispd::metrics::GlobalMetricsCollector *g_GlobalMetricsCollector = new ispd::metrics::GlobalMetricsCollector();

  void reportGlobalMetrics() {
    /// Forward the report to the global metrics collector.
    g_GlobalMetricsCollector->reportGlobalMetrics();
  }
  
}; // namespace ispd::global_metrics

