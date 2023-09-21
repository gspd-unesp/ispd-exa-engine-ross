#pragma once

namespace ispd::configuration {

/// \class MachineConfiguration
///
/// \brief Represents the configuration of a machine in the simulation.
///
/// The MachineConfiguration class encapsulates various parameters related to
/// the configuration of a machine in the simulation environment. It provides
/// methods to calculate processing time and obtain power-related information
/// based on the specified parameters.
class MachineConfiguration final {
private:
  double m_PowerPerCore; ///< Computational power per core (in megaflops).
  double m_Load;         ///< Load factor of the machine (0.0 to 1.0).
  unsigned m_CoreCount;  ///< Number of cores in the machine.

  double m_availableMemory; ///< available memory (in GB)
  double m_availableDiskSpace; ///< available disk space (in GB)

  double m_GpuPowerPerCore;
  unsigned m_GpuCoreCount;        ///< Number of GPU cores in the machine.
  double m_InterconnectBandwidth; ///< Total interconnection bandwidth (in
                                  ///< gigatransfer per second).

  double m_WattageIdle; ///< Power consumption (in watts) being idle.
  double
      m_WattageMax; ///< Power consumpttion (in watts) at maximum utilization.
  double m_WattagePerCore; ///< Average power consumption (in watts) per core.
public:
  /// \brief Constructor for MachineConfiguration.
  ///
  /// Initializes a new instance of the MachineConfiguration class with the
  /// provided parameters.
  ///
  /// \param power Computational power of the machine (in megaflops).
  /// \param load Load factor of the machine (0.0 to 1.0).
  /// \param coreCount Number of cores in the machine.
  [[nodiscard]] constexpr explicit MachineConfiguration(
      const double power, const double load, const unsigned coreCount,
      const double availableMemory, const double availableDiskSpace,
      const double gpuPower, const unsigned gpuCoreCount,
      const double interconnectionBandwidth, const double wattageIdle,
      const double wattageMax) noexcept
      : m_PowerPerCore(power / coreCount), m_Load(load), m_CoreCount(coreCount),
        m_availableMemory(availableMemory), m_availableDiskSpace(availableDiskSpace),
        m_InterconnectBandwidth(interconnectionBandwidth),
        m_GpuPowerPerCore(gpuPower / gpuCoreCount), m_GpuCoreCount(gpuCoreCount),
        m_WattageIdle(wattageIdle), m_WattageMax(wattageMax),
        m_WattagePerCore((wattageMax - wattageIdle) / coreCount) {}

  /// \brief Calculates the time required to process a task.
  ///
  /// Calculates and returns the time required to process a workload of the
  /// given size based on the machine's load and computational power per core.
  ///
  /// \param processingSize Processing size of the task to be processed (in
  ///                       megaflop).
  ///
  /// \return Time required to process the task (in seconds).
  [[nodiscard]] inline double
  timeToProcess(const double processingSize, const double communicationSize,
                const double computingOffload) const noexcept {
    /// Caclulates the offloaded computatioanl size to the GPU and the remaining
    /// computtational size that will be processed by the CPU.
    const double offloadProcSize = computingOffload * processingSize;
    const double nonOffloadedProcSize =
        (1.0 - computingOffload) * processingSize;

#define GT_TO_GBITS (10.0)
#define GBITS_TO_MBITS (1000.0)

    /// Calculates the offloaded communication size to the GPU and the time
    /// taken to offload the copmputation (in seconds).
    const double offloadCommSize = computingOffload * communicationSize;
    const double offloadCommTime =
        offloadCommSize /
        (m_InterconnectBandwidth * GT_TO_GBITS * GBITS_TO_MBITS);

#undef GT_TO_GBITS
#undef GBITS_TO_MBITS

    /// Calculates the time taken (in seconds) to process the non-offloaded
    /// computational size and the time taken (in seconds) to process the
    /// offloaded computational size.
    const double nonOffloadProcTime =
        nonOffloadedProcSize / ((1.0 - m_Load) * m_PowerPerCore);
    const double offloadProcTime = offloadProcSize / m_GpuPowerPerCore;

    return nonOffloadProcTime + offloadCommTime + offloadProcTime;
  }

