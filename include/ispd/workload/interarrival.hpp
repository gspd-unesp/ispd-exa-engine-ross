#ifndef ISPD_WORKLOAD_INTERARRIVAL_HPP
#define ISPD_WORKLOAD_INTERARRIVAL_HPP

#include <ross.h>

namespace ispd::workload {

class InterarrivalDistribution {
public:
  virtual void generateInterarrival(tw_rng_stream *const rng, double &offset) = 0;
  virtual void reverseGenerateInterarrival(tw_rng_stream *rng) = 0;
};

class FixedInterarrivalDistribution final : public InterarrivalDistribution {
private:
  double m_Interval;
 
public:
  explicit FixedInterarrivalDistribution(const double interval);

  void generateInterarrival(tw_rng_stream *const rng, double &offset) override;
  void reverseGenerateInterarrival(tw_rng_stream *const rng) override; 
};

class PoissonInterarrivalDistribution final : public InterarrivalDistribution {
private:
  double m_Lambda;

public:
  explicit PoissonInterarrivalDistribution(const double lambda);

  void generateInterarrival(tw_rng_stream *const rng, double &offset) override;
  void reverseGenerateInterarrival(tw_rng_stream *const rng) override;
};

} // namespace ispd::workload

#endif // ISPD_WORKLOAD_INTERARRIVAL_HPP
