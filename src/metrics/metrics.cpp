#include <mpi.h>
#include <algorithm>
#include <ispd/log/log.hpp>
#include <ispd/metrics/metrics.hpp>

namespace ispd::metrics {

void NodeMetricsCollector::notifyLastActivityTime(const double lastActivityTime) {
	m_NodeSimulationTime = std::max(m_NodeSimulationTime, lastActivityTime);
}

void NodeMetricsCollector::reportNodeMetrics() {
  /// Report to the master node the simulation time of this node.
  if (MPI_SUCCESS != MPI_Reduce(&m_NodeSimulationTime, &ispd::global_metrics::g_GlobalMetricsCollector->m_GlobalSimulationTime, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD))
    ispd_error("Global simulation time could not be reduced, exiting...");
}

void GlobalMetricsCollector::reportGlobalMetrics() {
  std::printf("Global Simulation Time: %lf.\n", m_GlobalSimulationTime);
}

}; // namespace ispd::metrics

namespace ispd::node_metrics {

  ispd::metrics::NodeMetricsCollector *g_NodeMetricsCollector = new ispd::metrics::NodeMetricsCollector();

  void notifyLastActivityTime(const double lastActivityTime) {
    /// Forward the notification to the node metrics collector.
    g_NodeMetricsCollector->notifyLastActivityTime(lastActivityTime);
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

