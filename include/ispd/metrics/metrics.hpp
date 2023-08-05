#ifndef ISPD_METRICS_HPP
#define ISPD_METRICS_HPP

namespace ispd::metrics {

class NodeMetricsCollector {
	double m_NodeSimulationTime;
public:
	void notifyLastActivityTime(const double lastActivityTime);
	void reportNodeMetrics();
};

class GlobalMetricsCollector {
  friend NodeMetricsCollector;

  double m_GlobalSimulationTime;
public:
  void reportGlobalMetrics();
};

}; // namespace ispd::metrics

namespace ispd::node_metrics {
  extern ispd::metrics::NodeMetricsCollector *g_NodeMetricsCollector;

  void notifyLastActivityTime(const double lastActivityTime);
	void reportNodeMetrics();
}; // namespace ispd::node_metrics

namespace ispd::global_metrics {
  extern ispd::metrics::GlobalMetricsCollector *g_GlobalMetricsCollector;

  void reportGlobalMetrics();
}; // namespace ispd::global_metrics


#endif // ISPD_METRICS_HPP
