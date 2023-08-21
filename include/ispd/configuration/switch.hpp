#pragma once

namespace ispd::configuration {

/// \struct SwitchConfiguration
///
/// \brief Represents the configuration of a communication switch in the
/// simulation.
///
/// The `SwitchConfiguration` structure encapsulates various parameters related to
/// the configuration of a communication switch in the simulation environment. It
/// provides methods to obtain information about the switch's bandwidth, latency,
/// and load factor based on the specified parameters.
class SwitchConfiguration final {
private:
  double m_Bandwidth; ///< Total switch's bandwidth (in megabits per second).
  double m_Load;      ///< Load factor of the switch (0.0 to 1.0).
  double m_Latency;   ///< Total switch's latency (in seconds).

public:
  /// \brief Constructor for SwitchConfiguration.
  ///
  /// Initializes a new instance of the `SwitchConfiguration` structure with the
  /// provided parameters.
  ///
  /// \param bandwidth Total bandwidth of the switch (in megabits per second).
  /// \param load Load factor of the switch (0.0 to 1.0).
  /// \param latency Total latency of the switch (in seconds).
  [[nodiscard]] constexpr explicit SwitchConfiguration(
      const double bandwidth, const double load, const double latency) noexcept
      : m_Bandwidth(bandwidth), m_Load(load), m_Latency(latency) {}

  /// \brief Calculates the time required for communication over the switch.
  ///
  /// This member function calculates and returns the time required for
  /// communication over the switch based on the provided communication size.
  /// The calculation takes into account the switch's latency, load factor, and
  /// bandwidth.
  ///
  /// \param communicationSize Size of the communication (in megabits).
  /// \return Time required for communication (in seconds).
  [[nodiscard]] inline double
  timeToCommunicate(const double communicationSize) const noexcept {
    return m_Latency + communicationSize / ((1.0 - m_Load) * m_Bandwidth);
  }

  /// \brief Returns the total bandwidth of the switch.
  ///
  /// \return Total bandwidth of the switch (in megabits per second).
  [[nodiscard]] inline double getBandwidth() const noexcept {
    return m_Bandwidth;
  }

  /// \brief Returns the total latency of the switch.
  ///
  /// \return Total latency of the switch (in seconds).
  [[nodiscard]] inline double getLatency() const noexcept { return m_Latency; }

  /// \brief Returns the load factor of the switch.
  ///
  /// \return Load factor of the switch (0.0 to 1.0).
  [[nodiscard]] inline double getLoad() const noexcept { return m_Load; }
};

} // namespace ispd::configuration