  /// \brief Returns the total computational power (in megaflops) of the
  ///        machine.
  ///
  /// \return Total computational power of the machine (in megaflops).
  [[nodiscard]] inline double getPower() const noexcept {
    return m_PowerPerCore * m_CoreCount;
  }

  /// \brief Returns the computational power per core of the machine.
  ///
  /// \return Computational power per core of the machine (in megaflops).
  [[nodiscard]] inline double getPowerPerCore() const noexcept {
    return m_PowerPerCore;
  }

  /// \brief Returns the load  factor of the machine.
  ///
  /// \return Load factor of the machine (0.0 to 1.0).
  [[nodiscard]] inline double getLoad() const noexcept { return m_Load; }

  /// \brief Returns the amount of CPU cores in this machine.
  ///
  /// \return The core count of the machine.
  [[nodiscard]] inline unsigned getCoreCount() const noexcept {
    return m_CoreCount;
  }

  /// \brief Changes the amount of CPU cores available to use in this machine.
  ///
  /// The number of cores is changed when a new virtual machine is hosted in this physical machine.
  /// \brief Returns the available memory in this machine.
  inline void setCoreCount(unsigned coreCount){
    m_CoreCount = coreCount;
  }


  /// \return The available memory of the machine
  [[nodiscard]] inline double getAvailableMemory() const noexcept{
    return m_availableMemory;
  }


  /// \brief Changes the available memory in this machine.
  ///
  /// The available memory  is changed when a new virtual machine is hosted in this physical machine.

  inline void setAvailableMemory(double availableMemory)
  {
    m_availableMemory = availableMemory;
  }

  /// \brief Returns the available disk space in this machine.
  ///
  /// \return The available memory of the machine
  [[nodiscard]] inline double getAvailableDiskSpace() const noexcept{
    return m_availableDiskSpace;
  }

  /// \brief Changes the available disk space in this machine.
  ///
  /// The available disk space is changed when a new virtual machine is hosted in this physical machine.
  inline void setAvailableDiskSpace(double diskSpace)
  {
    m_availableDiskSpace = diskSpace;
  }


  /// \brief Returns the total computational power supplied by the GPU (in
  ///        megaflops).
  ///
  /// \return The total computational power supplied by the GPU (in megaflops).
  [[nodiscard]] inline unsigned getGpuPower() const noexcept {
    return m_GpuPowerPerCore * m_GpuCoreCount;
  }

  /// \brief Returns the amount of GPU cores in this machine.
  ///
  /// \return The GPU core count of the machine.
  [[nodiscard]] inline unsigned getGpuCoreCount() const noexcept {
    return m_GpuCoreCount;
  }

  /// \brief Retrieves the idle power consumption of the machine.
  ///
  /// This function returns the power consumption of the machine when it is
  /// idle, measured in watts (W). Idle power consumption refers to the amount
  /// of power consumed by the machine's components when they are not actively
  /// processing tasks or performing any computational work. It provides insight
  /// into the baseline power usage of the machine under minimal or no load
  /// conditions.
  ///
  /// \return Idle power consumption of the machine in watts (W).
  [[nodiscard]] inline double getWattageIdle() const noexcept {
    return m_WattageIdle;
  }

  /// \brief Retrieves the maximum power consumption of the machine.
  ///
  /// This function returns the maximum power consumption of the machine,
  /// measured in watts (W). The maximum power consumption represents the
  /// highest amount of power that the machine's components can consume when
  /// running at full capacity or under heavy load. Understanding the maximum
  /// power consumption is crucial for estimating power supply requirements and
  /// managing energy usage in resource-intensive scenarios.
  ///
  /// \return Maximum power consumption of the machine in watts (W).
  [[nodiscard]] inline double getWattageMax() const noexcept {
    return m_WattageMax;
  }

  /// \brief Retrieves the power consumption per core of the machine.
  ///
  /// This function returns the power consumption per core of the machine,
  /// measured in watts (W). It indicates the amount of power consumed by each
  /// individual processing core or unit within the machine. The power
  /// consumption per core is a valuable metric for optimizing power efficiency
  /// and workload distribution, as it helps in balancing computational tasks
  /// across cores to achieve better energy utilization.
  ///
  /// \return Power consumption per core of the machine in watts (W).
  [[nodiscard]] inline double getWattagePerCore() const noexcept {
    return m_WattagePerCore;
  }
};

} // namespace ispd::configuration
