#ifndef ISPD_CUSTOMER_TASK_HPP
#define ISPD_CUSTOMER_TASK_HPP

#include <ross.h>
#include <ispd/model/user.hpp>

namespace ispd::customer {

/// \struct Task
///
/// \brief Represents a task within the simulation framework.
///
/// The Task structure encapsulates various attributes and information related
/// to a task that is processed within the simulation environment. It provides a
/// comprehensive set of fields that describe the task's properties, execution
/// details, and ownership.
struct Task final {
  double m_ProcSize; ///< The processing size of the task (in megaflops).
  double m_CommSize; ///< The communication size of the task (in megabits).
  double m_Offload;  ///< The computational offloading factor (0.0 to 1.0).

  tw_lpid m_Origin; ///< The origin node of the task.
  tw_lpid m_Dest;   ///< The destination node of the task.

  double
      m_SubmitTime; ///< The time at which the task was submitted (in seconds).
  double m_EndTime; ///< The time at which the task completed execution (in
                    ///< seconds).

  ispd::model::User::uid_t
      m_Owner; ///< The unique identifier of the task owner.
};

} // namespace ispd::customer

#endif // ISPD_CUSTOMER_TASK_HPP
