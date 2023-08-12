#ifndef ISPD_SERVICES_HPP
#define ISPD_SERVICES_HPP

#include <array>
#include <unordered_map>

namespace ispd::services {

enum class ServiceType {
  MASTER,
  LINK,
  MACHINE,
  SWITCH,
};

constexpr std::array<ServiceType, 4> g_ServiceTypes = {
  ServiceType::MASTER,
  ServiceType::LINK,
  ServiceType::MACHINE,
  ServiceType::SWITCH,
};

constexpr const char* getServiceTypeName(const ServiceType type) {
  switch (type) {
    case ServiceType::MASTER: return "master";
    case ServiceType::LINK: return "link";
    case ServiceType::MACHINE: return "machine";
    case ServiceType::SWITCH: return "switch";
    default:
      return nullptr;
  } 
}

}; // namespace ispd::services

#endif // ISPD_SERVICES_HPP
