#ifndef ISPD_METRICS_HPP
#define ISPD_METRICS_HPP

namespace ispd::metrics {

/// \brief Enumeration class representing various node-level metrics.
///
/// This enumeration class defines a set of flags that correspond to different types of node-level metrics.
/// These metrics provide insights into the performance and behavior of simulation components within a node.
enum class NodeMetricsFlag { 
  /// \brief The accumulation of all communicated megabits of all simulated links in this node.
  NODE_TOTAL_COMMUNICATED_MBITS,
  
  /// \brief The accumulation of all processed megaflops of all simulated machines in this node.
  NODE_TOTAL_PROCESSED_MFLOPS,

  /// \brief The accumulation of all processing time of all simulated machines in this node.
  NODE_TOTAL_PROCESSING_TIME,

  /// \brief The accumulation of all processing waiting time of all simulated machines in this node.
  NODE_TOTAL_PROCESSING_WAITING_TIME,

  /// \brief The accumulation of all communication time of all simulated links in this node.
  NODE_TOTAL_COMMUNICATION_TIME,

  /// \brief The accumulation of all communication waiting time of all simulated links in this node.
  NODE_TOTAL_COMMUNICATION_WAITING_TIME,

  /// \brief The count of how many masters have been simulated in this node.
  NODE_TOTAL_MASTER_SERVICES,

  /// \bnrief The count of how many links have been simulated in this node.
  NODE_TOTAL_LINK_SERVICES,

  /// \brief The count of how many machines have been simulated in this node.
  NODE_TOTAL_MACHINE_SERVICES,

  /// \brief The count of how many switches have been simulated in this node.
  NODE_TOTAL_SWITCH_SERVICES,

  /// \brief The count of how many completed tasks have been received by the masters simulated by this node.
  NODE_TOTAL_COMPLETED_TASKS,

  /// \brief The accumulation of the computational power of all machines simulated in this node.
  NODE_TOTAL_COMPUTATIONAL_POWER,
  
  /// \brief The accumulation of all the CPU cores count of all machines simulated in this node.
  NODE_TOTAL_CPU_CORES,
  
  /// \brief The accumulation of the turnaround time of all completed tasks received by the masters simulated in this node.
  NODE_TOTAL_TURNAROUND_TIME,

  /// \brief The simulation time in this node.
  NODE_SIMULATION_TIME
};

class NodeMetricsCollector {
  unsigned m_NodeTotalMasterServices;
  unsigned m_NodeTotalLinkServices;
  unsigned m_NodeTotalMachineServices;
  unsigned m_NodeTotalSwitchServices;
  unsigned m_NodeTotalCompletedTasks;

  double m_NodeTotalComputationalPower;
  unsigned m_NodeTotalCpuCores;

  double m_NodeTotalCommunicatedMBits;
  double m_NodeTotalProcessedMFlops;
  double m_NodeTotalProcessingTime;
  double m_NodeTotalProcessingWaitingTime;
  double m_NodeTotalCommunicationTime;
  double m_NodeTotalCommunicationWaitingTime;
  double m_NodeTotalTurnaroundTime;
	double m_NodeSimulationTime;
public:
  void notifyMetric(const enum NodeMetricsFlag flag);

  template <typename T>
  void notifyMetric(const enum NodeMetricsFlag flag, const T value);
	void reportNodeMetrics();
};

class GlobalMetricsCollector {
  friend NodeMetricsCollector;

  unsigned m_GlobalTotalMasterServices;
  unsigned m_GlobalTotalLinkServices;
  unsigned m_GlobalTotalMachineServices;
  unsigned m_GlobalTotalSwitchServices;
  unsigned m_GlobalTotalCompletedTasks;

  double m_GlobalTotalComputationalPower;
  unsigned m_GlobalTotalCpuCores;

  double m_GlobalTotalCommunicatedMBits;
  double m_GlobalTotalProcessedMFlops;
  double m_GlobalTotalProcessingTime;
  double m_GlobalTotalProcessingWaitingTime;
  double m_GlobalTotalCommunicationTime;
  double m_GlobalTotalCommunicationWaitingTime;
  double m_GlobalTotalTurnaroundTime;
  double m_GlobalSimulationTime;
public:
  void reportGlobalMetrics();
};

}; // namespace ispd::metrics

namespace ispd::node_metrics {
  extern ispd::metrics::NodeMetricsCollector *g_NodeMetricsCollector;

  void notifyMetric(const enum ispd::metrics::NodeMetricsFlag flag);

  template <typename T>
  void notifyMetric(const enum ispd::metrics::NodeMetricsFlag flag, const T value);

	void reportNodeMetrics();
}; // namespace ispd::node_metrics

namespace ispd::global_metrics {
  extern ispd::metrics::GlobalMetricsCollector *g_GlobalMetricsCollector;

  void reportGlobalMetrics();
}; // namespace ispd::global_metrics


#endif // ISPD_METRICS_HPP
