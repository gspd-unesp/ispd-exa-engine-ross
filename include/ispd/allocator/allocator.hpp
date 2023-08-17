
#ifndef ISPD_ALLOCATOR_HPP
#define ISPD_ALLOCATOR_HPP

#include <ross.h>

#include <ispd/message/message.hpp>
#include <ispd/model/builder.hpp>
namespace ispd {
namespace allocator {

struct allocator {

  virtual void init() = 0;

  virtual tw_lpid forward_allocation(std::vector<tw_lpid> &machines, tw_bf *bf,
                                     ispd_message *msg, tw_lp *lp) = 0;

  virtual void reverse_allocation(std::vector<tw_lpid> &machines, tw_bf *bf,
                                  ispd_message *msg, tw_lp *lp) = 0;
};

}; // namespace allocator
}; // namespace ispd

#endif // ISPD_EXA_ENGINE_ROSS_ALLOCATOR_HPP
