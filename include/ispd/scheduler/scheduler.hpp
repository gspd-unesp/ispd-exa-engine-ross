#ifndef ISPD_SCHEDULER_HPP
#define ISPD_SCHEDULER_HPP

#include <ross.h>
#include <ispd/message/message.hpp>

namespace ispd {
namespace scheduler {

struct scheduler {

  virtual void init_scheduler() = 0;

  virtual tw_lpid forward_schedule(
      std::vector<tw_lpid> &slaves,
      tw_bf *bf,
      ispd_message *msg,
      tw_lp *lp
  ) = 0;

  virtual void reverse_schedule(
      std::vector<tw_lpid> &slaves,
      tw_bf *bf,
      ispd_message *msg,
      tw_lp *lp
  ) = 0;

};

}; // namespace scheduler
}; // namespace ispd

#endif // ISPD_SCHEDULER_HPP
