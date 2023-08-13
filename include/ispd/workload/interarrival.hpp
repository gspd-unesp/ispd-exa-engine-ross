#ifndef ISPD_WORKLOAD_INTERARRIVAL_HPP
#define ISPD_WORKLOAD_INTERARRIVAL_HPP

#include <ross.h>

namespace ispd::workload {

/// \class InterarrivalDistribution
///
/// \brief An abstract base class for representing event interarrival time distributions.
///
/// This class defines the interface for generating event interarrival times in a simulation.
/// Subclasses should implement the generateInterarrival() and reverseGenerateInterarrival()
/// methods to provide specific interarrival time distribution behaviors.
class InterarrivalDistribution {
public:
  /// \brief Generates the time until the next event's arrival using the distribution.
  ///
  /// This pure virtual function should be implemented by derived classes to generate the time
  /// until the next event's arrival based on the specific interarrival time distribution.
  ///
  /// \param rng A pointer to the logical process reversible-pseudorandom number generator.
  /// \param offset A reference to a variable where the generated time offset will be stored.
  virtual void generateInterarrival(tw_rng_stream *const rng, double &offset) = 0;

  /// \brief Reverses the generation of the last interarrival time, necessary due to the Time Warp's rollback mechanism.
  ///
  /// This pure virtual function should be implemented by derived classes to reverse the generation
  /// of the last interarrival time when performing reverse computation during the simulation. It allows
  /// the simulation to properly handle rollback and recovery.
  ///
  /// \param rng A pointer to the logical process reversible-pseudorandom number generator.
  virtual void reverseGenerateInterarrival(tw_rng_stream *const rng) = 0;

  /// \brief Virtual destructor for the InterarrivalDistribution class.
  ///
  /// This virtual destructor ensures proper cleanup when objects of derived classes are destroyed.
  virtual ~InterarrivalDistribution() = default;
};

/// \class FixedInterarrivalDistribution
/// \brief Represents a fixed interarrival time distribution for generating event arrival times.
///
/// This class implements the InterarrivalDistribution interface to provide a fixed interarrival
/// time distribution. The interarrival time between consecutive events is constant and determined
/// by the specified interval value.
class FixedInterarrivalDistribution final : public InterarrivalDistribution {
public:
  /// Constructor for FixedInterarrivalDistribution.
  ///
  /// \param interval The fixed interarrival time interval.
  explicit FixedInterarrivalDistribution(const double interval);

  /// Generates the time until the next event's arrival using the fixed interarrival distribution.
  ///
  /// \param rng A pointer to the logical process reversible-pseudorandom number generator.
  /// \param offset A reference to a variable where the generated time offset will be stored.
  void generateInterarrival(tw_rng_stream *const rng, double &offset) override;

  /// Reverses the generation of the last interarrival time.
  ///
  /// Since there is no state to reverse in the fixed interarrival distribution, this function has an empty implementation.
  ///
  /// \param rng A pointer to the logical process reversible-pseudorandom number generator.
  void reverseGenerateInterarrival([[maybe_unused]] tw_rng_stream *const rng) override;

private:
  double m_Interval; ///< The fixed interarrival interval.
};

/// \class PoissonInterarrivalDistribution
/// \brief Represents a Poisson interarrival time distribution for generating event arrival times.
///
/// This class implements the InterarrivalDistribution interface to provide a Poisson interarrival
/// time distribution. The interarrival times follow a Poisson distribution with the specified
/// lambda parameter.
class PoissonInterarrivalDistribution final : public InterarrivalDistribution {
public:
  /// Constructor for PoissonInterarrivalDistribution.
  ///
  /// \param lambda The Poisson distribution lambda parameter.
  explicit PoissonInterarrivalDistribution(const double lambda);

  /// Generates the time until the next event's arrival using the Poisson interarrival distribution.
  ///
  /// \param rng A pointer to the logical process reversible-pseudorandom number generator.
  /// \param offset A reference to a variable where the generated time offset will be stored.
  void generateInterarrival(tw_rng_stream *const rng, double &offset) override;

  /// Reverses the generation of the last interarrival time.
  ///
  /// This function reverses the previously generated interarrival time by reversing the exponential random
  /// number generator state.
  ///
  /// \param rng A pointer to the logical process reversible-pseudorandom number generator.
  void reverseGenerateInterarrival(tw_rng_stream *const rng) override;

private:
  double m_Lambda; ///< The Poisson distribution lambda parameter.
};

} // namespace ispd::workload

#endif // ISPD_WORKLOAD_INTERARRIVAL_HPP
