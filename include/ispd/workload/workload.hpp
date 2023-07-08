#ifndef ISPD_WORKLOAD_HPP
#define ISPD_WORKLOAD_HPP

#include <ross.h>

namespace ispd {
namespace workload {

struct workload {
  /// \brief The amount of remanining tasks to be generated.
  unsigned count;

  virtual void workload_generate(double &proc_size, double &comm_size) = 0;
  virtual void workload_generate_rc() = 0;

};

struct workload_constant : public workload {
  double constant_proc_size;
  double constant_comm_size;

  explicit workload_constant(const unsigned task_count, const double proc_size, const double comm_size)
    : constant_proc_size(proc_size), constant_comm_size(comm_size) {
      count = task_count;
    }

  inline void workload_generate(double &proc_size, double &comm_size) {
    /// Set the processing and communication sizes.
    proc_size = constant_proc_size;
    comm_size = constant_comm_size;

    /// Update the workload information.
    count--;
  }

  inline void workload_generate_rc() {
    /// Reverse the workload information update.
    count++;
  }

};

}; // namespace workload
}; // namespace ispd

#endif // ISPD_WORKLOAD_HPP
