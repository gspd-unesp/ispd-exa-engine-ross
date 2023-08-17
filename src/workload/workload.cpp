#include <ispd/log/log.hpp>
#include <ispd/model/builder.hpp>
#include <ispd/workload/workload.hpp>

namespace ispd::workload {

Workload::Workload(const std::string& owner,
                   const unsigned remainingTasks,
                   std::unique_ptr<InterarrivalDistribution> interarrivalDist) {
    // Fetch the registered users in the simulated model.
    const auto& registeredUsers = ispd::this_model::getUsers();
    const auto& userIterator = ispd::this_model::getUserByName(owner);

    // Check if the user registering the workload is valid.
    if (userIterator == registeredUsers.end()) {
        ispd_error("Creating a workload with an unregistered user: %s.", owner.c_str());
    }

    m_Owner = userIterator->second.getId();
    m_RemainingTasks = remainingTasks;
    m_InterarrivalDist = std::move(interarrivalDist);
}

/// The cloud's version
Workload::Workload(const std::string& owner,
                   const unsigned remainingTasks,
                   const unsigned remainingVms,
                   std::unique_ptr<InterarrivalDistribution> interarrivalDist) {
    // Fetch the registered users in the simulated model.
    const auto& registeredUsers = ispd::this_model::getUsers();
    const auto& userIterator = ispd::this_model::getUserByName(owner);

    // Check if the user registering the workload is valid.
    if (userIterator == registeredUsers.end()) {
        ispd_error("Creating a workload with an unregistered user: %s.", owner.c_str());
    }

    m_Owner = userIterator->second.getId();
    m_RemainingTasks = remainingTasks;
    m_RemainingVms = remainingVms;
    m_InterarrivalDist = std::move(interarrivalDist);
}

ConstantWorkload::ConstantWorkload(const std::string& user,
                            const unsigned remainingTasks,
                            const double constantProcSize,
                            const double constantCommSize,
                            std::unique_ptr<InterarrivalDistribution> interarrivalDist)
      : Workload(user, remainingTasks, std::move(interarrivalDist)),
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

    ispd_debug("[Constant Workload] PS: %lf, CS: %lf, RT: %u.",
               constantProcSize, constantCommSize, remainingTasks);
}



/// the cloud's version
ConstantWorkload::ConstantWorkload(const std::string& user,
                                   const unsigned remainingTasks,
                                   const unsigned remainingVms,
                                   const double constantProcSize,
                                   const double constantCommSize,
                                   std::unique_ptr<InterarrivalDistribution> interarrivalDist)
    : Workload(user, remainingTasks, remainingVms, std::move(interarrivalDist)),
      m_ConstantProcSize(constantProcSize),
      m_ConstantCommSize(constantCommSize){
    if (constantProcSize <= 0.0)
      ispd_error("Constant processing size must be positive (Specified "
                 "constant processing size: %lf).",
                 constantProcSize);

    if (constantCommSize <= 0.0)
      ispd_error("Constant communication size must be positive (Specified "
                 "constant communication size: %lf).",
                 constantCommSize);

    ispd_debug("[Constant Workload] PS: %lf, CS: %lf, RT: %u.",
               constantProcSize, constantCommSize, remainingTasks);
}
UniformWorkload::UniformWorkload(const std::string& user,
                           const unsigned remainingTasks,
                           const double minProcSize, const double maxProcSize,
                           const double minCommSize, const double maxCommSize,
                           std::unique_ptr<InterarrivalDistribution> interarrivalDist)
      : Workload(user, remainingTasks, std::move(interarrivalDist)), 
        m_MinProcSize(minProcSize),
        m_MaxProcSize(maxProcSize),
        m_MinCommSize(minCommSize),
        m_MaxCommSize(maxCommSize) {
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

NullWorkload::NullWorkload(const std::string& user) : Workload(user, 0, nullptr) {}

ConstantWorkload *constant(const std::string& user,
                           const unsigned remainingTasks,
                           const double constantProcSize,
                           const double constantCommSize,
                           std::unique_ptr<InterarrivalDistribution> interarrivalDist) {
  return new ConstantWorkload(user,
                              remainingTasks,
                              constantProcSize,
                              constantCommSize,
                              std::move(interarrivalDist));
}

ConstantWorkload *constant(const std::string& user,
                           const unsigned remainingTasks,
                           const unsigned remainingVms,
                           const double constantProcSize,
                           const double constantCommSize,
                           std::unique_ptr<InterarrivalDistribution> interarrivalDist) {
  return new ConstantWorkload(user,
                              remainingTasks,
                              remainingVms,
                              constantProcSize,
                              constantCommSize,
                              std::move(interarrivalDist));
}
UniformWorkload *uniform(const std::string& user,
                         const unsigned remainingTasks,
                         const double minProcSize,
                         const double maxProcSize,
                         const double minCommSize,
                         const double maxCommSize,
                         std::unique_ptr<InterarrivalDistribution> interarrivalDist) {
  return new UniformWorkload(user, remainingTasks, 
                             minProcSize, maxProcSize,
                             minCommSize, maxCommSize,
                             std::move(interarrivalDist));
}

NullWorkload *null(const std::string& user) {
  return new NullWorkload(user);
}

}; // namespace ispd::workload
