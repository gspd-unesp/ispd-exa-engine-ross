#ifndef ISPD_SCHEDULER_HPP
#define ISPD_SCHEDULER_HPP

#include <ross.h>
#include <vector>
#include <ispd/message/message.hpp>

/// \namespace ispd::scheduler
///
/// \brief Contains classes related to the scheduling policies for simulation.
namespace ispd::scheduler {

/// \class Scheduler
///
/// \brief Represents an abstract base class for scheduling policy in
///        simulations.
///
/// The Scheduler class provides an interface for scheduling tasks within a
/// simulation. It defines methods for initializing the scheduler and performing
/// forward and reverse scheduling of tasks for simulation entities.
class Scheduler {
public:
  /// \brief Initializes the scheduler.
  ///
  /// This method is responsible for initializing any necessary data structures
  /// or state required by the scheduler before scheduling tasks.
  ///
  virtual void initScheduler(std::vector<tw_lpid> &slaves) = 0;

  /// \brief Performs forward scheduling of tasks.
  ///
  /// This method is used to perform forward scheduling of tasks for simulation
  /// entities. The implementation of this method should schedule tasks for the
  /// specified entities based on the provided parameters.
  ///
  /// \param slaves A vector containing the identifiers of the simulation
  ///               entities to be scheduled.
  /// \param bf A pointer to the bitfield associated with the simulation
  ///           entities.
  /// \param msg A pointer to the message associated with the scheduling
  ///          operation.
  /// \param lp A pointer to the logical process performing the scheduling.
  ///
  /// \return The identifier of the simulation entity that is scheduled to
  ///         execute the task.
  ///
  [[nodiscard]] virtual tw_lpid forwardSchedule(std::vector<tw_lpid> &slaves,
                                                tw_bf *const bf,
                                                ispd_message *const msg,
                                                tw_lp *const lp) = 0;

  /// \brief Performs reverse scheduling of tasks.
  ///
  /// This method is used to perform reverse scheduling of tasks for simulation
  /// entities. The implementation of this method should reverse the scheduling
  /// operation performed during the forward simulation step.
  ///
  /// \param slaves A vector containing the identifiers of the simulation
  ///               entities to be reversed.
  /// \param bf A pointer to the bitfield associated with the simulation
  ///           entities.
  /// \param msg A pointer to the message associated with the scheduling
  ///            operation.
  /// \param lp A pointer to the logical process performing the scheduling.
  ///
  virtual void reverseSchedule(std::vector<tw_lpid> &slaves, tw_bf *const bf,
                               ispd_message *const msg, tw_lp *const lp) = 0;
};

} // namespace ispd::scheduler

#endif // ISPD_SCHEDULER_HPP
