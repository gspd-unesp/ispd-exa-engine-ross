#include <mpi.h>
#include <ross.h>
#include <algorithm>
#include <ispd/log/log.hpp>
#include <ispd/metrics/metrics.hpp>

namespace ispd::metrics {

void NodeMetricsCollector::notifyMetric(const enum NodeMetricsFlag flag, const double value) {
  switch (flag) {
    case NODE_TOTAL_PROCESSED_MFLOPS:
      /// In this case, the total processed MFOPS in this node is just the
      /// addition of all processed MFLOPS by all processing services that
      /// have been simulated in this node.
      m_NodeTotalProcessedMFlops += value;
      break;
    case NODE_TOTAL_COMMUNICATED_MBITS:
      /// In this case, the total communicated MBits in this node is just the
      /// addition of all communicated MBits by all communication services that
      /// have been simulated in this node.
      m_NodeTotalCommunicatedMBits += value; 
      break;
    case NODE_TOTAL_PROCESSING_WAITING_TIME:
      /// In this case, the total waiting time for processing in this node is
      /// just the addition of all waiting time for processing by all processing
      /// services that have been simulated in this node.
      m_NodeTotalProcessingWaitingTime += value;
      break;
    case NODE_TOTAL_COMMUNICATION_WAITING_TIME:
      /// In this case, the total waiting time for communication in this node is
      /// just the addition of all waiting time for communication by all communication
      /// services that have been simulated in this node.
      m_NodeTotalCommunicationWaitingTime += value;
      break;
    case NODE_TOTAL_PROCESSING_SERVICES:
      m_NodeTotalProcessingServices++;
      break;
    case NODE_TOTAL_COMMUNICATION_SERVICES:
      m_NodeTotalCommunicationServices++;
      break;
    case NODE_SIMULATION_TIME:
      /// In this case, the value acts as the last activity time of
      /// the service center.
      m_NodeSimulationTime = std::max(m_NodeSimulationTime, value);
      break;
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

  /// Report to the master node the total processing services count.
  if (MPI_SUCCESS != MPI_Reduce(&m_NodeTotalProcessingServices, &gmc->m_GlobalTotalProcessingServices, 1, MPI_UNSIGNED, MPI_SUM, 0, MPI_COMM_ROSS))
    ispd_error("Global total processing services could not be reduced, exiting...");

  /// Report to the master node the total communication services count.
  if (MPI_SUCCESS != MPI_Reduce(&m_NodeTotalCommunicationServices, &gmc->m_GlobalTotalCommunicationServices, 1, MPI_UNSIGNED, MPI_SUM, 0, MPI_COMM_ROSS))
    ispd_error("Global total communication services could not be reduced, exiting...");
}

void GlobalMetricsCollector::reportGlobalMetrics() {
  /// Check if the current node is not the master one. If so, the global metrics will
  /// not be reported, since only the master node will report the global metrics.
  if (g_tw_mynode)
    return;

  const double avgProcessingWaitingTime = m_GlobalTotalProcessingWaitingTime / m_GlobalTotalProcessingServices;
  const double avgCommunicationWaitingTime = m_GlobalTotalCommunicationWaitingTime / m_GlobalTotalCommunicationServices;

  ispd_log(LOG_INFO, "");
  ispd_log(LOG_INFO, "Global Simulation Time...........: %lf seconds.", m_GlobalSimulationTime);
  ispd_log(LOG_INFO, "");
  ispd_log(LOG_INFO, "Total Metrics");
  ispd_log(LOG_INFO, " Total Processed MFLOPS...........: %lf MFLOPS.", m_GlobalTotalProcessedMFlops);
  ispd_log(LOG_INFO, " Total Communicated MBits.........: %lf MBits.", m_GlobalTotalCommunicatedMBits);
  ispd_log(LOG_INFO, " Total Processing Waiting Time....: %lf seconds.", m_GlobalTotalProcessingWaitingTime);
  ispd_log(LOG_INFO, " Total Communication Waiting Time.: %lf seconds.", m_GlobalTotalCommunicationWaitingTime);
  ispd_log(LOG_INFO, " Total Processing Services........: %u services.", m_GlobalTotalProcessingServices);
  ispd_log(LOG_INFO, " Total Communication Services.....: %u services.", m_GlobalTotalCommunicationServices);
  ispd_log(LOG_INFO, "");
  ispd_log(LOG_INFO, "Average Metrics");
  ispd_log(LOG_INFO, " Avg. Processing Waiting Time.....: %lf seconds.", avgProcessingWaitingTime);
  ispd_log(LOG_INFO, " Avg. Communication Waiting Time..: %lf seconds.", avgCommunicationWaitingTime);
  ispd_log(LOG_INFO, "");
}

}; // namespace ispd::metrics

namespace ispd::node_metrics {

  ispd::metrics::NodeMetricsCollector *g_NodeMetricsCollector = new ispd::metrics::NodeMetricsCollector();

  void notifyMetric(const enum ispd::metrics::NodeMetricsFlag flag, const double value) {
    /// Forward the notification to the node metrics collector.
    g_NodeMetricsCollector->notifyMetric(flag, value);
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

