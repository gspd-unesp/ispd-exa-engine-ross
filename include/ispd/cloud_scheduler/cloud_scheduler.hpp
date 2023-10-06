#ifndef ISPD_CLOUD_SCHEDULER_HPP
#define ISPD_CLOUD_SCHEDULER_HPP

#include <ross.h>
#include <vector>
#include <ispd/message/message.hpp>


namespace ispd::cloud_scheduler {


    class CloudScheduler {
    public:


        virtual void initScheduler(int size) = 0;



        [[nodiscard]] virtual tw_lpid forwardSchedule(tw_lpid slaves[10000],
                                                      tw_bf *const bf,
                                                      ispd_message *const msg,
                                                      tw_lp *const lp) = 0;

       
        virtual void reverseSchedule(tw_lpid slaves[10000], tw_bf *const bf,
                                     ispd_message *const msg, tw_lp *const lp) = 0;
    };

} // namespace ispd::scheduler

#endif // ISPD_SCHEDULER_HPP
