#ifndef ISPD_MODEL_BUILDER_HPP
#define ISPD_MODEL_BUILDER_HPP

#include <ross.h>
#include <vector>
#include <functional>
#include <unordered_map>
#include <ispd/log/log.hpp>
#include <ispd/workload/workload.hpp>
#include <ispd/scheduler/scheduler.hpp>

namespace ispd::model {

class User {
public:
  using uid_t = uint32_t;

  explicit User(const uid_t id, const std::string& name, const double energyConsumptionLimit = 0.0) :
    m_Id(id), m_Name(name), m_EnergyConsumptionLimit(energyConsumptionLimit) {}

  const uid_t getId() const { return m_Id; }
  const std::string& getName() const { return m_Name; }
  double getEnergyConsumptionLimit() const { return m_EnergyConsumptionLimit; }

private:
  /// \brief The user unique identifier.
  uid_t m_Id;

  /// \brief The user name.
  std::string m_Name;

  /// \brief The user's energy consumption limit.
  double m_EnergyConsumptionLimit = 0.0;
};

class SimulationModel {
  std::unordered_map<tw_lpid, std::function<void(void *)>> service_initializers;
  std::unordered_map<std::string, User> m_Users;

  inline void
  registerServiceInitializer(const tw_lpid gid,
                             std::function<void(void *)> initializer) {
    /// Checks if a service with the specified global identifier has already
    /// been registered. If so, the program is immediately aborted.
    if (service_initializers.find(gid) != service_initializers.end())
      ispd_error("A service with GID %lu has already been registered.", gid);

    /// Emplace the pair (gid, initializer).
    service_initializers.emplace(gid, initializer);
  }

public:
  void registerMachine(const tw_lpid gid, const double power, const double load,
                       const unsigned coreCount);

  void registerLink(const tw_lpid gid, const tw_lpid from, const tw_lpid to,
                    const double bandwidth, const double load,
                    const double latency);

  void registerSwitch(const tw_lpid gid, const double bandwidth,
                      const double load, const double latency);

  void registerMaster(const tw_lpid gid, std::vector<tw_lpid> &&slaves,
                      ispd::scheduler::scheduler *const scheduler,
                      ispd::workload::Workload *const workload);

  void registerUser(const std::string& name, const double energyConsumptionLimit);

  const std::function<void(void *)> &getServiceInitializer(const tw_lpid gid);

  inline const std::unordered_map<std::string, User>& getUsers() const {
    return m_Users; 
  } 
};

}; // namespace ispd::model

namespace ispd::this_model {
void registerMachine(const tw_lpid gid, const double power, const double load,
                     const unsigned coreCount);

void registerLink(const tw_lpid gid, const tw_lpid from, const tw_lpid to,
                  const double bandwidth, const double load,
                  const double latency);

void registerSwitch(const tw_lpid gid, const double bandwidth,
                    const double load, const double latency);

void registerMaster(const tw_lpid gid, std::vector<tw_lpid> &&slaves,
                    ispd::scheduler::scheduler *const scheduler,
                    ispd::workload::Workload *const workload);

void registerUser(const std::string& name, const double energyConsumptionLimit);

const std::function<void(void *)> &getServiceInitializer(const tw_lpid gid);

const std::unordered_map<std::string, ispd::model::User>& getUsers();
}; // namespace ispd::this_model

#endif // ISPD_MODEL_BUILDER_HPP
