#ifndef ISPD_SCHEDULER_HPP
#define ISPD_SCHEDULER_HPP

#include <ross.h>
#include <vector>
#include <ispd/message/message.hpp>

namespace ispd::scheduler {

class Scheduler {
public:
    virtual void initScheduler() = 0;

    [[nodiscard]] virtual tw_lpid forwardSchedule(
        std::vector<tw_lpid> &slaves,
        tw_bf *const bf,
        ispd_message *const msg,
        tw_lp *const lp
    ) = 0;

    virtual void reverseSchedule(
        std::vector<tw_lpid> &slaves,
        tw_bf *const bf,
        ispd_message *const msg,
        tw_lp *const lp
    ) = 0;
};

} // namespace ispd::scheduler

#endif // ISPD_SCHEDULER_HPP
