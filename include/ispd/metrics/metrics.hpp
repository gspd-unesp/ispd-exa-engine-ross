#ifndef ISPD_METRICS_HPP
#define ISPD_METRICS_HPP

namespace ispd::metrics {

enum NodeMetricsFlag { 
  NODE_TOTAL_COMMUNICATED_MBITS,
  NODE_TOTAL_PROCESSED_MFLOPS,
  NODE_TOTAL_PROCESSING_WAITING_TIME,
  NODE_TOTAL_COMMUNICATION_WAITING_TIME,
  NODE_TOTAL_PROCESSING_SERVICES,
  NODE_TOTAL_COMMUNICATION_SERVICES,
  NODE_SIMULATION_TIME
};

class NodeMetricsCollector {
  unsigned m_NodeTotalProcessingServices;
  unsigned m_NodeTotalCommunicationServices;

  double m_NodeTotalCommunicatedMBits;
  double m_NodeTotalProcessedMFlops;
  double m_NodeTotalProcessingWaitingTime;
  double m_NodeTotalCommunicationWaitingTime;
	double m_NodeSimulationTime;
public:
  void notifyMetric(const enum NodeMetricsFlag flag, const double value);
	void reportNodeMetrics();
};

class GlobalMetricsCollector {
  friend NodeMetricsCollector;

  unsigned m_GlobalTotalProcessingServices;
  unsigned m_GlobalTotalCommunicationServices;

  double m_GlobalTotalCommunicatedMBits;
  double m_GlobalTotalProcessedMFlops;
  double m_GlobalTotalProcessingWaitingTime;
  double m_GlobalTotalCommunicationWaitingTime;
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
