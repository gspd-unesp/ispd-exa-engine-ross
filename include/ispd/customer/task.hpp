#ifndef ISPD_CUSTOMER_TASK_HPP
#define ISPD_CUSTOMER_TASK_HPP

#include <ross.h>
#include <ispd/model/user.hpp>

namespace ispd {
namespace customer {

struct task {
  double proc_size;
  double comm_size;

  tw_lpid origin;
  tw_lpid dest;
  
  double submit_time;
  double end_time;
  
  ispd::model::User::uid_t owner;
};

}; // namespace customer
}; // namespace ispd

#endif // ISPD_CUSTOMER_TASK_HPP
