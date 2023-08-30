//
// Created by willao on 29/08/23.
//

#ifndef ISPD_METRICS_VIRTUAL_MACHINE_HPP
#define ISPD_METRICS_VIRTUAL_MACHINE_HPP

namespace  ispd::metrics{

/// \struct Virtual Machine Metrics
/// \brief Encapsulates various metrics and information related to the
/// virtual machine's performance during the simulation.

struct virtual_machine_metrics final{

  double m_ProcMFlops = 0.0; /// total of megaflops processed by this virtual machine

  double m_ProcTime = 0.0; /// total time in seconds the vm spent processing tasks

  double m_ProcWaitingTime = 0.0; /// total time in seconds the tasks wait in the queue
                                  /// before being executed

  unsigned m_Proc_Tasks = 0.0; /// the total of tasks successfully executed by the machine.


};
}
#endif // ISPD_EXA_ENGINE_ROSS_VIRTUAL_MACHINE_METRICS_HPP
