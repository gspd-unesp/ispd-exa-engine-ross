#ifndef ISPD_MODEL_USER_HPP
#define ISPD_MODEL_USER_HPP

#include <cstdint>
#include <ispd/metrics/user_metrics.hpp>

namespace ispd::model {

/// \class User
///
/// \brief Represents a user in the system with associated attributes.
class User {
public:
  using uid_t = std::uint32_t; ///< Type alias for user identifier.

  /// \brief Constructor for creating a user with specified attributes.
  ///
  /// Creates a new `User` object with the given user identifier, name, and
  /// optional energy consumption limit.
  ///
  /// \param id The unique identifier of the user.
  /// \param name The name of the user.
  /// \param energyConsumptionLimit The energy consumption limit of the user
  /// (default: 0.0).
  [[nodiscard]] explicit User(
      const uid_t id, const std::string &name,
      const double energyConsumptionLimit = 0.0) noexcept
      : m_Id(id), m_Name(name),
        m_EnergyConsumptionLimit(energyConsumptionLimit) {}

  /// \brief Get the unique identifier of the user.
  ///
  /// \return The unique identifier assigned to the user.
  [[nodiscard]] inline const uid_t getId() const noexcept { return m_Id; }

  /// \brief Get the name of the user.
  ///
  /// \return The name associated with the user.
  [[nodiscard]] inline const std::string &getName() const noexcept {
    return m_Name;
  }

  /// \brief Retrieve the node's view of user metrics associated with this
  /// instance.
  ///
  /// This member function allows access to the `UserMetrics` object associated
  /// with the current instance. Users can use this function to retrieve and
  /// manipulate the metrics related to their activities within the system.
  ///
  /// \return A reference to the `UserMetrics` object storing the user's
  /// metrics.
  ///
  [[nodiscard]] inline ispd::metrics::UserMetrics &getMetrics() noexcept {
    return m_Metrics;
  }

  /// \brief Retrieve the node's view of user metrics associated with this
  /// instance.
  ///
  /// This member function allows access to the `UserMetrics` object associated
  /// with the current instance. Users can use this function to retrieve and
  /// manipulate the metrics related to their activities within the system.
  ///
  /// \return A constant reference to the `UserMetrics` object storing the
  /// user's metrics.
  ///
  [[nodiscard]] inline const ispd::metrics::UserMetrics &
  getMetrics() const noexcept {
    return m_Metrics;
  }

  /// \brief Get the energy consumption limit of the user.
  ///
  /// \return The energy consumption limit set for the user.
  [[nodiscard]] inline double getEnergyConsumptionLimit() const noexcept {
    return m_EnergyConsumptionLimit;
  }

private:
  uid_t m_Id;                            ///< The user's unique identifier.
  std::string m_Name;                    ///< The user's name.
  ispd::metrics::UserMetrics m_Metrics;  ///< The node's view of user's metrics.
  double m_EnergyConsumptionLimit = 0.0; ///< The energy consumption limit.
};

} // namespace ispd::model

#endif // ISPD_MOOEL_USER_HPP
