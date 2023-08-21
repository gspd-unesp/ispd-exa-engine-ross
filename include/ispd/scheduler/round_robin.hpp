#pragma once

#include <ispd/scheduler/scheduler.hpp>

namespace ispd::scheduler {

class RoundRobin final : public Scheduler {
private:
  /// \brief The next slave index that will be selected
  ///        in the circular queue.
  unsigned next_slave_index;

public:
  void initScheduler() override {
    next_slave_index = 0;
  }

  [[nodiscard]] tw_lpid forwardSchedule(
      std::vector<tw_lpid> &slaves,
      tw_bf *bf,
      ispd_message *msg,
      tw_lp *lp
  ) override {
    bf->c0 = 0;

    /// Select the next slave.
    const tw_lpid slave_id = slaves[next_slave_index];

    /// Increment to the next slave identifier.
    next_slave_index++;

    /// Check if the next slave index to be selected has
    /// overflown the slaves vector. Therefore, the next
    /// slave index is set back to 0.
    if (next_slave_index == slaves.size()) {
      /// Mark the bitfield that the next slave identifier
      /// has overflown and, therefore, has set back to 0.
      ///
      /// This is necessary for the reverse computation.
      bf->c0 = 1;

      /// Set the next slave identifier back to 0.
      next_slave_index = 0;
    }

    return slave_id;
  }

  void reverseSchedule(
      std::vector<tw_lpid> &slaves,
      tw_bf *bf,
      ispd_message *msg,
      tw_lp *lp
  ) override {
    /// Check if the bitfield if the incoming event when
    /// forward processed HAS overflown the slave count. Therefore,
    /// the next slave index MUST be set to the slave count minus 1.
    if (bf->c0) {
      bf->c0 = 0;

      next_slave_index = slaves.size() - 1;
    }
    /// Check the bitfield if the incoming event when forward
    /// processed HAS NOT overflown the slave count. Therefore,
    /// the next slave identifier is ONLY decremented.
    else {
      next_slave_index--;
    }
  }

};

} // namespace ispd::scheduler

