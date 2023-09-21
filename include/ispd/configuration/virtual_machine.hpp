#pragma once

namespace ispd::configuration {
/// \class VmConfiguration
///
/// \brief Represents the configuration of a virtual machine in the simulation
///
/// The VmConfiguration encapsulates various parameters related to the configuration of a virtual machine in the simulation environment and provides methods for obtaining information.

class VmConfiguration final {

private:
  double m_PowerPerCore; ///< computational power in megaflops.
  double m_Load;         ///< load factor of the virtual machine  (0.0 to 1.0).
  unsigned m_CoreCount;  ///< Number of cores in this virtual machine.

  double m_Memory;    ///< Memory of this virtual machine (GB).
  double m_DiskSpace; ///< Disk space of this virtual machine (GB).

public:
  /// \brief Constructor for VmConfiguration
  ///
  /// Initializes a new instance of VmConfiguration class with the provided parameters. \param power \param load \param coreCount \param memory \param diskSpace

  [[nodiscard]] constexpr explicit VmConfiguration(
      const double power, const double load, const unsigned coreCount,
      const double memory, const double diskSpace) noexcept
      : m_PowerPerCore(power / coreCount), m_Load(load), m_CoreCount(coreCount),
        m_Memory(memory), m_DiskSpace(diskSpace) {}

  /// \brief Calculates and returns the time to process given a processingSize
  ///
  /// Calculates and returns the time required to process a given processing size.
  ///
  /// \param processingSize The size of the processing to be carried out.
  /// \return The time required to process the given size.
  [[nodicard]] inline double timeToProcess(const double processingSize) {
    return processingSize / (1.0 - m_Load) * m_PowerPerCore;
  }

  /// \brief Returns the total computational power (in megaflops)
  /// of the machine.
  ///
  /// \return Total computational power of the machine (in megaflops).
  [[nodiscard]] inline double getPower() const noexcept {
    return m_PowerPerCore * m_CoreCount;
  }

  /// \brief Returns the load factor of the machine.
  /// \return Load Factor of the machine (0.0 to 1.0).
  [[nodiscard]] inline double getLoad() const noexcept { return m_Load; }

  ///  \brief Returns the amount of CPU cores in this machine.
  /// \return amount of cpu cores.
  [[nodiscard]] inline unsigned getCoreCount() const noexcept {
    return m_CoreCount;
  }


  /// \brief Returns the amount of memory in this machine.
  /// return amount of memory in GB.
  [[nodiscard]] inline double getMemory() const noexcept {
    return m_Memory;
  }


  /// \brief Returns the amount of disk space in this machine.
  /// return amount of disk space in GB.
  [[nodiscard]] inline double getDiskSpace() const noexcept{
    return m_DiskSpace;
  }



};
};
