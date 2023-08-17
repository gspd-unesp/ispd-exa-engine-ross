
#ifndef ISPD_ALLOCATOR_FIRST_FIT_HPP
#define ISPD_ALLOCATOR_FIRST_FIT_HPP
#include <ispd/allocator/allocator.hpp>

namespace ispd {
namespace allocator {

struct first_fit : allocator {

  unsigned next_machine_id;

  virtual void init() { next_machine_id = 0; }

  virtual tw_lpid forward_allocation(std::vector<tw_lpid> &machines, tw_bf *bf,
                                     ispd_message *msg, tw_lp *lp) {
    bf->c0 = 0;

    const tw_lpid machine_id = machines[next_machine_id];

    next_machine_id++;

    if (next_machine_id == machines.size()) {
      /// Mark the bitfield that the next slave identifier
      /// has overflown and, therefore, has set back to 0.
      ///
      /// This is necessary for the reverse computation.
      bf->c0 = 1;

      /// Set the next slave identifier back to 0.
      next_machine_id = 0;
    }

    return machine_id;
  }

  virtual void reverse_allocation(std::vector<tw_lpid> &machines, tw_bf *bf,
                                  ispd_message *msg, tw_lp *lp) {

    if (bf->c0) {
      bf->c0 = 0;

      next_machine_id = machines.size() - 1;
    }

    else {
      next_machine_id--;
    }
  }
};

}; // namespace allocator
}; // namespace ispd

#endif // ISPD_EXA_ENGINE_ROSS_FIRST_FIT_HPP
