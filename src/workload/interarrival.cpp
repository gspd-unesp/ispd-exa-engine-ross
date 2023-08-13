#include <ispd/log/log.hpp>
#include <ispd/workload/interarrival.hpp>

namespace ispd::workload {

FixedInterarrivalDistribution::FixedInterarrivalDistribution(const double interval) :
    InterarrivalDistribution(), m_Interval(interval) {
  /// Checks if the specified interval is not a number. If so, the program is immediately aborted.
  if (std::isnan(interval))
    ispd_error("Error in FixedInterarrivalDistribution constructor: The specified interarrival interval is not a valid number (NaN).");
  
  /// Checks if the specified interval is not finite. If so, the program is immediately aborted.
  if (!std::isfinite(interval))
    ispd_error("Error in FixedInterarrivalDistribution constructor: The specified interarrival interval must be finite.");

  /// Checks if the specified interval is non-positive. If so, the program is immediately aborted.
  if (interval <= 0.0)
    ispd_error("Error in FixedInterarrivalDistribution constructor: The specified interarrival is non-positive.");
}

void FixedInterarrivalDistribution::generateInterarrival(tw_rng_stream *const rng, double &offset) {
  offset = m_Interval;
}

void FixedInterarrivalDistribution::reverseGenerateInterarrival([[maybe_unused]] tw_rng_stream *const rng) {
  /// There is no reverse action to be taken in fixed interarrival distribution.
}

PoissonInterarrivalDistribution::PoissonInterarrivalDistribution(const double lambda) :
  InterarrivalDistribution(), m_Lambda(lambda) {
  /// Checks if the specified lambda is not a number. If so, the program is immediately aborted.
  if (std::isnan(lambda))
    ispd_error("Error in PoissonInterarrivalDistribution constructor: The specified interarrival lambda is not a valid number (NaN).");

  /// Checks if the specified lambda is non-positive. If so, the program is immediately aborted.
  if (lambda <= 0.0)
    ispd_error("Error in PoissonInterarrivalDistribution constructor: The specified interarrival is non-positive.");
}

void PoissonInterarrivalDistribution::generateInterarrival(tw_rng_stream *const rng, double &offset) {
  offset = tw_rand_exponential(rng, m_Lambda);
}

void PoissonInterarrivalDistribution::reverseGenerateInterarrival(tw_rng_stream *const rng) {
  tw_rand_reverse_unif(rng);
}

} // namespace ispd::workload
