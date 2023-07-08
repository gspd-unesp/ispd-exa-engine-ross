#ifndef ISPD_SCHEDULER_ROUND_ROBIN_HPP
#define ISPD_SCHEDULER_ROUND_ROBIN_HPP

#include <ispd/scheduler/scheduler.hpp>

namespace ispd {
namespace scheduler {

struct round_robin : scheduler {

  /// \brief The next slave index that will be selected
  ///        in the circular queue.
  unsigned next_slave_index;

  void init_scheduler() {
    next_slave_index = 0;
  }

  tw_lpid forward_schedule(
      std::vector<tw_lpid> &slaves,
      tw_bf *bf,
      ispd_message *msg,
      tw_lp *lp
  ) {
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

    printf("here 3!!!!\n");
      /// Set the next slave identifier back to 0.
      next_slave_index = 0;
    }

    printf("here 4!!!!\n");
    return slave_id;
  }

  void reverse_schedule(
      std::vector<tw_lpid> &slaves,
      tw_bf *bf,
      ispd_message *msg,
      tw_lp *lp
  ) {
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

}; // namespace scheduler
}; // namespace ispd

#endif // ISPD_SCHEDULER_ROUND_ROBIN_HPP
