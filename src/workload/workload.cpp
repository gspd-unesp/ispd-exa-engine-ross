#include <ispd/log/log.hpp>
#include <ispd/model/builder.hpp>
#include <ispd/workload/workload.hpp>

namespace ispd::workload {

Workload::Workload(const std::string& user, unsigned remainingTasks)
    : m_User(user), m_RemainingTasks(remainingTasks) {
    
    // Fetch the registered users in the simulated model.
    const auto& registeredUsers = ispd::this_model::getUsers();

    // Check if the user registering the workload is valid.
    if (registeredUsers.find(user) == registeredUsers.end()) {
        ispd_error("Creating a workload with an unregistered user: %s.", user.c_str());
    }
}

ConstantWorkload::ConstantWorkload(const std::string& user,
                            const unsigned remainingTasks,
                            const double constantProcSize,
                            const double constantCommSize)
      : Workload(user, remainingTasks), m_ConstantProcSize(constantProcSize),
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

UniformWorkload::UniformWorkload(const std::string& user,
                           const unsigned remainingTasks,
                           const double minProcSize, const double maxProcSize,
                           const double minCommSize, const double maxCommSize)
      : Workload(user, remainingTasks), m_MinProcSize(minProcSize),
        m_MaxProcSize(maxProcSize), m_MinCommSize(minCommSize),
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

NullWorkload::NullWorkload() : Workload("", 0) {}

ConstantWorkload *constant(const std::string& user,
                                         const unsigned remainingTasks,
                                         const double constantProcSize,
                                         const double constantCommSize) {
  return new ConstantWorkload(user,
                              remainingTasks,
                              constantProcSize,
                              constantCommSize);
}

UniformWorkload *uniform(const std::string& user,
                                       const unsigned remainingTasks,
                                       const double minProcSize,
                                       const double maxProcSize,
                                       const double minCommSize,
                                       const double maxCommSize) {
  return new UniformWorkload(user, remainingTasks, 
                             minProcSize, maxProcSize,
                             minCommSize, maxCommSize);
}

NullWorkload *null() {
    /// The function simply creates and returns a new instance of the `NullWorkload` class.
    /// @Note: After, it is possibel the use of a singleton instance and lazy-initialization.
    return new NullWorkload();
}

}; // namespace ispd::workload
