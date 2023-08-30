/// This file contains the definition of the ispd::services namespace and related functions.
/// The namespace defines the ServiceType enumeration class and provides a set of functions
/// to interact with it.
///
/// The ServiceType enumeration class represents different types of service centers within
/// a simulation framework. The namespace also includes functions to convert these service
/// types into human-readable strings, either in capitalized or lowercase form, based on
/// the provided template parameter.
///
/// The g_ServiceTypes array provides a pre-defined collection of all available service types
/// as an array, which can be useful for iterating over the available service types in a
/// loop or performing other operations that involve all service types.
///
#ifndef ISPD_SERVICES_HPP
#define ISPD_SERVICES_HPP

#include <array>

namespace ispd::services {

/// \brief Enumeration class representing different types of service centers.
///
/// The ServiceType enumeration class defines a set of named constants, each representing
/// a distinct type of service center in a simulation framework. These constants serve
/// as identifiers for various service center types, making it easier to categorize and
/// manage different services.
enum class ServiceType {
  MASTER,   ///< Represents a master service center type.
  LINK,     ///< Represents a link service center type.
  MACHINE,  ///< Represents a machine service center type.
  SWITCH,    ///< Represents a switch service center type.
  VMM,      /// < Represents a vmm service center type
  VIRTUAL_MACHINE /// < Represents a virtual machine service center type
};

/// \brief Array containing all available service types.
///
/// The g_ServiceTypes array contains all available service types as elements. It is defined
/// as a constexpr std::array<ServiceType, 6> to ensure that it is available at compile time.
constexpr std::array<ServiceType, 6> g_ServiceTypes = {
  ServiceType::MASTER,
  ServiceType::LINK,
  ServiceType::MACHINE,
  ServiceType::SWITCH,
  ServiceType::VMM,
  ServiceType::VIRTUAL_MACHINE,
};

/// \brief Get the name of a service type as a string.
///
/// This function returns the name of a given ServiceType as a string. The template parameter
/// _Capitalized determines whether the returned string should be in capitalized or lowercase form.
///
/// \tparam _Capitalized Specify true for capitalized string, false for lowercase string.
/// \param type The ServiceType value for which the name is to be retrieved.
/// \return A const char pointer to the name of the specified service type.
template <bool _Capitalized = false>
constexpr const char* getServiceTypeName(const ServiceType type) {
  if constexpr (_Capitalized) {
    switch (type) {
        case ServiceType::MASTER: return "Master";
        case ServiceType::LINK: return "Link";
        case ServiceType::MACHINE: return "Machine";
        case ServiceType::SWITCH: return "Switch";
        case ServiceType::VMM: return "VMM";
        case ServiceType::VIRTUAL_MACHINE: return "Virtual Machine";
        default:
            return nullptr;
    }
  } else {
    switch (type) {
        case ServiceType::MASTER: return "master";
        case ServiceType::LINK: return "link";
        case ServiceType::MACHINE: return "machine";
        case ServiceType::SWITCH: return "switch";
        case ServiceType::VMM: return "vmm";
        case ServiceType::VIRTUAL_MACHINE: return "virtual machine";

        default:
            return nullptr;
    }
  }
}

} // namespace ispd::services
#endif // ISPD_SERVICES_HPP
