#ifndef ISPD_MODEL_BUILDER_HPP
#define ISPD_MODEL_BUILDER_HPP
#include <ross.h>
#include <vector>
#include <functional>
#include <unordered_map>
#include <ispd/log/log.hpp>
#include <ispd/model/user.hpp>
#include <ispd/allocator/allocator.hpp>
#include <ispd/workload/workload.hpp>
#include <ispd/scheduler/scheduler.hpp>

namespace ispd::model {

class SimulationModel {
public:
  using service_init_map_type =
      std::unordered_map<tw_lpid, std::function<void(void *)>>;
  using user_map_type = std::unordered_map<User::uid_t, User>;

  void registerMachine(const tw_lpid gid, const double power, const double load,
                       const unsigned coreCount, const double memory,
                       const double disk_space,const double wattageIdle,
                       const double wattageMax);

  void registerLink(const tw_lpid gid, const tw_lpid from, const tw_lpid to,
                    const double bandwidth, const double load,
                    const double latency);

  void registerSwitch(const tw_lpid gid, const double bandwidth,
                      const double load, const double latency);

  void registerMaster(const tw_lpid gid, std::vector<tw_lpid> &&slaves,
                      ispd::scheduler::scheduler *const scheduler,
                      ispd::workload::Workload *const workload);

  void registerVMM(const tw_lpid gid, std::vector<tw_lpid> &&vms, std::vector<double> &&vms_mem,
                   std::vector<double> &&vms_disk, std::vector<unsigned> &&vms_cores,
                   std::vector<tw_lpid> &&machines, ispd::allocator::allocator *const allocator,
                   ispd::scheduler::scheduler *const scheduler,
                   ispd::workload::Workload *const workload);

  void registerUser(const std::string &name,
                    const double energyConsumptionLimit);

  const std::function<void(void *)> &getServiceInitializer(const tw_lpid gid);

  inline const user_map_type &getUsers() const { return m_Users; }

  inline User &getUserById(const User::uid_t id) { return m_Users.at(id); }

  inline user_map_type::const_iterator getUserByName(const std::string &name) {
    return std::find_if(
        m_Users.cbegin(), m_Users.cend(),
        [&name](const auto &pair) { return pair.second.getName() == name; });
  }

private:
  service_init_map_type service_initializers;
  user_map_type m_Users;

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
};

}; // namespace ispd::model

namespace ispd::this_model {

void registerMachine(const tw_lpid gid, const double power, const double load,
                     const unsigned coreCount, const double memory,
                     const double disk_space,const double wattageIdle,
                     const double wattageMax);

void registerLink(const tw_lpid gid, const tw_lpid from, const tw_lpid to,
                  const double bandwidth, const double load,
                  const double latency);

void registerSwitch(const tw_lpid gid, const double bandwidth,
                    const double load, const double latency);

void registerMaster(const tw_lpid gid, std::vector<tw_lpid> &&slaves,
                    ispd::scheduler::scheduler *const scheduler,
                    ispd::workload::Workload *const workload);

void registerVMM(const tw_lpid gid, std::vector<tw_lpid> &&vms, std::vector<double> &&vms_mem,
                 std::vector<double> &&vms_disk, std::vector<unsigned> &&vms_cores,
                 std::vector<tw_lpid> &&machines, ispd::allocator::allocator *const allocator,
                 ispd::scheduler::scheduler *const scheduler,
                 ispd::workload::Workload *const workload);

void registerUser(const std::string &name, const double energyConsumptionLimit);

const std::function<void(void *)> &getServiceInitializer(const tw_lpid gid);

const ispd::model::SimulationModel::user_map_type &getUsers();

ispd::model::User &getUserById(ispd::model::User::uid_t id);

const ispd::model::SimulationModel::user_map_type::const_iterator
getUserByName(const std::string &name);
}; // namespace ispd::this_model

#endif // ISPD_MODEL_BUILDER_HPP
