#ifndef ISPD_WORKLOAD_HPP
#define ISPD_WORKLOAD_HPP

#include <ross.h>

namespace ispd {
namespace workload {

struct workload {
  /// \brief The amount of remanining tasks to be generated.
  unsigned count;

  virtual void workload_generate(tw_rng_stream *rng, double &proc_size,
                                 double &comm_size) = 0;
  virtual void workload_generate_rc(tw_rng_stream *rng) = 0;
};

struct workload_constant : public workload {
  double constant_proc_size;
  double constant_comm_size;

  explicit workload_constant(const unsigned task_count, const double proc_size,
                             const double comm_size)
      : constant_proc_size(proc_size), constant_comm_size(comm_size) {
    count = task_count;
  }

  inline void workload_generate(tw_rng_stream *rng, double &proc_size,
                                double &comm_size) {
    /// Set the processing and communication sizes.
    proc_size = constant_proc_size;
    comm_size = constant_comm_size;

    /// Update the workload information.
    count--;
  }

  inline void workload_generate_rc(tw_rng_stream *rng) {
    /// Reverse the workload information update.
    count++;
  }
};

struct workload_uniform : public workload {
  double min_proc_size;
  double max_proc_size;

  double min_comm_size;
  double max_comm_size;

  explicit workload_uniform(const unsigned task_amount,
                            const double min_proc_size,
                            const double max_proc_size,
                            const double min_comm_size,
                            const double max_comm_size)
      : min_proc_size(min_proc_size), max_proc_size(max_proc_size),
        min_comm_size(min_comm_size), max_comm_size(max_comm_size) {
    count = task_amount;
  }

  inline void workload_generate(tw_rng_stream *rng, double &proc_size,
                                double &comm_size) {
    /// Set the processing and communication sizes.
    proc_size =
        min_proc_size + tw_rand_unif(rng) * (max_proc_size - min_proc_size);
    comm_size =
        min_comm_size + tw_rand_unif(rng) * (max_comm_size - min_comm_size);

    /// Update the workload information.
    count--;

    ispd_debug("[Uniform Workload] Workload (%lf, %lf) generated. Remaining Tasks: %u.", proc_size, comm_size, count);
  }

  inline void workload_generate_rc(tw_rng_stream *rng) {
    tw_rand_reverse_unif(
        rng); /// Reverse the PRNG for the processing size generation.
    tw_rand_reverse_unif(
        rng); /// Reverse the PRNG for the communication size generation.

    /// Reverse the workload information update.
    count++;

    ispd_debug("[Uniform Workload] Workload reversed. Remaining Tasks: %u", count);
  }
};

}; // namespace workload
}; // namespace ispd

#endif // ISPD_WORKLOAD_HPP
