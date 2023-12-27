#pragma once

namespace ispd::metrics {

/// \struct SwitchMetrics
///
/// \brief This structure encapsulates various metrics and information related
///        to a switch's communication and behavior during simulation.
///
/// The `SwitchMetrics` structure collects and organizes metrics that enable
/// users to analyze and evaluate the communication of switches within a
/// simulated environment.
struct SwitchMetrics final {
  double m_UpwardCommMbits;
  double m_DownwardCommMbits;

  unsigned m_UpwardCommPackets;
  unsigned m_DownwardCommPackets;
};

} // namespace ispd::metrics