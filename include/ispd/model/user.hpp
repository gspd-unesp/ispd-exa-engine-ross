#ifndef ISPD_MODEL_USER_HPP
#define ISPD_MODEL_USER_HPP

namespace ispd::model {

/// \class User
///
/// \brief Represents a user in the system with associated attributes.
class User {
public:
  using uid_t = uint32_t; ///< Type alias for user identifier.

  /// \brief Constructor for creating a user with specified attributes.
  ///
  /// Creates a new `User` object with the given user identifier, name, and
  /// optional energy consumption limit.
  ///
  /// \param id The unique identifier of the user.
  /// \param name The name of the user.
  /// \param energyConsumptionLimit The energy consumption limit of the user
  /// (default: 0.0).
  explicit User(const uid_t id, const std::string &name,
                const double energyConsumptionLimit = 0.0)
      : m_Id(id), m_Name(name),
        m_EnergyConsumptionLimit(energyConsumptionLimit) {}

  /// \brief Get the unique identifier of the user.
  ///
  /// \return The unique identifier assigned to the user.
  inline const uid_t getId() const { return m_Id; }

  /// \brief Get the name of the user.
  ///
  /// \return The name associated with the user.
  inline const std::string &getName() const { return m_Name; }

  /// \brief Get the energy consumption limit of the user.
  ///
  /// \return The energy consumption limit set for the user.
  inline double getEnergyConsumptionLimit() const { return m_EnergyConsumptionLimit; }

private:
  uid_t m_Id;                            ///< The user's unique identifier.
  std::string m_Name;                    ///< The user's name.
  double m_EnergyConsumptionLimit = 0.0; ///< The energy consumption limit.
};

} // namespace ispd::model

#endif // ISPD_MOOEL_USER_HPP
