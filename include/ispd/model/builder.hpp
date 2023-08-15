#ifndef ISPD_MODEL_BUILDER_HPP
#define ISPD_MODEL_BUILDER_HPP

#include <ross.h>
#include <vector>
#include <functional>
#include <unordered_map>
#include <ispd/log/log.hpp>
#include <ispd/model/user.hpp>
#include <ispd/workload/workload.hpp>
#include <ispd/scheduler/scheduler.hpp>

namespace ispd::model {

class SimulationModel {
  std::unordered_map<tw_lpid, std::function<void(void *)>> service_initializers;
  std::unordered_map<User::uid_t, User> m_Users;

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

  inline const std::unordered_map<User::uid_t, User>& getUsers() const {
    return m_Users; 
  } 

  inline std::unordered_map<User::uid_t, User>::const_iterator getUserByName(const std::string& name) {
    return std::find_if(m_Users.cbegin(), m_Users.cend(),
        [&name](const auto& pair) { return pair.second.getName() == name; });
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

const std::unordered_map<ispd::model::User::uid_t, ispd::model::User>& getUsers();

const std::unordered_map<ispd::model::User::uid_t, ispd::model::User>::const_iterator getUserByName(const std::string& name);
}; // namespace ispd::this_model

#endif // ISPD_MODEL_BUILDER_HPP
