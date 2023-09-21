#ifndef ISPD_METRICS_USER_HPP
#define ISPD_METRICS_USER_HPP

namespace ispd::metrics {

/// \struct UserMetrics
///
/// \brief A structure to hold various metrics related to a user's activities.
///
/// This structure is used to store different metrics associated with a user's
/// activities within a system. It provides a convenient way to track and manage
/// metrics related to processing, communication, energy consumption, and
/// workload for a specific user.
///
struct UserMetrics final {
  /// \name Processing-related metrics
  double m_ProcTime = 0.0;        ///< The total processing time used by the user (in seconds).
  double m_ProcWaitingTime = 0.0; ///< The waiting time for processing tasks (in
                                  ///< seconds).

  /// \name Communication-related metrics
  double m_CommTime = 0.0;        ///< The total communication time used by the user (in
                                  ///< seconds).
  double m_CommWaitingTime = 0.0; ///< The waiting time for communication tasks (in
                                  ///< seconds).

  /// \name Energy-related metrics
  double m_EnergyConsumption = 0.0; ///< The total energy consumption by the user (in
                                    ///< Joules).

  /// \name Workload-related metrics
  unsigned m_IssuedTasks = 0;    ///< The total number of tasks issued by the user.
  unsigned m_CompletedTasks = 0; ///< The total number of tasks completed by the user.
  unsigned m_IssuedAllocations = 0; ///< The total number of allocations issued by the user.
  unsigned m_CompletedAllocations = 0; ///< The total number of virtual machines allocated by the user.
};

} // namespace ispd::metrics

#endif // ISPD_METRICS_USER_HPP
