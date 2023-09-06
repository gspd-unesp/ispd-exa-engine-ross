#include <ispd/log/log.hpp>
#include <ispd/model/builder.hpp>
#include <ispd/workload/workload.hpp>

namespace ispd::workload {

[[nodiscard]] Workload::Workload(
    const std::string &owner, const unsigned remainingTasks,
    const double computingOffload,
    std::unique_ptr<InterarrivalDistribution> interarrivalDist) noexcept {
  // Fetch the registered users in the simulated model.
  const auto &registeredUsers = ispd::this_model::getUsers();
  const auto &userIterator = ispd::this_model::getUserByName(owner);

  // Check if the user registering the workload is valid.
  if (userIterator == registeredUsers.end()) {
    ispd_error("Creating a workload with an unregistered user: %s.",
               owner.c_str());
  }

  m_Owner = userIterator->second.getId();
  m_RemainingTasks = remainingTasks;
  m_InterarrivalDist = std::move(interarrivalDist);
  m_ComputingOffload = computingOffload;
}

[[nodiscard]] ConstantWorkload::ConstantWorkload(
    const std::string &user, const unsigned remainingTasks,
    const double constantProcSize, const double constantCommSize,
    const double computingOffload,
    std::unique_ptr<InterarrivalDistribution> interarrivalDist) noexcept
    : Workload(user, remainingTasks, computingOffload,
               std::move(interarrivalDist)),
      m_ConstantProcSize(constantProcSize),
      m_ConstantCommSize(constantCommSize) {
  if (constantProcSize <= 0.0)
    ispd_error("Constant processing size must be positive (Specified "
               "constant processing size: %lf).",
               constantProcSize);

  if (constantCommSize <= 0.0)
    ispd_error("Constant communication size must be positive (Specified "
               "constant communication size: %lf).",
               constantCommSize);

  ispd_debug("[Constant Workload] PS: %lf, CS: %lf, RT: %u.", constantProcSize,
             constantCommSize, remainingTasks);
}

[[nodiscard]] UniformWorkload::UniformWorkload(
    const std::string &user, const unsigned remainingTasks,
    const double minProcSize, const double maxProcSize,
    const double minCommSize, const double maxCommSize,
    const double computingOffload,
    std::unique_ptr<InterarrivalDistribution> interarrivalDist) noexcept
    : Workload(user, remainingTasks, computingOffload,
               std::move(interarrivalDist)),
      m_MinProcSize(minProcSize), m_MaxProcSize(maxProcSize),
      m_MinCommSize(minCommSize), m_MaxCommSize(maxCommSize) {
  if (minProcSize <= 0.0)
    ispd_error("Minimum processing size must be positive (Specified "
               "minimum processing size: %lf).",
               minProcSize);

  if (maxProcSize <= 0.0)
    ispd_error("Maximum processing size must be positive (Specified "
               "maximum processing size: %lf).",
               maxProcSize);

  if (minCommSize <= 0.0)
    ispd_error("Minimum communication size must be positive (Specified "
               "minimum communication size: %lf).",
               minCommSize);

  if (maxCommSize <= 0.0)
    ispd_error("Maximum communication size must be positive (Specified "
               "maximum communication size: %lf).",
               maxCommSize);

  ispd_debug("[Uniform Workload] PI: [%lf, %lf], CI: [%lf, %lf], RT: %u.",
             minProcSize, maxProcSize, minCommSize, maxCommSize,
             remainingTasks);
}

[[nodiscard]] NullWorkload::NullWorkload(const std::string &user) noexcept
    : Workload(user, 0, 0, nullptr) {}

ConstantWorkload *
constant(const std::string &user, const unsigned remainingTasks,
         const double constantProcSize, const double constantCommSize,
         const double computingOffload,
         std::unique_ptr<InterarrivalDistribution> interarrivalDist) {
  return new ConstantWorkload(user, remainingTasks, constantProcSize,
                              constantCommSize, computingOffload,
                              std::move(interarrivalDist));
}

UniformWorkload *
uniform(const std::string &user, const unsigned remainingTasks,
        const double minProcSize, const double maxProcSize,
        const double minCommSize, const double maxCommSize,
        const double computingOffload,
        std::unique_ptr<InterarrivalDistribution> interarrivalDist) {
  return new UniformWorkload(user, remainingTasks, minProcSize, maxProcSize,
                             minCommSize, maxCommSize, computingOffload,
                             std::move(interarrivalDist));
}

NullWorkload *null(const std::string &user) { return new NullWorkload(user); }

}; // namespace ispd::workload
