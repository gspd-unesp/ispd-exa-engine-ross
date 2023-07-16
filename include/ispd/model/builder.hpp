#ifndef ISPD_MODEL_BUILDER_HPP
#define ISPD_MODEL_BUILDER_HPP

#include <ross.h>
#include <functional>
#include <unordered_map>
#include <ispd/log/log.hpp>

namespace ispd {
namespace model {

struct built_model {
  std::unordered_map<tw_lpid, std::function<void(void *)>> service_initializers;
};

}; // namespace model
}; // namespace ispd

extern ispd::model::built_model g_built_model;

namespace ispd {
namespace model {

struct builder {
  static inline void
  register_service_initializer(const tw_lpid gid,
                               std::function<void(void *)> initializer) {
    /// Checks if a service with the specified global identifier has already
    /// been registered. If so, the program is immediately aborted.
    if (g_built_model.service_initializers.find(gid) !=
        g_built_model.service_initializers.end())
      ispd_error("A service with GID %lu has already been registered.", gid);

    /// Emplace the pair (gid, initializer).
    g_built_model.service_initializers.emplace(gid, initializer);

    /// Print a debug indicating that a service initializer to the specified
    /// global identifier has been registered.
    ispd_debug(
        "A service initializer for a service with GID %lu has been registered.",
        gid);
  }

  static inline const std::function<void(void *)> &
  get_service_initializer(const tw_lpid gid) {
    /// Checks if a service initializer for the specified global identifier has
    /// not been registered. If so, the program is immediately aborted, since
    /// service initializer is mandatory for every service.
    if (g_built_model.service_initializers.find(gid) ==
        g_built_model.service_initializers.end())
      ispd_error(
          "A service initializer for service with GID %lu has not been found.",
          gid);
    return g_built_model.service_initializers.at(gid);
  }
};

}; // namespace model
}; // namespace ispd

#endif // ISPD_MODEL_BUILDER_HPP
