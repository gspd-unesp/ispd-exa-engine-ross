#ifndef ISPD_SERVICE_MASTER_HPP
#define ISPD_SERVICE_MASTER_HPP

#include <ross.h>
#include <vector>
#include <memory>
#include <chrono>
#include <ispd/debug/debug.hpp>
#include <ispd/model/builder.hpp>
#include <ispd/routing/routing.hpp>
#include <ispd/metrics/metrics.hpp>
#include <ispd/workload/workload.hpp>
#include <ispd/scheduler/scheduler.hpp>
#include <ispd/scheduler/round_robin.hpp>
#include <ispd/metrics/master_metrics.hpp>



namespace ispd {
namespace services {


struct master_state {
  /// \brief Master's slaves.
  std::vector<tw_lpid> slaves;

  /// \brief Master's scheduler.
  ispd::scheduler::Scheduler *scheduler;

  /// \brief Master's workload generator.
  ispd::workload::Workload *workload;

  /// \brief Master's metrics.
  ispd::metrics::MasterMetrics metrics;
};


}; // namespace services
}; // namespace ispd
#endif // ISPD_SERVICE_MASTER_HPP

