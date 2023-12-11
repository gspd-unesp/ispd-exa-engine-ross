
#ifndef ISPD_ALLOCATOR_HPP
#define ISPD_ALLOCATOR_HPP

#include <ross.h>
#include <vector>
#include <ispd/message/message.hpp>

/// \namespace Allocator
///
/// \brief Contains classes related to the allocation policies for simulation
namespace ispd::allocator {

/// \class Allocator
///
/// \brief Represents an abstract class for allocation policy in simulations
/// of cloud environment.
///
/// The allocator class provides an interface for allocating virtual machines
/// within a simulation. It defines methods for initializing the allocator and
/// performing forward and reverse allocation of virtual machines.
class Allocator {
public:
  /// \brief Initializes the allocator.
  ///
  /// This method is responsible for initializing any necessary data
  /// structures or state required by the allocator before allocating tasks.
  virtual void initAllocator() = 0;

  /// \brief Performs forward scheduling of tasks.
  ///
  /// This method is used to perform forward allocation for simulation entities
  /// The implementation of this method should forward virtual machines for the
  /// specified entities based on the provided parameters.
  ///
  /// \param machines
  /// \param bf
  /// \param msg
  /// \param lp
  /// \return The identifier of the simulation entity that is scheduled
  /// to allocate the task
  [[nodiscard]] virtual tw_lpid
  forwardAllocation(std::vector<tw_lpid> &machines, tw_bf *bf,
                    ispd_message *msg, tw_lp *lp) = 0;

  /// \brief Performs reverse allocation of virtual machines.
  ///
  /// This method is used to perform reverse allocation for simulation entities
  /// The implementation of this method should reverse the allocating operation
  /// performed during the forward allocation step.
  virtual void reverseAllocation(std::vector<tw_lpid> &machines, tw_bf *bf,
                                 ispd_message *msg, tw_lp *lp) = 0;
};
}; // namespace ispd::allocator
#endif
