#include <ross.h>
#include <memory>
#include <vector>
#include <fstream>
#include <unordered_map>
#include <ispd/log/log.hpp>
#include <lib/nlohmann/json.hpp>
#include <ispd/model/builder.hpp>
#include <ispd/workload/workload.hpp>
#include <ispd/scheduler/scheduler.hpp>
#include <ispd/scheduler/round_robin.hpp>
#include <ispd/workload/interarrival.hpp>
#include <ispd/model_loader/model_loader.hpp>

/// \brief User - Keys.
#define MODEL_USERS_SECTION ("users")
#define MODEL_USER_NAME_KEY ("name")
#define MODEL_USER_ENERGYLIMIT_KEY ("energy_consumption_limit")

/// \brief Workload - Keys.
#define MODEL_WORKLOADS_SECTION ("workloads")
#define MODEL_WORKLOAD_TYPE_KEY ("type")
#define MODEL_WORKLOAD_OWNER_KEY ("owner")
#define MODEL_WORKLOAD_REMAININGTASKS_KEY ("remaining_tasks")
#define MODEL_WORKLOAD_MASTERID_KEY ("master_id")
#define MODEL_WORKLOAD_COMPUTINGOFFLOAD_KEY ("computing_offload")
#define MODEL_WORKLOAD_INTERARRIVALTYPE_KEY ("interarrival_type")

#define MODEL_WORKLOAD_UNIFORM_MINPROCSIZE_KEY ("min_proc_size")
#define MODEL_WORKLOAD_UNIFORM_MAXPROCSIZE_KEY ("max_proc_size")
#define MODEL_WORKLOAD_UNIFORM_MINCOMMSIZE_KEY ("min_comm_size")
#define MODEL_WORKLOAD_UNIFORM_MAXCOMMSIZE_KEY ("max_comm_size")

/// \brief Interarrival - Keys.
#define MODEL_INTERARRIVAL_TYPE_KEY ("type")

#define MODEL_INTERARRIVAL_POISSON_LAMBDA_KEY ("lambda")

/// \brief Services - Keys.
#define MODEL_SERVICES_SECTION ("services")
#define MODEL_SERVICES_MASTER_SUBSECTION ("masters")
#define MODEL_SERVICES_MACHINES_SUBSECTION ("machines")
#define MODEL_SERVICES_LINKS_SUBSECTION ("links")
#define MODEL_SERVICES_SWITCHES_SUBSECTION ("switches")

#define MODEL_SERVICE_MASTER_ID_KEY ("id")
#define MODEL_SERVICE_MASTER_SCHEDULER_KEY ("scheduler")
#define MODEL_SERVICE_MASTER_SLAVES_KEY ("slaves")

#define MODEL_SERVICE_MACHINE_ID_KEY ("id")
#define MODEL_SERVICE_MACHINE_POWER_KEY ("power")
#define MODEL_SERVICE_MACHINE_LOAD_KEY ("load")
#define MODEL_SERVICE_MACHINE_CORECOUNT_KEY ("core_count")
#define MODEL_SERVICE_MACHINE_GPUPOWER_KEY ("gpu_power")
#define MODEL_SERVICE_MACHINE_GPUCORECOUNT_KEY ("gpu_core_count")
#define MODEL_SERVICE_MACHINE_GPUINTERCONNECTIONBANDWIDTH_KEY                  \
  ("gpu_interconnection_bandwidth")
#define MODEL_SERVICE_MACHINE_WATTAGEIDLE_KEY ("wattage_idle")
#define MODEL_SERVICE_MACHINE_WATTAGEMAX_KEY ("wattage_max")

#define MODEL_SERVICE_LINK_ID_KEY ("id")
#define MODEL_SERVICE_LINK_FROM_KEY ("from")
#define MODEL_SERVICE_LINK_TO_KEY ("to")
#define MODEL_SERVICE_LINK_BANDWIDTH_KEY ("bandwidth")
#define MODEL_SERVICE_LINK_LOAD_KEY ("load")
#define MODEL_SERVICE_LINK_LATENCY_KEY ("latency")

