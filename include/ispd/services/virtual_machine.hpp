
#ifndef ISPD_VIRTUAL_MACHINE_HPP
#define ISPD_VIRTUAL_MACHINE_HPP
#include <ross.h>
#include <vector>
#include <ispd/message/message.hpp>
#include <ispd/routing/routing.hpp>
#include <ispd/model/builder.hpp>
#include <ispd/metrics/metrics.hpp>
#include <ispd/metrics/user_metrics.hpp>
#include <ispd/services/VMM.hpp>
#include <ispd/configuration/virtual_machine.hpp>
#include <ispd/metrics/virtual_machine_metrics.hpp>

namespace ispd {

namespace services{
struct VM_state {
  ispd::metrics::virtual_machine_metrics metrics;
  ispd::configuration::VmConfiguration conf;
  std::vector<double> cores_free_time;
};

struct virtual_machine {

  static double least_core_time(const std::vector<double> &cores_free_time,
                                unsigned &core_index) {
    double candidate = std::numeric_limits<double>::max();
    unsigned candidate_index;

    for (unsigned i = 0; i < cores_free_time.size(); i++) {
      if (candidate > cores_free_time[i]) {
        candidate = cores_free_time[i];
        candidate_index = i;
      }
    }

    /// Update the core index with the least free time.
    core_index = candidate_index;

    /// Return the least core's free time.
    return candidate;
  }

  static void init(VM_state *s, tw_lp *lp) {}

  static void forward(VM_state *s, tw_bf *bf, ispd_message *msg, tw_lp *lp) {}

  static void reverse(VM_state *s, tw_bf *bf, ispd_message *msg, tw_lp *lp) {}

  static void finish(VM_state *s, ispd_message *msg, tw_lp *lp) {}
};
};
};



#endif
