#include <mpi.h>
#include <algorithm>
#include <ispd/log/log.hpp>
#include <ispd/metrics/metrics.hpp>

namespace ispd::metrics {

void NodeMetricsCollector::notifyMetric(const enum NodeMetricsFlag flag, const double value) {
  switch (flag) {
    case NODE_TOTAL_PROCESSED_MFLOPS:
      /// In this case, the total processed MFLOPS in the simulation is
      /// just the addition of the all processed MFLOPS by all processing
      /// services that have been simulated in this node.
      m_NodeTotalProcessedMFlops += value;
      break;
    case NODE_TOTAL_COMMUNICATED_MBITS:
      /// In this case, the total communicated Mbits in the simulation is
      /// just the addition of the all communicated Mbits by all communication
      /// services that have been simulated in this node.
      m_NodeTotalCommunicatedMBits += value; 
      break;
    case NODE_SIMULATION_TIME:
      /// In this case, the value acts as the last activity time of
      /// the service center.
      m_NodeSimulationTime = std::max(m_NodeSimulationTime, value);
      break;
  }
}

void NodeMetricsCollector::reportNodeMetrics() {
  /// Report to the master node the simulation time of this node.
  if (MPI_SUCCESS != MPI_Reduce(&m_NodeSimulationTime, &ispd::global_metrics::g_GlobalMetricsCollector->m_GlobalSimulationTime, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD))
    ispd_error("Global simulation time could not be reduced, exiting...");

  /// Report to the master node the processed MFlops of this node.
  if (MPI_SUCCESS != MPI_Reduce(&m_NodeTotalProcessedMFlops, &ispd::global_metrics::g_GlobalMetricsCollector->m_GlobalTotalProcessedMFlops, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD))
    ispd_error("Global total processed mflops could not be reduced, exiting...");

  /// Report to the master node the processed MFlops of this node.
  if (MPI_SUCCESS != MPI_Reduce(&m_NodeTotalCommunicatedMBits, &ispd::global_metrics::g_GlobalMetricsCollector->m_GlobalTotalCommunicatedMBits, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD))
    ispd_error("Global total communicated mbits could not be reduced, exiting...");

}

void GlobalMetricsCollector::reportGlobalMetrics() {
  std::printf("Global Simulation Time...: %lf.\n", m_GlobalSimulationTime);
  std::printf("Total Processed MFLOPS...: %lf.\n", m_GlobalTotalProcessedMFlops);
  std::printf("Total Communicated MBits.: %lf.\n", m_GlobalTotalCommunicatedMBits);
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

