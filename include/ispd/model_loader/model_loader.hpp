#pragma once

#include <ross.h>
#include <filesystem>
#include <unordered_map>

namespace ispd::model_loader {

/// \brief Logical Process Types.
///
/// This enumeration lists the available logical process types used in a
/// discrete-event simulation. Each logical process type corresponds to a
/// specific role within the simulation model. The numbers assigned to each
/// logical process type must match the values used in `lps_type` when
/// configuring logical processes with `tw_lp_settype`.
///
/// \note Logical processes represent entities in the simulation model, and each
///       type serves a unique purpose.
///
/// \details The available logical process types are:
///   - MASTER: Represents the master service center.
///   - LINK: Represents a communication link.
///   - MACHINE: Represents a computational node or machine that performs tasks.
///   - SWITCH: Represents a network switch for communication in a distributed
///   system.
///   - DUMMY: Represents a dummy logical process with no specific role.
///
/// \note The numbering of the logical process types (0 for MASTER, 1 for LINK,
///       and so on) is crucial for compatibility with the configuration of
///       logical process types using `tw_lp_settype`.
///
/// \see tw_lp_settype
enum LogicalProcessType {
  MASTER = 0,
  LINK = 1,
  MACHINE = 2,
  SWITCH = 3,
  DUMMY = 4
};

auto loadModel(const std::filesystem::path modelPath) noexcept -> void;

[[nodiscard]] auto getLogicalProcessType(const tw_lpid gid) noexcept
    -> LogicalProcessType;

[[nodiscard]] auto getServicesSize() noexcept
    -> std::unordered_map<tw_lpid, LogicalProcessType>::size_type;

} // namespace ispd::model_loader