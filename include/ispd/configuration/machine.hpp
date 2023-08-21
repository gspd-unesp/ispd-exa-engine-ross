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
  [[nodiscard]] constexpr explicit MachineConfiguration(
      const double power, const double load, const unsigned coreCount,
      const double wattageIdle = 0.0, const double wattageMax = 0.0) noexcept
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
  [[nodiscard]] inline double
  timeToProcess(const double processingSize) const noexcept {
    return processingSize / ((1.0 - m_Load) * m_PowerPerCore);
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

  /// \brief Returns the load factor of the machine.
  ///
  /// \return Load factor of the machine (0.0 to 1.0).
  [[nodiscard]] inline double getLoad() const noexcept { return m_Load; }

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

