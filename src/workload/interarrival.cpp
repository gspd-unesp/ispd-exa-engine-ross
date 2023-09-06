#include <ispd/log/log.hpp>
#include <ispd/workload/interarrival.hpp>

namespace ispd::workload {

FixedInterarrivalDistribution::FixedInterarrivalDistribution(
    const double interval)
    : InterarrivalDistribution(), m_Interval(interval) {
  /// Checks if the specified interval is not a number. If so, the program is
  /// immediately aborted.
  if (std::isnan(interval))
    ispd_error("Error in FixedInterarrivalDistribution constructor: The "
               "specified interarrival interval is not a valid number (NaN).");

  /// Checks if the specified interval is not finite. If so, the program is
  /// immediately aborted.
  if (!std::isfinite(interval))
    ispd_error("Error in FixedInterarrivalDistribution constructor: The "
               "specified interarrival interval must be finite.");

  /// Checks if the specified interval is non-positive. If so, the program is
  /// immediately aborted.
  if (interval <= 0.0)
    ispd_error("Error in FixedInterarrivalDistribution constructor: The "
               "specified interarrival is non-positive.");
}

void FixedInterarrivalDistribution::generateInterarrival(
    tw_rng_stream *const rng, double &offset) {
  offset = m_Interval;
}

void FixedInterarrivalDistribution::reverseGenerateInterarrival(
    [[maybe_unused]] tw_rng_stream *const rng) {
  /// There is no reverse action to be taken in fixed interarrival distribution.
}

ExponentialInterarrivalDistribution::ExponentialInterarrivalDistribution(
    const double lambda) noexcept
    : InterarrivalDistribution(), m_Lambda(lambda) {
  /// Checks if the specified lambda is not a number. If so, the program is
  /// immediately aborted.
  if (std::isnan(lambda))
    ispd_error("Error in ExponentialInterarrivalDistribution constructor: The "
               "specified interarrival lambda is not a valid number (NaN).");

  /// Checks if the specified lambda is non-positive. If so, the program is
  /// immediately aborted.
  if (lambda <= 0.0)
    ispd_error("Error in ExponentialInterarrivalDistribution constructor: The "
               "specified interarrival is non-positive.");
}

void ExponentialInterarrivalDistribution::generateInterarrival(
    tw_rng_stream *const rng, double &offset) {
  offset = tw_rand_exponential(rng, m_Lambda);
}

void ExponentialInterarrivalDistribution::reverseGenerateInterarrival(
    tw_rng_stream *const rng) {
  tw_rand_reverse_unif(rng);
}

PoissonInterarrivalDistribution::PoissonInterarrivalDistribution(
    const double lambda)
    : InterarrivalDistribution(), m_Lambda(lambda) {
  /// Checks if the specified lambda is not a number. If so, the program is
  /// immediately aborted.
  if (std::isnan(lambda))
    ispd_error("Error in PoissonInterarrivalDistribution constructor: The "
               "specified interarrival lambda is not a valid number (NaN).");

  /// Checks if the specified lambda is non-positive. If so, the program is
  /// immediately aborted.
  if (lambda <= 0.0)
    ispd_error("Error in PoissonInterarrivalDistribution constructor: The "
               "specified interarrival is non-positive.");
}

void PoissonInterarrivalDistribution::generateInterarrival(
    tw_rng_stream *const rng, double &offset) {
  offset = tw_rand_poisson(rng, m_Lambda);
}

void PoissonInterarrivalDistribution::reverseGenerateInterarrival(
    tw_rng_stream *const rng) {
  tw_rand_reverse_unif(rng);
}

WeibullInterarrivalDistribution::WeibullInterarrivalDistribution(
    const double mean, const double shape) noexcept
    : InterarrivalDistribution(), m_Mean(mean), m_Shape(shape) {

  /// Checks if the specified mean is not a number. If so, the program is
  /// immediately aborted.
  if (std::isnan(mean))
    ispd_error("Error in WeibullInterarrivalDistribution constructor: The "
               "specified interarrival mean is not a valid number (NaN).");

  /// Checks if the specified mean is non-positive. If so, the program is
  /// immediately aborted.
  if (mean <= 0.0)
    ispd_error("Error in WeibullInterarrivalDistribution constructor: The "
               "specified interarrival mean is non-positive.");

  /// Checks if the specified shape is not a number. If so, the program is
  /// immediately aborted.
  if (std::isnan(shape))
    ispd_error("Error in WeibullInterarrivalDistribution constructor: The "
               "specified interarrival shape is not a valid number (NaN).");

  /// Checks if the specified shape is non-positive. If so, the program is
  /// immediately aborted.
  if (shape <= 0.0)
    ispd_error("Error in WeibullInterarrivalDistribution constructor: The "
               "specified interarrival is non-positive.");
}

WeibullInterarrivalDistribution::WeibullInterarrivalDistribution(
    const std::pair<double, double> &weibullDistributionParameters) noexcept
    : WeibullInterarrivalDistribution(
          std::get<0>(weibullDistributionParameters),
          std::get<1>(weibullDistributionParameters)) {}

void WeibullInterarrivalDistribution::generateInterarrival(
    tw_rng_stream *const rng, double &offset) {
  offset = tw_rand_weibull(rng, m_Mean, m_Shape);
}

void WeibullInterarrivalDistribution::reverseGenerateInterarrival(
    tw_rng_stream *const rng) {
  tw_rand_reverse_unif(rng);
}

} // namespace ispd::workload
