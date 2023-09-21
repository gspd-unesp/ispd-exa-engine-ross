#ifndef ISPD_METRICS_VIRTUAL_MACHINE_HPP
#define ISPD_METRICS_VIRTUAL_MACHINE_HPP
namespace ispd::metrics {

/// \struct VirtualMachineMetrics
///
/// \brief This structure encapsulates various metrics and information related
/// to a virtual machine's performance and behaviour during simulation.
///
/// Collects and organize metrics that enable users to analyze and evaluate
/// the performance of virtual machines within a simulated enviroment.
struct VirtualMachineMetrics final {

  double m_ProcMFlops = 0.0; ///< Total of megaflops processed by this virtual machine

  double m_ProcTime = 0.0; ///< Total time in seconds the virtual machine spent processing.

  double m_ProcWaitingTime = 0.0; ///< Total time in seconds the tasks wait in the queue
                                  ///< before being executed

  unsigned m_ProcTasks = 0.0; ///< Total of tasks successfully executed by this machine.
};

};

#endif
