#ifndef ISPD_METRICS_HPP
#define ISPD_METRICS_HPP

namespace ispd::metrics {

enum NodeMetricsFlag { 
  NODE_TOTAL_COMMUNICATED_MBITS,
  NODE_TOTAL_PROCESSED_MFLOPS,
  NODE_SIMULATION_TIME
};

class NodeMetricsCollector {
  double m_NodeTotalCommunicatedMBits;
  double m_NodeTotalProcessedMFlops;
	double m_NodeSimulationTime;
public:
  void notifyMetric(const enum NodeMetricsFlag flag, const double value);
	void reportNodeMetrics();
};

class GlobalMetricsCollector {
  friend NodeMetricsCollector;

  double m_GlobalTotalCommunicatedMBits;
  double m_GlobalTotalProcessedMFlops;
  double m_GlobalSimulationTime;
public:
  void reportGlobalMetrics();
};

}; // namespace ispd::metrics

namespace ispd::node_metrics {
  extern ispd::metrics::NodeMetricsCollector *g_NodeMetricsCollector;

  void notifyMetric(const enum ispd::metrics::NodeMetricsFlag flag, const double value);
	void reportNodeMetrics();
}; // namespace ispd::node_metrics

namespace ispd::global_metrics {
  extern ispd::metrics::GlobalMetricsCollector *g_GlobalMetricsCollector;

  void reportGlobalMetrics();
}; // namespace ispd::global_metrics


#endif // ISPD_METRICS_HPP
