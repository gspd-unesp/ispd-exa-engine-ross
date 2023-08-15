#include <sstream>
#include <string>
#include <algorithm>
#include <ispd/model/builder.hpp>
#include <ispd/services/master.hpp>
#include <ispd/services/link.hpp>
#include <ispd/services/machine.hpp>
#include <ispd/services/switch.hpp>

static inline std::string firstSlaves(const std::vector<tw_lpid> &slaves) {
  const auto maxToShow = std::vector<tw_lpid>::size_type(10);
  const auto slavesToShowCount = std::min(maxToShow, slaves.size());

  std::stringstream ss;

  for (int i = 0; i < slavesToShowCount - 1; i++)
    ss << slaves[i] << ", ";
  ss << slaves[slavesToShowCount - 1];
  return ss.str();
}

namespace ispd::model {

void SimulationModel::registerMachine(const tw_lpid gid, const double power,
                                      const double load,
                                      const unsigned coreCount) {
  /// Check if the power is not positive. If so, an error indicating the
  /// case is sent and the program is immediately aborted.
  if (power <= 0.0)
    ispd_error("At registering the machine %lu the power must be positive "
               "(Specified Power: %lf).",
               gid, power);

  /// Check if the load is not in the interval [0, 1]. If so, an error
  /// indicating the case is sent and the program is immediately aborted.
  if (load < 0.0 || load > 1.0)
    ispd_error("At registering the machine %lu the load must be in the "
               "interval [0, 1] (Specified Load: %lf).",
               gid, load);

  /// Check if the core count is not positive. If so, an error indicating
  /// the case is sent and the program is immediately aborted.
  if (coreCount <= 0)
    ispd_error(
        "At registering  the machine %lu the core count must be positive "
        "(Specified Core Count: %u).",
        gid, coreCount);

  /// Register the service initializer for a machine with the specified
  /// logical process global identifier (GID).
  registerServiceInitializer(gid, [=](void *state) {
    ispd::services::machine_state *s =
        static_cast<ispd::services::machine_state *>(state);

    /// Initialize machine's configuration.
    s->conf = ispd::services::MachineConfiguration(power, load, coreCount);
    s->cores_free_time.resize(coreCount, 0.0);
  });

  /// Print a debug indicating that a machine initializer has been registered.
  ispd_debug(
      "A machine with GID %lu has been registered (P: %lf, L: %lf, C: %u).",
      gid, power, load, coreCount);
}

void SimulationModel::registerLink(const tw_lpid gid, const tw_lpid from,
                                   const tw_lpid to, const double bandwidth,
                                   const double load, const double latency) {
  /// Check if the bandwidth is not positive. If so, an error indicating the
  /// case is sent and the program is immediately aborted.
  if (bandwidth <= 0.0)
    ispd_error("At registering the link %lu the bandwidth must be positive "
               "(Specified Bandwidth: %lf).",
               gid, bandwidth);

  /// Check if the load is not in the interval [0, 1]. If so, an error
  /// indicating the case is sent and the program is immediately aborted.
  if (load < 0.0 || load > 1.0)
    ispd_error("At registering the link %lu the load must be in the "
               "interval [0, 1] (Specified Load: %lf).",
               gid, load);

  /// Check if the latency is not positive. If so, an error indicating the case
  /// is sent and the program is immediately aborted.
  if (latency < 0.0)
    ispd_error("At registering the link %lu the latency must be positive "
               "(Specified Latency: %lf).",
               gid, latency);

  /// Register the service initializer for a link with the specified
  /// logical process global identifier (GID).
  registerServiceInitializer(gid, [=](void *state) {
    ispd::services::link_state *s =
        static_cast<ispd::services::link_state *>(state);

    /// Initialize the link's ends.
    s->from = from;
    s->to = to;

    /// Initialize the link's configuration.
    s->conf.bandwidth = bandwidth;
    s->conf.load = load;
    s->conf.latency = latency;
  });

  /// Print a debug indicating that a link initializer has been registered.
  ispd_debug(
      "A link with GID %lu has been registered (B: %lf, L: %lf, LT: %lf).", gid,
      bandwidth, load, latency);
}

void SimulationModel::registerSwitch(const tw_lpid gid, const double bandwidth,
                                     const double load, const double latency) {
  /// Check if the bandwidth is not positive. If so, an error indicating the
  /// case is sent and the program is immediately aborted.
  if (bandwidth <= 0.0)
    ispd_error("At registering the switch %lu the bandwidth must be positive "
               "(Specified Bandwidth: %lf).",
               gid, bandwidth);

  /// Check if the load is not in the interval [0, 1]. If so, an error
  /// indicating the case is sent and the program is immediately aborted.
  if (load < 0.0 || load > 1.0)
    ispd_error("At registering the switch %lu the load must be in the "
               "interval [0, 1] (Specified Load: %lf).",
               gid, load);

  /// Check if the latency is not positive. If so, an error indicating the case
  /// is sent and the program is immediately aborted.
  if (latency < 0.0)
    ispd_error("At registering the switch %lu the latency must be positive "
               "(Specified Latency: %lf).",
               gid, latency);

  /// Register the service initializer for a switch with the specified
  /// logical process global identifier (GID).
  registerServiceInitializer(gid, [=](void *state) {
    ispd::services::SwitchState *s =
        static_cast<ispd::services::SwitchState *>(state);

    /// Initialize the switch's configuration.
    s->m_Conf.m_Bandwidth = bandwidth;
    s->m_Conf.m_Load = load;
    s->m_Conf.m_Latency = latency;
  });

  /// Print a debug indicating that a switch initializer has been registered.
  ispd_debug(
      "A switch with GID %lu has been registered (B: %lf, L: %lf, LT: %lf).",
      gid, bandwidth, load, latency);
}

void SimulationModel::registerMaster(
    const tw_lpid gid, std::vector<tw_lpid> &&slaves,
    ispd::scheduler::scheduler *const scheduler,
    ispd::workload::Workload *const workload) {

  /// Check if the scheduler has not been specified. If so, an error indicating
  /// the case is sent and the program is immediately aborted.
  if (!scheduler)
    ispd_error(
        "At registering the master %lu the scheduler has not been specified.",
        gid);

  /// Check if the workload has not been specified. If so, an error indicating
  /// the case is sent and the program is immediately aborted.
  if (!workload)
    ispd_error(
        "At registering the master %lu the workload has not been specified.",
        gid);

  const auto slaveCount = slaves.size();
  const auto someSlaves = firstSlaves(slaves);

  /// Register the service initializer for a master with the specified
  /// logical process global identifier.
  registerServiceInitializer(gid, [workload, scheduler, &slaves](void *state) {
    ispd::services::master_state *s =
        static_cast<ispd::services::master_state *>(state);

    /// Specify the master's slaves.
    s->slaves = std::move(slaves);

    /// Specify the master's schedule and workload.
    s->scheduler = scheduler;
    s->workload = workload;
  });

  /// Print a debug indicating that a master initializer has been registered.
  ispd_debug("A master with GID %lu has been registered (SC: %u, S: %s).", gid,
             slaveCount, someSlaves.c_str());
}

void SimulationModel::registerUser(const std::string& name, const double energyConsumptionLimit) {
  /// Checks if a user with that name has already been regisitered. If so, the
  /// program is immediately aborted, since unique named users are mandatory.
  if (getUserByName(name) != m_Users.end())
    ispd_error("A user named %s has already been registered.", name.c_str());
  
  /// Checks if the specified energy consumption limit is not finite. If so, the
  /// program is immediately aborted, since a finite limit must be specified.
  if (!std::isfinite(energyConsumptionLimit))
    ispd_error("The specified energy consumption limit for user %s must be finite.", name.c_str());

  /// Checks if the specified energy consumption limit is negative. If so, the
  /// program is immediately aborted, since a non-negative limit must be specified.
  if (energyConsumptionLimit < 0.0)
    ispd_error("The specified energy consumption limit for user %s must be positive.", name.c_str());

  /// A copy of the specified name to be checked.
  std::string checkedName = name;
  
  /// Remove all blank spaces from the specified name.
  std::remove(checkedName.begin(), checkedName.end(), ' ');

  /// Checks if the specified name is empty or only contains blank spaces. If so, the
  /// program is immediately aborted.
  if (checkedName.size() == 0)
    ispd_error("An invalid username has been specified. It must contain at least one letter.");
  
  /// Assign automatically a user identifier.
  const uid_t id = static_cast<uid_t>(m_Users.size());

  /// Construct the user and insert into the users mapping.
  m_Users.emplace(id, User(id, name, energyConsumptionLimit));
  
  ispd_debug("A user named %s with consumption limit of %.2lf has been registered.", name.c_str(), energyConsumptionLimit);
}

const std::function<void(void *)> &
SimulationModel::getServiceInitializer(const tw_lpid gid) {
  /// Checks if a service initializer for the specified global identifier has
  /// not been registered. If so, the program is immediately aborted, since
  /// service initializer is mandatory for every service.
  if (service_initializers.find(gid) == service_initializers.end())
    ispd_error(
        "A service initializer for service with GID %lu has not been found.",
        gid);
  return service_initializers.at(gid);
}
}; // namespace ispd::model