using json = nlohmann::json;

namespace ispd::model_loader {

std::unordered_map<tw_lpid, ispd::workload::Workload *> g_ModelLoader_Workloads;

/// \brief Loads user information from a JSON data structure and registers
///        users in the simulation model.
///
/// This function is responsible for parsing user information from a JSON
/// data structure and registering each user in the simulation model.
///
/// \param data The JSON data containing the model specification.
///
/// \note The function assumes a specific structure in the JSON data, where
///       users are listed under the "users" section. Each user is expected
///       to have attributes like "name" (MODEL_USER_NAME_KEY) and
///       "energy_limit_consumption" (MODEL_USER_ENERGYLIMIT_KEY).
///
/// \warning If the "users" section is missing in the provided JSON data,
///          the function will log an error using the ispd_error macro.
///          If any user is missing the required attributes, an error will
///          be logged as well.
///
/// \details The function iterates over each user in the "users" section
///          of the JSON data, checks for the presence of required attributes,
///          and registers the user in the simulation model using the
///          ispd::this_model::registerUser function.
///
/// \warning If any user is missing the required attributes, an error will be
///          logged using the ispd_error macro, specifying the index of the
///          user in the "users" section and the missing attribute.
///
/// \note The function is marked as noexcept to indicate that it does not
///       throw exceptions. Instead, it relies on error logging using the
///       ispd_error macro for error reporting.
static auto loadUsers(const json &data) noexcept -> void {
  // Checks if there is no users section in the model to be loaded.
  if (!data.contains(MODEL_USERS_SECTION))
    ispd_error("Model must have `users` section.");

  const json &users = data[MODEL_USERS_SECTION];
  size_t userIndex = 0;

  for (const auto &user : users) {
    // Check if the model specification does not specify the name of the user
    // to be registered in the model to be simulated.
    if (!user.contains(MODEL_USER_NAME_KEY))
      ispd_error("User listed at index %lu in model specification does not "
                 "have the `%s` attribute.",
                 userIndex, MODEL_USER_NAME_KEY);

    // Check if the model specification does not specify the energy consumption
    // of the user to be registered in the model to be simulated.
    if (!user.contains(MODEL_USER_ENERGYLIMIT_KEY))
      ispd_error("User listed at index %lu in model specification does noot "
                 "have the `%s` attribute.",
                 userIndex, MODEL_USER_ENERGYLIMIT_KEY);

    // Register the user in the model to be simulated.
    ispd::this_model::registerUser(user[MODEL_USER_NAME_KEY],
                                   user[MODEL_USER_ENERGYLIMIT_KEY]);

    userIndex++;
  }

  ispd_debug(
      "An amount of %lu users have been loaded from the model specification.",
      userIndex);
}

/// \brief Loads an Interarrival Distribution from a JSON workload specification.
///
/// This function parses and loads an Interarrival Distribution from a JSON
/// workload specification. The Interarrival Distribution type is specified
/// under the "interarrival_type" attribute in the workload JSON. Based on the
/// specified type, the function creates an instance of the corresponding
/// InterarrivalDistribution subclass and returns it as a unique pointer.
///
/// \param workload The JSON workload specification containing interarrival type.
/// \param workloadIndex The index of the workload in the model specification.
///
/// \note The function assumes that the workload JSON contains the
///       "interarrival_type" attribute, specifying the type of interarrival
///       distribution for the workload. If this attribute is missing, the
///       function logs an error using the ispd_error macro.
///
/// \note The function assumes that the interarrival type is specified as a
///       sub-object under the "interarrival_type" attribute. The type of
///       distribution is extracted from this sub-object, and based on the type,
///       an instance of the corresponding InterarrivalDistribution subclass is
///       created.
///
/// \return A unique pointer to an instance of the InterarrivalDistribution
///         subclass based on the specified type.
static auto loadInterarrivalDist(const json &workload,
                                 const size_t workloadIndex)
    -> std::unique_ptr<ispd::workload::InterarrivalDistribution> {
  // Checks if the current workload does not have the interarrival type
  // atttribute.
  if (!workload.contains(MODEL_WORKLOAD_INTERARRIVALTYPE_KEY))
    ispd_error("Workload listed at index %lu in model specification does not "
               "have the `%s` attribute.",
               workloadIndex, MODEL_WORKLOAD_INTERARRIVALTYPE_KEY);

  const json &interarrival = workload[MODEL_WORKLOAD_INTERARRIVALTYPE_KEY];
  const auto &type = interarrival[MODEL_INTERARRIVAL_TYPE_KEY];

  if (type == "poisson") {
    const auto &lambda = interarrival[MODEL_INTERARRIVAL_POISSON_LAMBDA_KEY];
    return std::make_unique<ispd::workload::PoissonInterarrivalDistribution>(
        lambda);
  } else {
    ispd_error("Unexpected `%s` interarrival distribution type.",
               type.get<std::string>().c_str());
    throw std::runtime_error("Unreachable!"); // Make the compiler happy!
  }
}

/// \brief Loads Workloads from a JSON model specification.
///
/// This function is responsible for parsing and loading Workloads from a JSON
/// model specification. Workloads are expected to be listed under the
/// "workloads" section in the provided JSON data. The function iterates over
/// each workload, checking for the presence of required attributes, and then
/// creates corresponding Workload objects based on the specified type.
///
/// \param data The JSON data representing the model specification.
///
/// \note The function assumes that the model specification contains the
///       "workloads" section, where each workload is specified as an object.
///       If this section is missing, an error is logged using the ispd_error
///       macro.
///
/// \note The function assumes that each workload specification includes
///       attributes such as "type," "owner," "remainingTasks," and
///       "masterId." It also extracts additional attributes based on the
///       workload type.
///
/// \note Currently, the function supports the "uniform" workload type. For
///       each uniform workload, it checks for additional attributes specific
///       to the uniform workload type. If an unexpected type is encountered,
///       an error is logged using the ispd_error macro.
///
/// \note The function uses the loadInterarrivalDist function to load the
///       Interarrival Distribution for each workload.
///
/// \note The loaded Workloads are registered in a temporary storage
///       (g_ModelLoader_Workloads) with master IDs as keys, as these will be
///       fetched later to register them with the masters.
///
/// \note The function logs debug messages indicating the successful loading
///       of each workload, including details such as the type, sizes, and
///       owner.
static auto loadWorkloads(const json &data) noexcept -> void {
  // Checks if there is no workloads section in the model to be loaded.
  if (!data.contains(MODEL_WORKLOADS_SECTION))
    ispd_error("Model must have `%s` section.", MODEL_WORKLOADS_SECTION);

  const json &workloads = data[MODEL_WORKLOADS_SECTION];
  size_t workloadIndex = 0;

  for (const auto &workload : workloads) {
    const auto &workloadRequiredAttributes = {
        MODEL_WORKLOAD_TYPE_KEY, MODEL_WORKLOAD_OWNER_KEY,
        MODEL_WORKLOAD_REMAININGTASKS_KEY, MODEL_WORKLOAD_MASTERID_KEY};

    // Checks if the current workload specifications has all the required
    // attributes.
    for (const auto &attribute : workloadRequiredAttributes)
      if (!workload.contains(attribute))
        ispd_error(
            "Workload listed at index %lu in model specification does not "
            "have the `%s` attribute.",
            workloadIndex, attribute);

    const auto &type = workload[MODEL_WORKLOAD_TYPE_KEY];
    const auto &owner = workload[MODEL_WORKLOAD_OWNER_KEY];
    const auto &remainingTasks = workload[MODEL_WORKLOAD_REMAININGTASKS_KEY];
    const auto &masterId = workload[MODEL_WORKLOAD_MASTERID_KEY];
    const auto &computingOffload =
        workload[MODEL_WORKLOAD_COMPUTINGOFFLOAD_KEY];

    ispd::workload::Workload *w;
    std::unique_ptr<ispd::workload::InterarrivalDistribution> interarrivalDist =
        loadInterarrivalDist(workload, workloadIndex);

    if (type == "uniform") {
      const auto &uniformWorkloadRequiredAttributes = {
          MODEL_WORKLOAD_UNIFORM_MINPROCSIZE_KEY,
          MODEL_WORKLOAD_UNIFORM_MAXPROCSIZE_KEY,
          MODEL_WORKLOAD_UNIFORM_MINCOMMSIZE_KEY,
          MODEL_WORKLOAD_UNIFORM_MAXCOMMSIZE_KEY};

      // Checks if the current uniform workload specifications has all the
      // required attributes.
      for (const auto &attribute : uniformWorkloadRequiredAttributes)
        if (!workload.contains(attribute))
          ispd_error("Uniform Workload listed at index %lu in model "
                     "specification does not "
                     "have the `%s` attribute.",
                     workloadIndex, attribute);

      const auto minProcSize =
          workload[MODEL_WORKLOAD_UNIFORM_MINPROCSIZE_KEY].get<double>();
      const auto maxProcSize =
          workload[MODEL_WORKLOAD_UNIFORM_MAXPROCSIZE_KEY].get<double>();
      const auto minCommSize =
          workload[MODEL_WORKLOAD_UNIFORM_MINCOMMSIZE_KEY].get<double>();
      const auto maxCommSize =
          workload[MODEL_WORKLOAD_UNIFORM_MAXCOMMSIZE_KEY].get<double>();

      w = new ispd::workload::UniformWorkload(
          owner, remainingTasks, minProcSize, maxProcSize, minCommSize,
          maxCommSize, computingOffload, std::move(interarrivalDist));

      ispd_debug("Uniform Workload (%.2lf, %.2lf, %.2lf, %.2lf) for master "
                 "with id %lu has been loaded from the model specification.",
                 minProcSize, maxProcSize, minCommSize, maxCommSize,
                 masterId.get<tw_lpid>());
    } else {
      ispd_error("Unexpected workload type %s.",
                 type.get<std::string>().c_str());
    }

    // Register the workload in a temporary storage, because this will be
    // fetched after to register them with the masters.
    g_ModelLoader_Workloads.insert(std::make_pair<>(masterId.get<tw_lpid>(), w));

    workloadIndex++;
  }

  ispd_debug("An amount of %lu workloads have been loaded from the model "
             "specification.",
             workloadIndex);
}

static auto loadMasterScheduler(const json &type) noexcept
    -> ispd::scheduler::Scheduler * {
  if (type == "RoundRobin") {
    return new ispd::scheduler::RoundRobin;
  } else {
    ispd_error("Unexepected %s scheduler.", type.get<std::string>().c_str());
  }
  return nullptr;
}

static auto loadMasterSlaves(const json &slavesArray) noexcept
    -> std::vector<tw_lpid> {
  std::vector<tw_lpid> slaves;
  for (const auto &slaveId : slavesArray)
    slaves.emplace_back(slaveId.get<tw_lpid>());
  return slaves;
}

static auto loadMaster(const json &master, const size_t masterIndex) noexcept
    -> void {
  const auto &masterRequiredAttributes = {MODEL_SERVICE_MASTER_ID_KEY,
                                          MODEL_SERVICE_MASTER_SCHEDULER_KEY,
                                          MODEL_SERVICE_MASTER_SLAVES_KEY};

  // Checks if the current master specification has all the required attributes.
  for (const auto &attribute : masterRequiredAttributes)
    if (!master.contains(attribute))
      ispd_error("Master listed at index %lu in model "
                 "specification does not "
                 "have the `%s` attribute.",
                 masterIndex, attribute);

  const tw_lpid id = master[MODEL_SERVICE_MASTER_ID_KEY];

  // Checks if there is no workloads registered to this master.
  if (g_ModelLoader_Workloads.find(id) == g_ModelLoader_Workloads.cend())
    ispd_error("No workloads have been loaded to master listed at %lu with "
               "identifier %lu.",
               masterIndex, id);

  ispd::scheduler::Scheduler *scheduler =
      loadMasterScheduler(master[MODEL_SERVICE_MASTER_SCHEDULER_KEY]);
  std::vector<tw_lpid> slaves =
      loadMasterSlaves(master[MODEL_SERVICE_MASTER_SLAVES_KEY]);
  ispd::workload::Workload *workload = g_ModelLoader_Workloads.at(id);

  // Register the master.
  ispd::this_model::registerMaster(id, std::move(slaves), scheduler, workload);

  ispd_debug("Master listed at %lu with identifier %lu has been loaded from "
             "the model specification.",
             masterIndex, id);
}

static auto loadMasters(const json &services) noexcept -> void {
  // Checks if there is no masters subsection in the services section.
  if (!services.contains(MODEL_SERVICES_MASTER_SUBSECTION))
    ispd_error("Services section must have `%s` subsection.",
               MODEL_SERVICES_MASTER_SUBSECTION);

  const auto &masters = services[MODEL_SERVICES_MASTER_SUBSECTION];
  size_t masterIndex = 0;

  for (const auto &master : masters) {
    loadMaster(master, masterIndex);
    masterIndex++;
  }

  ispd_debug("An amount of %lu masters have been loaded from the model specification.", masterIndex);
}

static auto loadMachine(const json &machine, const size_t machineIndex) noexcept
    -> void {
  const auto &machineRequiredAttributes = {
      MODEL_SERVICE_MACHINE_ID_KEY,
      MODEL_SERVICE_MACHINE_POWER_KEY,
      MODEL_SERVICE_MACHINE_LOAD_KEY,
      MODEL_SERVICE_MACHINE_CORECOUNT_KEY,
      MODEL_SERVICE_MACHINE_GPUPOWER_KEY,
      MODEL_SERVICE_MACHINE_GPUCORECOUNT_KEY,
      MODEL_SERVICE_MACHINE_GPUINTERCONNECTIONBANDWIDTH_KEY,
      MODEL_SERVICE_MACHINE_WATTAGEIDLE_KEY,
      MODEL_SERVICE_MACHINE_WATTAGEMAX_KEY,
  };

  // Checks if the current machine specification has all the required
  // attributes.
  for (const auto &attribute : machineRequiredAttributes)
    if (!machine.contains(attribute))
      ispd_error("Machine listed at index %lu in model "
                 "specification does not "
                 "have the `%s` attribute.",
                 machineIndex, attribute);

  const tw_lpid id = machine[MODEL_SERVICE_MACHINE_ID_KEY].get<tw_lpid>();
  const double power = machine[MODEL_SERVICE_MACHINE_POWER_KEY].get<double>();
  const double load = machine[MODEL_SERVICE_MACHINE_LOAD_KEY].get<double>();
  const unsigned coreCount =
      machine[MODEL_SERVICE_MACHINE_CORECOUNT_KEY].get<unsigned>();
  const double gpuPower =
      machine[MODEL_SERVICE_MACHINE_GPUPOWER_KEY].get<double>();
  const unsigned gpuCoreCount =
      machine[MODEL_SERVICE_MACHINE_GPUCORECOUNT_KEY].get<unsigned>();
  const double interconnectionBandwidth =
      machine[MODEL_SERVICE_MACHINE_GPUINTERCONNECTIONBANDWIDTH_KEY]
          .get<double>();
  const double wattageIdle =
      machine[MODEL_SERVICE_MACHINE_WATTAGEIDLE_KEY].get<double>();
  const double wattageMax =
      machine[MODEL_SERVICE_MACHINE_WATTAGEMAX_KEY].get<double>();

  // Registter the machine.
  ispd::this_model::registerMachine(id, power, load, coreCount, gpuPower,
                                    gpuCoreCount, interconnectionBandwidth,
                                    wattageIdle, wattageMax);

  ispd_debug("Machine listed at %lu with identifier %lu has been loaded from "
             "the model specification.",
             machineIndex, id);
}

static auto loadMachines(const json &services) noexcept -> void {
  // Checks if there is no machines subsection in the services section.
  if (!services.contains(MODEL_SERVICES_MACHINES_SUBSECTION))
    ispd_error("Services section must have `%s` subsection.",
               MODEL_SERVICES_MACHINES_SUBSECTION);

  const auto &machines = services[MODEL_SERVICES_MACHINES_SUBSECTION];
  size_t machineIndex = 0;

  for (const auto &machine : machines) {
    loadMachine(machine, machineIndex);
    machineIndex++;
  }

  ispd_debug("An amount of %lu machines have been loaded from the model "
             "specification.",
             machineIndex);
}

static auto loadLink(const json &link, const size_t linkIndex) noexcept
    -> void {
   const auto &linkRequiredAttributes = {
      MODEL_SERVICE_LINK_ID_KEY,
      MODEL_SERVICE_LINK_FROM_KEY,
      MODEL_SERVICE_LINK_TO_KEY,
      MODEL_SERVICE_LINK_BANDWIDTH_KEY,
      MODEL_SERVICE_LINK_LOAD_KEY,
      MODEL_SERVICE_LINK_LATENCY_KEY
  };

  // Checks if the current link specification has all the required
  // attributes.
  for (const auto &attribute : linkRequiredAttributes)
    if (!link.contains(attribute))
      ispd_error("Link listed at index %lu in model "
                 "specification does not "
                 "have the `%s` attribute.",
                 linkIndex, attribute);

  const tw_lpid id = link[MODEL_SERVICE_LINK_ID_KEY].get<tw_lpid>();
  const tw_lpid from = link[MODEL_SERVICE_LINK_FROM_KEY].get<tw_lpid>();
  const tw_lpid to = link[MODEL_SERVICE_LINK_TO_KEY].get<tw_lpid>();
  const double bandwidth = link[MODEL_SERVICE_LINK_BANDWIDTH_KEY].get<double>();
  const double load = link[MODEL_SERVICE_LINK_LOAD_KEY].get<double>();
  const double latency = link[MODEL_SERVICE_LINK_LATENCY_KEY].get<double>();

  // Register the link.
  ispd::this_model::registerLink(id, from, to, bandwidth, load, latency);

  ispd_debug("Link listed at %lu with identifier %lu has been loaded from "
             "the model specification.",
             linkIndex, id);
}

static auto loadLinks(const json &services) noexcept -> void {
  // Checks if there is no links subsection in the services section.
  if (!services.contains(MODEL_SERVICES_LINKS_SUBSECTION))
    ispd_error("Services section must have `%s` subsection.",
               MODEL_SERVICES_LINKS_SUBSECTION);

  const auto &links = services[MODEL_SERVICES_LINKS_SUBSECTION];
  size_t linkIndex = 0;

  for (const auto &link : links) {
    loadLink(link, linkIndex);
    linkIndex++;
  }

  ispd_debug("An amount of %lu links have been loaded from the model "
             "specification.",
             linkIndex);
}

static auto loadServices(const json &data) noexcept -> void {
  // Checks if there is no services section in the model to be loaded.
  if (!data.contains(MODEL_SERVICES_SECTION))
    ispd_error("Model must have `%s` section.", MODEL_SERVICES_SECTION);

  const auto &services = data[MODEL_SERVICES_SECTION];

  loadMasters(services);
  loadMachines(services);
  loadLinks(services);
}

void loadModel(const std::filesystem::path modelPath) {
  // Checks if the specified model file path does not exists.
  if (!std::filesystem::exists(modelPath))
    ispd_error("Model path %s does not exists.", modelPath.c_str());

  std::ifstream f(modelPath);
  json data = json::parse(f);

  loadUsers(data);
  loadWorkloads(data);
  loadServices(data);
}
} // namespace ispd::model_loader