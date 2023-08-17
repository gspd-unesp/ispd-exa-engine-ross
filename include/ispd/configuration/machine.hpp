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
  double m_AvaliableMemory;
  double m_AvaliableDiskSpace;
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
  explicit MachineConfiguration(const double power, const double load,
                                const unsigned coreCount, const double avaliable_mem,
                                const double avaliable_disk,
                                const double wattageIdle = 0.0,
                                const double wattageMax = 0.0)
      : m_PowerPerCore(power / coreCount), m_Load(load), m_CoreCount(coreCount),
        m_AvaliableMemory(avaliable_mem), m_AvaliableDiskSpace(avaliable_disk),
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
  inline double timeToProcess(const double processingSize) {
    return processingSize / ((1.0 - m_Load) * m_PowerPerCore);
  }

  /// \brief Returns the total computational power (in megaflops) of the
  ///        machine.
  ///
  /// \return Total computational power of the machine (in megaflops).
  inline double getPower() const { return m_PowerPerCore * m_CoreCount; }

  /// \brief Returns the computational power per core of the machine.
  ///
  /// \return Computational power per core of the machine (in megaflops).
  inline double getPowerPerCore() const { return m_PowerPerCore; }

  /// \brief Returns the load factor of the machine.
  ///
  /// \return Load factor of the machine (0.0 to 1.0).
  inline double getLoad() const { return m_Load; }

  inline double getAvaliableMemory() const {return m_AvaliableMemory;}

  inline double getAvaliableDisk() const {return m_AvaliableDiskSpace;}

  inline double getNumCores() const {return m_CoreCount;}

  inline void setAvaliableMemory(double new_memory) {m_AvaliableMemory = new_memory;}
  inline void setAvaliableDisk (double new_disk) {m_AvaliableDiskSpace = new_disk;}
  inline void setNumCores (unsigned new_cores) {m_CoreCount = new_cores;}

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
  inline double getWattageIdle() const { return m_WattageIdle; }

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
  inline double getWattageMax() const { return m_WattageMax; }

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
  inline double getWattagePerCore() const { return m_WattagePerCore; }
};

} // namespace ispd::configuration

