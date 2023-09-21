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


  /// \name Virtual Machine related metrics.
  unsigned m_AllocatedVms = 0; ///< The total number of virtual machines allocated
///< in this machine.

  unsigned m_RejectedVms = 0; ///< The total number of virtual machines rejected
                              ///< in this machine

  double m_TotalCpuCost = 0; ///< The total cost charged for using the cores of
                            ///< this machine (individual price multiplied by the
                            ///< number of cores used).


  double m_TotalMemoryCost = 0; ///< The total cost charged for using the memory
                                ///< of this machine (individual price multiplied
                                ///< by the amount of memory used).


  double m_TotalDiskSpaceCost = 0; ///< The total cost charged for using the disk space
                                ///< of this machine (individual price multiplied
                                ///< by the amount of disk space used).

  /// \name Energy-related Metrics
  double m_EnergyConsumption; ///< Total energy consumpttion (in Joules).
};

} // namespace ispd::metrics

#endif // ISPD_METRICS_MACHINE_HPP
