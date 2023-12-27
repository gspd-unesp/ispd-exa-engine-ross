#pragma once

namespace ispd::metrics {

struct MasterMetrics final {
  /// \brief Amount of cmpleted tasks scheduled by the master.
  unsigned completed_tasks;
  
  /// \brief Sum of all turnaround times of completed tasks.
  double total_turnaround_time;
};

} // namespace ispd::metrics