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
                                const unsigned coreCount,
                                const double wattageIdle = 0.0,
                                const double wattageMax = 0.0)
      : m_PowerPerCore(power / coreCount), m_Load(load), m_CoreCount(coreCount),
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
};

} // namespace ispd::configuration

