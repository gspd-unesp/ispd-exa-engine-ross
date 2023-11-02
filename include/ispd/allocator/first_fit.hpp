/// \file first_fit.hpp
///
/// \brief This file defines the FirstFit class, a concrete implementation of
/// the Allocator class.
///
/// The FirstFit class implements a simple first fit allocator algorithm.
/// It gets the first available machine and return its id. The check if the
/// vm fits or not in that machine is done after the event is send.

#pragma once
#include <cstdint>
#include <ispd/allocator/allocator.hpp>

namespace ispd::allocator {

/// \class FirstFit
///
/// \brief Implements a first-fit allocation algorithm.
///
/// The FirstFit  inherits from the Allocator base class and implements
/// the first-fit allocation strategy.
///
class FirstFit final : public Allocator {
private:
  /// \brief The next machine index that will be selected in the circular queue.
  std::vector<tw_lpid>::size_type m_NextMachineIndex;

public:
  void initAllocator() override {
    m_NextMachineIndex = std::vector<tw_lpid>::size_type{0};
  }

  [[nodiscard]] tw_lpid forwardAllocation(std::vector<tw_lpid> &machines,
                                          tw_bf *bf, ispd_message *msg,
                                          tw_lp *lp) override {
    bf->c0 = 0;

    const tw_lpid machine_id = machines[m_NextMachineIndex];

    m_NextMachineIndex++;

    /// Check if the next machine index to be selected has overflown the
    ///  machines vector. Therefore, the next machine index is set to zero

    if (m_NextMachineIndex == machines.size()) {
      /// Mark the bitfield that the next machine identifier
      /// has overflown and, therefore, has set back to 0.
      ///
      /// This is necessary for the reverse computation.
      bf->c0 = 1;

      m_NextMachineIndex = 0;
    }

    return machine_id;
  }

  void reverseAllocation(std::vector<tw_lpid> &machines, tw_bf *bf,
                         ispd_message *msg, tw_lp *lp) override {
    /// Check if the bitfield if the incoming event when
    /// forward processed HAS overflown the machine count. Therefore,
    /// the next machine index MUST be set to the slave count minus 1.
    if (bf->c0) {
      bf->c0 = 0;

      m_NextMachineIndex = machines.size() - 1;
    }
    /// Check the bitfield if the incoming event when forward
    /// processed HAS NOT overflown the machine count. Therefore,
    /// the next machine identifier is ONLY decremented.
    else {
      m_NextMachineIndex--;
    }
  }
};
} // namespace ispd::allocator