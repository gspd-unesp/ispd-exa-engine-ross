#ifndef ISPD_METRICS_HPP
#define ISPD_METRICS_HPP

#include <ross.h>
#include <filesystem>
#include <unordered_map>
#include <ispd/model/user.hpp>
#include <lib/nlohmann/json.hpp>
#include <ispd/services/services.hpp>
#include <ispd/configuration/link.hpp>
#include <ispd/metrics/link_metrics.hpp>
#include <ispd/configuration/machine.hpp>
#include <ispd/metrics/machine_metrics.hpp>

#ifdef DEBUG_ON
  #include <cstdint>
#endif // DEBUG_ON

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

  /// \brief The accumulation of all the GPU cores count of all machines simulated in this node.
  NODE_TOTAL_GPU_CORES,
  
  /// \brief The accumulation of the turnaround time of all completed tasks received by the masters simulated in this node.
  NODE_TOTAL_TURNAROUND_TIME,

  /// \brief The accumulation of the total non idle energy consupmtion of all services simulated in this node.
  NODE_TOTAL_NON_IDLE_ENERGY_CONSUMPTION,

  /// \brief The accumulation of the total power consumption of all services in this node while being idle.
  NODE_TOTAL_POWER_IDLE,

  /// \brief The simulation time in this node.
  NODE_SIMULATION_TIME,

#ifdef DEBUG_ON
  /// \brief The accumulation of the real time taken (ns) to process forwarding an event in a master.
  NODE_MASTER_FORWARD_TIME,

  /// \brief The accumulation of the real time taken (ns) to reverse the process of an event in a master.
  NODE_MASTER_REVERSE_TIME,

  /// \brief The accumulation of the real time taken (ns) to process forwarding an event in a link.
  NODE_LINK_FORWARD_TIME,

  /// \brief The accumulation of the real time taken (ns) to reverse the process of an event in a link.
  NODE_LINK_REVERSE_TIME,

  /// \brief The accumulation of the real time taken (ns) to process forwarding an event in a machine.
  NODE_MACHINE_FORWARD_TIME,
  
  /// \brief The accumulation of the real time taken (ns) to reverse the process of an event in a machine.
  NODE_MACHINE_REVERSE_TIME,

  /// \brief The accumulation of the real time taken (ns) to process forwarding an event in a switch.
  NODE_SWITCH_FORWARD_TIME,
  
  /// \brief The accumulation of the real time taken (ns) to reverse the process of an event in a switch.
  NODE_SWITCH_REVERSE_TIME,
#endif // DEBUG_ON
};

/// \brief Collects and reports various node-level metrics related to simulation components within a node.
///
/// The NodeMetricsCollector class provides a mechanism for collecting and reporting a variety of node-level metrics
/// related to different simulation components within a node. These metrics help track and analyze the performance,
/// behavior, and resource utilization of the simulated system at a granular level.
class NodeMetricsCollector {
private:
  unsigned m_NodeTotalMasterServices;           ///< Total count of master services simulated in this node.
  unsigned m_NodeTotalLinkServices;             ///< Total count of link services simulated in this node.
  unsigned m_NodeTotalMachineServices;          ///< Total count of machine services simulated in this node.
  unsigned m_NodeTotalSwitchServices;           ///< Total count of switch services simulated in this node.
  unsigned m_NodeTotalCompletedTasks;           ///< Total count of completed tasks simulated in this node.

  double m_NodeTotalComputationalPower;         ///< Total computational power simulatted in this node.
  unsigned m_NodeTotalCpuCores;                 ///< Total count of CPU cores simulated in this node.
  unsigned m_NodeTotalGpuCores;                 ///< Total count of GPU cores simulated in this node.

  double m_NodeTotalCommunicatedMBits;          ///< Total communicated MBits simulated in this node.
  double m_NodeTotalProcessedMFlops;            ///< Total processed MFLOPS simulated in this node.
  double m_NodeTotalProcessingTime;             ///< Total processing time simulated in this node.
  double m_NodeTotalProcessingWaitingTime;      ///< Total waiting time for processing simulated in this node.
  double m_NodeTotalCommunicationTime;          ///< Total communication time simulated in this node.
  double m_NodeTotalCommunicationWaitingTime;   ///< Total waiting time for communication simulated in this node.
  double m_NodeTotalTurnaroundTime;             ///< Total turnaround time simulated in this node.
  double m_NodeTotalNonIdleEnergyConsumption;   ///< Total non idle energy consumption (in Joules) in this node.
  double m_NodeTotalPowerIdle;                  ///< Total idle power consumption from the services in this node.
  double m_NodeSimulationTime;                  ///< The highest last activity time of a service center simulated in this node.

#ifdef DEBUG_ON
  std::unordered_map<ispd::services::ServiceType, double> m_NodeTotalForwardTime;
  std::unordered_map<ispd::services::ServiceType, uint64_t> m_NodeTotalForwardEventsCount;
  std::unordered_map<ispd::services::ServiceType, double> m_NodeTotalReverseTime;
  std::unordered_map<ispd::services::ServiceType, uint64_t> m_NodeTotalReverseEventsCount;
#endif // DEBUG_ON