namespace ispd::this_model {

/// The simulation model global variable.
ispd::model::SimulationModel *g_Model = new ispd::model::SimulationModel();

void registerMachine(const tw_lpid gid, const double power, const double load,
                     const unsigned coreCount) {
  /// Forward the machine registration to the global model.
  g_Model->registerMachine(gid, power, load, coreCount);
}

void registerLink(const tw_lpid gid, const tw_lpid from, const tw_lpid to,
                  const double bandwidth, const double load,
                  const double latency) {
  /// Forward the link registration to the global model.
  g_Model->registerLink(gid, from, to, bandwidth, load, latency);
}

void registerSwitch(const tw_lpid gid, const double bandwidth,
                    const double load, const double latency) {
  /// Forward the switch registration to the global model.
  g_Model->registerSwitch(gid, bandwidth, load, latency);
}

void registerMaster(const tw_lpid gid, std::vector<tw_lpid> &&slaves,
                    ispd::scheduler::scheduler *const scheduler,
                    ispd::workload::Workload *const workload) {
  /// Forward the master registration to the global model.
  g_Model->registerMaster(gid, std::move(slaves), scheduler, workload);
}

void registerUser(const std::string& name, const double energyConsumptionLimit) {
  /// Forward the user registration to the global model.
  g_Model->registerUser(name, energyConsumptionLimit);
}

const std::function<void(void *)> &getServiceInitializer(const tw_lpid gid) {
  /// Forward the service initializer query to the global model.
  return g_Model->getServiceInitializer(gid);
}

const std::unordered_map<ispd::model::User::uid_t, ispd::model::User>& getUsers() {
  /// Forward the users query to the global model.
  return g_Model->getUsers();
}

const std::unordered_map<ispd::model::User::uid_t, ispd::model::User>::const_iterator getUserByName(const std::string& name) {
  /// Forward the user query by name to the global model.
  return g_Model->getUserByName(name);
}

}; // namespace ispd::this_model
