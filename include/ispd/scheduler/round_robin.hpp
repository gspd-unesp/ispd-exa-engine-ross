/// \file round_robin.hpp
///
/// \brief This file defines the RoundRobin class, a concrete implementation of
/// the Scheduler interface.
///
/// The RoundRobin class implements a simple round-robin scheduling algorithm.
/// It cycles through a list of slaves in a circular manner, distributing tasks
/// to each slave in sequence. The next slave to be selected is tracked using
/// the `m_NextSlaveIndex` member variable.
///
#pragma once

#include <cstdint>
#include <ispd/scheduler/scheduler.hpp>

namespace ispd::scheduler {

/// \class RoundRobin
///
/// \brief Implements a round-robin scheduling algorithm.
///
/// The RoundRobin class inherits from the Scheduler base class and implements
/// the round-robin scheduling strategy. It cycles through a list of slaves in a
/// circular manner, distributing tasks to each slave in sequence.
///
class RoundRobin final : public Scheduler {
private:
  /// \brief The next slave index that will be selected
  ///        in the circular queue.
  std::vector<tw_lpid>::size_type m_NextSlaveIndex;

public:
  void initScheduler(std::vector<tw_lpid> &slaves) override {
    m_NextSlaveIndex = std::vector<tw_lpid>::size_type{0};
  }

  [[nodiscard]] tw_lpid forwardSchedule(std::vector<tw_lpid> &slaves, tw_bf *bf,
                                        ispd_message *msg, tw_lp *lp) override {
    bf->c0 = 0;

    /// Select the next slave.
    const tw_lpid slave_id = slaves[m_NextSlaveIndex];

    /// Increment to the next slave identifier.
    m_NextSlaveIndex++;

    /// Check if the next slave index to be selected has
    /// overflown the slaves vector. Therefore, the next
    /// slave index is set back to 0.
    if (m_NextSlaveIndex == slaves.size()) {
      /// Mark the bitfield that the next slave identifier
      /// has overflown and, therefore, has set back to 0.
      ///
      /// This is necessary for the reverse computation.
      bf->c0 = 1;

      /// Set the next slave identifier back to 0.
      m_NextSlaveIndex = 0;
    }

    return slave_id;
  }

  void reverseSchedule(std::vector<tw_lpid> &slaves, tw_bf *bf,
                       ispd_message *msg, tw_lp *lp) override {
    /// Check if the bitfield if the incoming event when
    /// forward processed HAS overflown the slave count. Therefore,
    /// the next slave index MUST be set to the slave count minus 1.
    if (bf->c0) {
      bf->c0 = 0;

      m_NextSlaveIndex = slaves.size() - 1;
    }
    /// Check the bitfield if the incoming event when forward
    /// processed HAS NOT overflown the slave count. Therefore,
    /// the next slave identifier is ONLY decremented.
    else {
      m_NextSlaveIndex--;
    }
  }
};

} // namespace ispd::scheduler