  void reportNodeUserMetrics();
public:
    /// \brief Notify the NodeMetricsCollector about a node-level metric with a flag.
    ///
    /// This method allows notifying the NodeMetricsCollector about a specific node-level metric using the provided flag.
    /// The corresponding metric value will be updated based on the flag provided.
    ///
    /// \param flag The flag indicating the type of metric to be notified.
    void notifyMetric(const NodeMetricsFlag flag);

    /// \brief Notify the NodeMetricsCollector about a node-level metric with a flag and a value.
    ///
    /// This templated method allows notifying the NodeMetricsCollector about a specific node-level metric using the
    /// provided flag and associated value. The corresponding metric value will be updated based on the flag and value
    /// provided.
    ///
    /// \tparam T The type of the metric value.
    /// \param flag The flag indicating the type of metric to be notified.
    /// \param value The value of the metric to be updated.
    template <typename T>
    void notifyMetric(const NodeMetricsFlag flag, const T value);

    /// \brief Report the collected node-level metrics to the master node.
    ///
    /// This method is responsible for reporting the collected node-level metrics to the master node.
    /// It performs communication and aggregation of metrics as required by the simulation setup.
    void reportNodeMetrics();
};

/// \brief Collects and reports global-level metrics aggregated from multiple nodes within the simulation.
///
/// The GlobalMetricsCollector class is responsible for collecting and reporting global-level metrics that are aggregated
/// from multiple nodes within the simulation. These metrics provide a high-level overview of the entire simulated system's
/// performance, resource utilization, and behavior.
class GlobalMetricsCollector {
    friend NodeMetricsCollector;  ///< Allows NodeMetricsCollector to access private members.

private:
    unsigned m_GlobalTotalMasterServices;           ///< Total count of master services across all nodes.
    unsigned m_GlobalTotalLinkServices;             ///< Total count of link services across all nodes.
    unsigned m_GlobalTotalMachineServices;          ///< Total count of machine services across all nodes.
    unsigned m_GlobalTotalSwitchServices;           ///< Total count of switch services across all nodes.
    unsigned m_GlobalTotalCompletedTasks;           ///< Total count of completed tasks across all nodes.

    double m_GlobalTotalComputationalPower;         ///< Total computational power across all nodes.
    unsigned m_GlobalTotalCpuCores;                 ///< Total count of CPU cores across all nodes.
    unsigned m_GlobalTotalGpuCores;                 ///< Total count of GPU cores across all nodes.

    double m_GlobalTotalCommunicatedMBits;          ///< Total communicated MBits across all nodes.
    double m_GlobalTotalProcessedMFlops;            ///< Total processed MFLOPS across all nodes.
    double m_GlobalTotalProcessingTime;             ///< Total processing time across all nodes.
    double m_GlobalTotalProcessingWaitingTime;      ///< Total waiting time for processing across all nodes.
    double m_GlobalTotalCommunicationTime;          ///< Total communication time across all nodes.
    double m_GlobalTotalCommunicationWaitingTime;   ///< Total waiting time for communication across all nodes.
    double m_GlobalTotalTurnaroundTime;             ///< Total turnaround time across all nodes.
    double m_GlobalTotalNonIdleEnergyConsumption;   ///< Total non idle energy consumption across all nodes.
    double m_GlobalTotalPowerIdle;                  ///< Total power idle across all nodes.
    double m_GlobalSimulationTime;                  ///< Total simulation time.

    std::unordered_map<ispd::model::User::uid_t, ispd::metrics::UserMetrics> m_GlobalUserMetrics; ///< Total user metrics.
#ifdef DEBUG_ON
  std::unordered_map<ispd::services::ServiceType, double> m_GlobalTotalForwardTime;
  std::unordered_map<ispd::services::ServiceType, uint64_t> m_GlobalTotalForwardEventsCount;
  std::unordered_map<ispd::services::ServiceType, double> m_GlobalTotalReverseTime;
  std::unordered_map<ispd::services::ServiceType, uint64_t> m_GlobalTotalReverseEventsCount;
#endif // DEBUG_ON
public:
    /// \brief Report the aggregated global-level metrics to an external source.
    ///
    /// This method is responsible for reporting the aggregated global-level metrics and some other calculated
    /// metrics to the standard output.
    void reportGlobalMetrics();

