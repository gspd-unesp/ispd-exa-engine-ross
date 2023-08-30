#pragma once

namespace ispd::configuration{

/**
 * \class VmConfiguration
 *
 * \brief Represents the configuration of a virtual machine in the simulation
 *
 * Encapsulates various parameters related to the virtual machine configuration
 * in the simulation and provides methods for calculating the process time and
 * obtain more information.
 */

class VmConfiguration final{
private:
  double m_PowerPerCore; ///< computational power in megaflops
  double m_Load;         ///< load factor, between 0 and 1
  unsigned m_CoreCount;
  double m_AvaliableMemory;
  double m_AvaliableStorage;

public:
  /**
   * \brief Constructor for virtual machine class.
   * @param power
   * @param load
   * @param coreCount
   * @param avaliableMemory
   * @param avaliableStorage
   */
  explicit VmConfiguration(const double power, const double load, const double
                           coreCount, const double avaliableMemory,
                           const double avaliableStorage):
  m_PowerPerCore(power/coreCount), m_Load(load), m_CoreCount(coreCount),
  m_AvaliableMemory(avaliableMemory), m_AvaliableStorage(avaliableStorage){}

  /**
 * @brief Calculates and returns the time to process a given size of processing
 *
 * This function calculates the time to process a given size using the formula:
 * processingSize/((1.0 - m_Load) * m_PowerPerCore)
 *
 * @param processingSize The size of the processing to be carried out.
 * @return The time required to process the given size.
   */
  inline double timeToProcess(const double processingSize){
    return processingSize/((1.0 - m_Load) * m_PowerPerCore);
  }

  // Getter for m_PowerPerCore
  double GetPowerPerCore() const {
    return m_PowerPerCore;
  }

  // Getter for m_Load
  double GetLoad() const {
    return m_Load;
  }

  // Getter for m_CoreCount
  unsigned GetCoreCount() const {
    return m_CoreCount;
  }

  // Getter for m_AvaliableMemory
  double GetAvaliableMemory() const {
    return m_AvaliableMemory;
  }

  // Getter for m_AvaliableStorage
  double GetAvaliableStorage() const {
    return m_AvaliableStorage;
  }




};
};