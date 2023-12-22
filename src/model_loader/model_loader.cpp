#include <ross.h>
#include <memory>
#include <iostream>
#include <fstream>
#include <typeinfo>
#include <unordered_map>
#include <ispd/log/log.hpp>
#include <lib/nlohmann/json.hpp>
#include <ispd/model/builder.hpp>
#include <ispd/workload/workload.hpp>
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

void loadModel(const std::filesystem::path modelPath) {
  // Checks if the specified model file path does not exists.
  if (!std::filesystem::exists(modelPath))
    ispd_error("Model path %s does not exists.", modelPath.c_str());

  std::ifstream f(modelPath);
  json data = json::parse(f);

  loadUsers(data);
  loadWorkloads(data);
}
} // namespace ispd::model_loader