    /// \brief Write aggregated global-level metrics to a file in JSON format.
    ///
    /// This function plays a crucial role in the reporting mechanism of the simulation engine. Its primary
    /// responsibility is to write the aggregated global-level metrics, obtained during the simulation, to a
    /// specified file in the JSON format. The generated report serves as an essential input for the
    /// `ispd-exa-gui` application, enhancing the visualization and analysis of simulation results.
    ///
    /// \param reportFilePath The file path where the aggregated metrics report will be written in JSON format.
    void reportGlobalMetricsToFile(const std::filesystem::path reportFilePath);
};

}; // namespace ispd::metrics

/// \brief Provides functionality to collect, notify, and report node-level metrics within the iSPD simulation framework.
///
/// The ispd::node_metrics namespace encapsulates the functionality related to node-level metrics collection, notification,
/// and reporting within the iSPD simulation framework. It defines functions and variables that allow the simulation components
/// to track and analyze various metrics associated with individual nodes in the simulated system.
namespace ispd::node_metrics {

    /// Pointer to the global instance of the NodeMetricsCollector responsible for tracking node-level metrics.
    extern ispd::metrics::NodeMetricsCollector *g_NodeMetricsCollector;
    
    /// Pointer to the global instance of the JSON object responsible for tracking node-level metrics that will
    /// be written into the node-level metrics report file.
    extern nlohmann::json *g_NodeMetricsReport;

    /// \brief Notify the metrics collector about a node-level metric without a value.
    ///
    /// This function is used to notify the metrics collector about a specific node-level metric that does not require
    /// an associated value. The provided metric flag indicates the type of metric being reported.
    ///
    /// \param flag The flag representing the specific node-level metric to notify.
    void notifyMetric(const ispd::metrics::NodeMetricsFlag flag);

    /// \brief Notify the metrics collector about a node-level metric with a value.
    ///
    /// This function is used to notify the metrics collector about a specific node-level metric along with an associated value.
    /// The provided metric flag indicates the type of metric being reported, and the value parameter holds the corresponding value.
    ///
    /// \tparam T The data type of the metric value.
    /// \param flag The flag representing the specific node-level metric to notify.
    /// \param value The value associated with the reported metric.
    template <typename T>
    void notifyMetric(const ispd::metrics::NodeMetricsFlag flag, const T value);

    void notifyReport(const ispd::metrics::MachineMetrics &metrics, 
                      const ispd::configuration::MachineConfiguration &configuration,
                      const tw_lpid gid);

    void notifyReport(const ispd::metrics::LinkMetrics &metrics,
                      const ispd::configuration::LinkConfiguration &configuration,
                      const tw_lpid gid);

    /// \brief Report the aggregated node-level metrics to an external source.
    ///
    /// This function is responsible for reporting the aggregated node-level metrics to the node master.
    /// It facilitates communication and aggregation of metrics for individual nodes across the simulation setup.
    void reportNodeMetrics();

    /// \brief Write aggregated node-level metrics to a file in JSON format.
    ///
    /// This function plays a crucial role in the reporting mechanism of the simulation engine. Its primary
    /// responsibility is to write the aggregated node-level metrics, obtained during the simulation, to a
    /// specified file in the JSON format.
    void reportNodeMetricsToFile();

} // namespace ispd::node_metrics

/// \brief Provides functionality to collect and report global-level metrics within the iSPD simulation framework.
///
/// The ispd::global_metrics namespace encapsulates the functionality related to global-level metrics collection and reporting
/// within the iSPD simulation framework. It defines functions and variables that allow the simulation components to track and
/// analyze various metrics associated with the entire simulation setup, enabling aggregation and analysis of metrics at the
/// global level.
namespace ispd::global_metrics {

    /// Pointer to the global instance of the GlobalMetricsCollector responsible for tracking global-level metrics.
    extern ispd::metrics::GlobalMetricsCollector *g_GlobalMetricsCollector;

    /// \brief Report the aggregated global-level metrics to an external source.
    ///
    /// This function is responsible for reporting the aggregated global-level metrics to the standard output.
    /// It facilitates the communication and analysis of metrics collected from across the entire simulation setup.
    void reportGlobalMetrics();

    /// \brief Write aggregated global-level metrics to a file in JSON format.
    ///
    /// This function plays a crucial role in the reporting mechanism of the simulation engine. Its primary
    /// responsibility is to write the aggregated global-level metrics, obtained during the simulation, to a
    /// specified file in the JSON format. The generated report serves as an essential input for the
    /// `ispd-exa-gui` application, enhancing the visualization and analysis of simulation results.
    ///
    /// \param reportFilePath The file path where the aggregated metrics report will be written in JSON format.
    void reportGlobalMetricsToFile(const std::filesystem::path reportFilePath);

} // namespace ispd::global_metrics

#endif // ISPD_METRICS_HPP
