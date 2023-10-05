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
#include <ispd/cloud_scheduler/cloud_scheduler.hpp>

namespace ispd::cloud_scheduler {

/// \class RoundRobin
///
/// \brief Implements a round-robin scheduling algorithm.
///
/// The RoundRobin class inherits from the Scheduler base class and implements
/// the round-robin scheduling strategy. It cycles through a list of slaves in a
/// circular manner, distributing tasks to each slave in sequence.
///
    class RoundRobinCloud final : public CloudScheduler {
    private:

        std::vector<tw_lpid>::size_type m_NextSlaveIndex;
        int m_ArraySize;

    public:
        void initScheduler(int size) override {
            m_ArraySize = size;
            m_NextSlaveIndex = 0;
        }

        [[nodiscard]] tw_lpid forwardSchedule(tw_lpid slaves[1000], tw_bf *bf,
                                              ispd_message *msg, tw_lp *lp) override {
            bf->c0 = 0;

            /// Select the next slave.
            const tw_lpid slave_id = slaves[m_NextSlaveIndex];

            ispd_info("%d ", m_ArraySize);
            /// Increment to the next slave identifier.
            m_NextSlaveIndex++;

            /// Check if the next slave index to be selected has
            /// overflown the slaves vector. Therefore, the next
            /// slave index is set back to 0.
            if (m_NextSlaveIndex == m_ArraySize) {
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

        void reverseSchedule(tw_lpid slaves[10000], tw_bf *bf,
                             ispd_message *msg, tw_lp *lp) override {
            /// Check if the bitfield if the incoming event when
            /// forward processed HAS overflown the slave count. Therefore,
            /// the next slave index MUST be set to the slave count minus 1.
            if (bf->c0) {
                bf->c0 = 0;

                m_NextSlaveIndex = m_ArraySize - 1;
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
