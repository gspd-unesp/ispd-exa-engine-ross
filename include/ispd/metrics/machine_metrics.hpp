#ifndef ISPD_METRICS_MACHINE_HPP
#define ISPD_METRICS_MACHINE_HPP

namespace ispd::metrics {

/// \struct MachineMetrics
///
/// \brief This structure encapsulates various metrics and information related
///        to a machine's performance and behavior during simulation.
///
/// The `MachineMetrics` structure collects and organizes metrics that enable
/// users to analyze and evaluate the performance of machines within a
/// simulated environment.
struct MachineMetrics final {
  /// \name Processing-related Metrics
  double m_ProcMflops = 0.0; ///< The total amount of megaflops processed by
                             ///< this machine.

  /// \brief The total time in seconds that the machine spent processing tasks.
  double m_ProcTime = 0.0; ///< The total time in seconds that the machine spent
                           ///< processing tasks.

  double m_ProcWaitingTime = 0.0; ///< The waiting time in seconds that tasks
                                  ///< spent in the processing queue before
                                  ///< being executed by the machine.

  /// \name Tasks-related Metrics
  unsigned m_ProcTasks = 0; ///< The total nubmer of tasks successfully
                            ///< processed by the machine.

  unsigned m_ForwardedTasks =
      0; ///< The total number of tasks forwarded by the machine
         ///< to other components within the simulation.

  unsigned m_allocated_vms = 0;

  double m_Total_cpu_cost = 0;
  double m_Total_memory_cost = 0;
  double m_Total_storage_cost = 0;
  /// \name Energy-related Metrics
  double m_EnergyConsumption; ///< Total energy consumpttion (in Joules).
};

} // namespace ispd::metrics

#endif // ISPD_METRICS_MACHINE_HPP
