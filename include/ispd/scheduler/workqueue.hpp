/// \file workqueue.hppp
///
/// \brief This file defines the Workqueue class, a concrete implementation of
/// the Scheduler interface
///
/// The Workqueue class implements a simple workqueue scheduling algorithm.
/// A yet-to-execute task is send to processors as soon as they became
/// available. After the completion of a task, the processors sends back the
/// results and the scheduler assigns a new task to this processor.
#pragma once
#include <ispd/scheduler/scheduler.hpp>
#include <deque>
#include <stack>

namespace ispd::scheduler {
/// \class Workqueue
///
/// \brief Implements the workqueue scheduling policy.
///
/// The Workqueue class inherit from Scheduler class and implements the
/// the workqueue scheduling policy. The tasks are send to processors as soon
/// as they became available. After the completion of a task, the workqueue
/// assigns a new task to this processor.
class Workqueue final : public Scheduler {
private:
  /// \brief Queue of machines that are available for execution
  ///
  std::deque<tw_lpid> freeMachines;
  /// \brief Stores the last available machines for reverse computation
  std::stack<tw_lpid> lastAvailableMachine;

public:
  void initScheduler(std::vector<tw_lpid> &slaves) override {
    for (auto i : slaves) {
      freeMachines.push_back(i);
    }
  }

  [[nodiscard]] tw_lpid forwardSchedule(std::vector<tw_lpid> &slaves, tw_bf *bf,
                                        ispd_message *msg, tw_lp *lp) override {
    /// the master sends the id of the recently freed machine.
    /// if the serviceid is greater than zero, it means it is an arrival message
    if (msg->service_id) {
      /// adds the new free machine in the queue
      freeMachines.push_back(msg->service_id);

    }
    tw_lpid machine = freeMachines.front();
    lastAvailableMachine.push(machine);
    freeMachines.pop_front();
    return machine;
  }

  void reverseSchedule(std::vector<tw_lpid> &slaves, tw_bf *bf,
                       ispd_message *msg, tw_lp *lp) override {
    std::printf("reverse scheduler\n");
    if (msg->service_id) {
      freeMachines.pop_back();
    }
    freeMachines.push_front(lastAvailableMachine.top());
    lastAvailableMachine.pop();
  }
};
} // namespace ispd::scheduler

