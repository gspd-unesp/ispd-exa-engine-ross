#include <ispd/log/log.hpp>
#include <ispd/model/builder.hpp>
#include <ispd/cloud_workload/cloud_workload.hpp>

namespace ispd::cloud_workload{

    [[nodiscard]] CloudWorkload::CloudWorkload(
            const std::string &owner, const unsigned remainingApplications,
            std::unique_ptr<ispd::workload::InterarrivalDistribution> interarrivalDist) noexcept{
    const auto &registers_users = ispd::this_model::getUsers();
    const auto &userIterator = ispd::this_model::getUserByName(owner);

    // Check if the user registering the workload is valid.
    if (userIterator == registers_users.end()) {
    ispd_error("Creating a workload with an unregistered user: %s.",
    owner.c_str());
}


m_Owner = userIterator->second.getId();
m_RemainingApplications = remainingApplications;
m_InterarrivalDist = std::move(interarrivalDist);

}


[[nodiscard]] ConstantCloudWorkload::ConstantCloudWorkload(
        const std::string &user, const unsigned remainingApplications,
        const double constantCommSize,const double constantProcSize,
        std::unique_ptr<ispd::workload::InterarrivalDistribution> interarrivalDist) noexcept
: CloudWorkload(user, remainingApplications,
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

    ispd_debug("[Constant Cloud Workload] PS: %lf, CS: %lf, RT: %u.", constantProcSize,
               constantCommSize, remainingApplications);
}

ConstantCloudWorkload *constant(const std::string &owner, const unsigned remainingApplications,
                                const double constantCommSize,const double constantProcSize,
                                std::unique_ptr<ispd::workload::InterarrivalDistribution> interarrivalDist)
{
        return new ConstantCloudWorkload(owner, remainingApplications, constantCommSize, constantProcSize,
                                         std::move(interarrivalDist));
}


};