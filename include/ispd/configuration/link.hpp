#pragma once

namespace ispd::configuration {

/// \struct LinkConfiguration
///
/// \brief Represents the configuration of a communication link in the
/// simulation.
///
/// The `LinkConfiguration` structure encapsulates various parameters related to
/// the configuration of a communication link in the simulation environment. It
/// provides methods to obtain information about the link's bandwidth, latency,
/// and load factor based on the specified parameters.
class LinkConfiguration final {
private:
  double m_Bandwidth; ///< Total link's bandwidth (in megabits per second).
  double m_Load;      ///< Load factor of the link (0.0 to 1.0).
  double m_Latency;   ///< Total link's latency (in seconds).

public:
  /// \brief Constructor for LinkConfiguration.
  ///
  /// Initializes a new instance of the `LinkConfiguration` structure with the
  /// provided parameters.
  ///
  /// \param bandwidth Total bandwidth of the link (in megabits per second).
  /// \param load Load factor of the link (0.0 to 1.0).
  /// \param latency Total latency of the link (in seconds).
  [[nodiscard]] constexpr explicit LinkConfiguration(
      const double bandwidth, const double load, const double latency) noexcept
      : m_Bandwidth(bandwidth), m_Load(load), m_Latency(latency) {}

  /// \brief Calculates the time required for communication over the link.
  ///
  /// This member function calculates and returns the time required for
  /// communication over the link based on the provided communication size. The
  /// calculation takes into account the link's latency, load factor, and
  /// bandwidth.
  ///
  /// \param communicationSize Size of the communication (in megabits).
  /// \return Time required for communication (in seconds).
  [[nodiscard]] inline double
  timeToCommunicate(const double communicationSize) const noexcept {
    return m_Latency + communicationSize / ((1.0 - m_Load) * m_Bandwidth);
  }

  /// \brief Returns the total bandwidth of the link.
  ///
  /// \return Total bandwidth of the link (in megabits per second).
  [[nodiscard]] inline double getBandwidth() const noexcept {
    return m_Bandwidth;
  }

  /// \brief Returns the total latency of the link.
  ///
  /// \return Total latency of the link (in seconds).
  [[nodiscard]] inline double getLatency() const noexcept { return m_Latency; }

  /// \brief Returns the load factor of the link.
  ///
  /// \return Load factor of the link (0.0 to 1.0).
  [[nodiscard]] inline double getLoad() const noexcept { return m_Load; }
};

} // namespace ispd::configuration
