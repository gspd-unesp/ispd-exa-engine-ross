#pragma once

namespace ispd::metrics {

/// \struct LinkMetrics
///
/// \brief This structure encapsulates various metrics and information related
///        to a link's communication and behavior during simulation.
///
/// The `LinkMetrics` structure collects and organizes metrics that enable
/// users to analyze and evaluate the communication of links within a
/// simulated environment.
struct LinkMetrics final {
  /// \brief The amount of communication time performed by the upward link.
  double upward_comm_time;

  /// \brief The amount of communication time performed by the downward link.
  double downward_comm_time;

  /// \brief The amount of communicated Mbits by the upward link.
  double upward_comm_mbits;

  /// \brief The amount of communicated Mbits by the downward link.
  double downward_comm_mbits;

  /// \brief The amount of communicated packets by the upward link.
  unsigned upward_comm_packets;

  /// \brief The amount of communicated packets by the downward link.
  unsigned downward_comm_packets;

  /// \brief The amount of upward waiting time occurred in this link.
  double upward_waiting_time;

  /// \brief The amount of downward waiting time occurred in this link.
  double downward_waiting_time;
};

}