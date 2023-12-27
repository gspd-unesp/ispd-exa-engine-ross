#include <mpi.h>
#include <ross.h>
#include <string>
#include <fstream>
#include <algorithm>
#include <ispd/log/log.hpp>
#include <lib/nlohmann/json.hpp>
#include <ispd/model/builder.hpp>
#include <ispd/metrics/metrics.hpp>
#include <ispd/metrics/machine_metrics.hpp>

namespace ispd::metrics {

void NodeMetricsCollector::reportNodeUserMetrics() {
  /// An alias for the global metrics collector.
  auto gmc = ispd::global_metrics::g_GlobalMetricsCollector;

  /// Fetch the mapping containing all registered users in the system being simulated.
  const auto& registeredUsers = ispd::this_model::getUsers();

  /// Report each user metrics.
  for (const auto& [id, user] : registeredUsers) {
    const auto& metrics = user.getMetrics();

    #define REDUCE_USER_METRIC(op, type, field, fieldName) \
      if (MPI_SUCCESS != MPI_Reduce(&metrics.field, &gmc->m_GlobalUserMetrics[id].field, 1, type, op, 0, MPI_COMM_ROSS)) \
          ispd_error("User (%u, %s) %s could not be reduced, exiting...", id, user.getName().c_str(), fieldName);

    REDUCE_USER_METRIC(MPI_SUM, MPI_DOUBLE, m_ProcTime, "processing time");
    REDUCE_USER_METRIC(MPI_SUM, MPI_DOUBLE, m_ProcWaitingTime, "processing waiting time");
    REDUCE_USER_METRIC(MPI_SUM, MPI_DOUBLE, m_CommTime, "communication time");
    REDUCE_USER_METRIC(MPI_SUM, MPI_DOUBLE, m_CommWaitingTime, "communication waiting time");
    REDUCE_USER_METRIC(MPI_SUM, MPI_DOUBLE, m_EnergyConsumption, "energy consumption");
    REDUCE_USER_METRIC(MPI_SUM, MPI_UNSIGNED, m_IssuedTasks, "issued tasks");
    REDUCE_USER_METRIC(MPI_SUM, MPI_UNSIGNED, m_CompletedTasks, "completed tasks");

    #undef REDUCE_USER_METRIC
  }
}

template <>
void NodeMetricsCollector::notifyMetric(const NodeMetricsFlag flag, const double value) {
  switch (flag) {
    case NodeMetricsFlag::NODE_TOTAL_PROCESSED_MFLOPS:
      /// Updates total processed MFLOPS.
      m_NodeTotalProcessedMFlops += value;
      break;
    case NodeMetricsFlag::NODE_TOTAL_COMMUNICATED_MBITS:
      /// Updates total communicated MBits.
      m_NodeTotalCommunicatedMBits += value;
      break;
    case NodeMetricsFlag::NODE_TOTAL_PROCESSING_WAITING_TIME:
      /// Updates total processing waiting time.
      m_NodeTotalProcessingWaitingTime += value;
      break;
    case NodeMetricsFlag::NODE_TOTAL_COMMUNICATION_WAITING_TIME:
      /// Updates total communication waiting time.
      m_NodeTotalCommunicationWaitingTime += value;
      break;
    case NodeMetricsFlag::NODE_TOTAL_COMPUTATIONAL_POWER:
      /// Updates total computational power.
      m_NodeTotalComputationalPower += value;
    case NodeMetricsFlag::NODE_TOTAL_PROCESSING_TIME:
      /// Updates total processing time.
      m_NodeTotalProcessingTime += value;
      break;
     case NodeMetricsFlag::NODE_TOTAL_COMMUNICATION_TIME:
      /// Updates total communication time.
      m_NodeTotalCommunicationTime += value;
      break;
    case NodeMetricsFlag::NODE_TOTAL_TURNAROUND_TIME:
      /// Updates total turnaround time.
      m_NodeTotalTurnaroundTime += value;
      break;
    case NodeMetricsFlag::NODE_TOTAL_NON_IDLE_ENERGY_CONSUMPTION:
      /// Updates the total energy consumption.
      m_NodeTotalNonIdleEnergyConsumption += value;
      break;
    case NodeMetricsFlag::NODE_TOTAL_POWER_IDLE:
      /// Updates the total power idle.
      m_NodeTotalPowerIdle += value;
      break;
    case NodeMetricsFlag::NODE_SIMULATION_TIME:
      /// Updates simulation time.
      m_NodeSimulationTime = std::max(m_NodeSimulationTime, value);
      break;
#ifdef DEBUG_ON
    case NodeMetricsFlag::NODE_MASTER_FORWARD_TIME:
      /// Updates the total forward time and forward events count by the master.
      m_NodeTotalForwardTime[ispd::services::ServiceType::MASTER] += value;
      m_NodeTotalForwardEventsCount[ispd::services::ServiceType::MASTER]++;
      break;
    case NodeMetricsFlag::NODE_MASTER_REVERSE_TIME:
      /// Updates the total reverse time and reverse events count by the master.
      m_NodeTotalReverseTime[ispd::services::ServiceType::MASTER] += value;
      m_NodeTotalReverseEventsCount[ispd::services::ServiceType::MASTER]++;
      break;
    case NodeMetricsFlag::NODE_LINK_FORWARD_TIME:
      /// Updates the total forward time and forward events count by the link.
      m_NodeTotalForwardTime[ispd::services::ServiceType::LINK] += value;
      m_NodeTotalForwardEventsCount[ispd::services::ServiceType::LINK]++;
      break;
    case NodeMetricsFlag::NODE_LINK_REVERSE_TIME:
      /// Updates the total reverse time and reverse events count by the link.
      m_NodeTotalReverseTime[ispd::services::ServiceType::LINK] += value;
      m_NodeTotalReverseEventsCount[ispd::services::ServiceType::LINK]++;
      break;
    case NodeMetricsFlag::NODE_MACHINE_FORWARD_TIME:
      /// Updates the total forward time and forward events count by the machine.
      m_NodeTotalForwardTime[ispd::services::ServiceType::MACHINE] += value;
      m_NodeTotalForwardEventsCount[ispd::services::ServiceType::MACHINE]++;
      break;
    case NodeMetricsFlag::NODE_MACHINE_REVERSE_TIME:
      /// Updates the total reverse time and reverse events count by the machine.
      m_NodeTotalReverseTime[ispd::services::ServiceType::MACHINE] += value;
      m_NodeTotalReverseEventsCount[ispd::services::ServiceType::MACHINE]++;
      break;
    case NodeMetricsFlag::NODE_SWITCH_FORWARD_TIME:
      /// Updates the total forward time and forward events count by the switch.
      m_NodeTotalForwardTime[ispd::services::ServiceType::SWITCH] += value;
      m_NodeTotalForwardEventsCount[ispd::services::ServiceType::SWITCH]++;
      break;
    case NodeMetricsFlag::NODE_SWITCH_REVERSE_TIME:
      /// Updates the total reverse time and reverse events count by the master.
      m_NodeTotalReverseTime[ispd::services::ServiceType::SWITCH] += value;
      m_NodeTotalReverseEventsCount[ispd::services::ServiceType::SWITCH]++;
      break;
#endif // DEBUG_ON
    default:
      ispd_error("Unknown node metrics flag (%d) or incorrect argument type.", flag);
  }
}

template <>
void NodeMetricsCollector::notifyMetric(const enum NodeMetricsFlag flag, const unsigned value) {
  switch (flag) {
    case NodeMetricsFlag::NODE_TOTAL_COMPLETED_TASKS:
      /// Updates total completed tasks.
      m_NodeTotalCompletedTasks += value;
      break;
    case NodeMetricsFlag::NODE_TOTAL_CPU_CORES:
      /// Updates the total CPU cores.
      m_NodeTotalCpuCores += value;
      break;
    case NodeMetricsFlag::NODE_TOTAL_GPU_CORES:
      /// Updates the total GPU cores.
      m_NodeTotalGpuCores += value;
      break;
    default:
      ispd_error("Unknown node metrics flag (%d) or it may be the case the flag is correct but the argument is not of the required type.", flag);
  }
}

void NodeMetricsCollector::notifyMetric(const enum NodeMetricsFlag flag) {
  switch (flag) {
     case NodeMetricsFlag::NODE_TOTAL_MASTER_SERVICES:
      /// Updates the count of master services.
      m_NodeTotalMasterServices++;
      break;
     case NodeMetricsFlag::NODE_TOTAL_LINK_SERVICES:
      /// Updates the count of link services.
      m_NodeTotalLinkServices++;
      break;
     case NodeMetricsFlag::NODE_TOTAL_MACHINE_SERVICES:
      /// Updates the count of machine services.
      m_NodeTotalMachineServices++;
      break;
     case NodeMetricsFlag::NODE_TOTAL_SWITCH_SERVICES:
      /// Updates the count of switch services.
      m_NodeTotalSwitchServices++;
      break;
    default:
      ispd_error("Unknown node metrics flag (%d) or it may be the case the flag is correct but the argument is not of the required type.", flag);
    }
}

void NodeMetricsCollector::reportNodeMetrics() {
  /// An alias for the global metrics collector.
  auto gmc = ispd::global_metrics::g_GlobalMetricsCollector;

  /// Report to the master node the simulation time of this node.
  if (MPI_SUCCESS != MPI_Reduce(&m_NodeSimulationTime, &gmc->m_GlobalSimulationTime, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_ROSS))
    ispd_error("Global simulation time could not be reduced, exiting...");

  /// Report to the master node the processed MFlops of this node.
  if (MPI_SUCCESS != MPI_Reduce(&m_NodeTotalProcessedMFlops, &gmc->m_GlobalTotalProcessedMFlops, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_ROSS))
    ispd_error("Global total processed mflops could not be reduced, exiting...");

  /// Report to the master node the processed MFlops of this node.
  if (MPI_SUCCESS != MPI_Reduce(&m_NodeTotalCommunicatedMBits, &gmc->m_GlobalTotalCommunicatedMBits, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_ROSS))
    ispd_error("Global total communicated mbits could not be reduced, exiting...");

  /// Report to the master node the total processing waiting time.
  if (MPI_SUCCESS != MPI_Reduce(&m_NodeTotalProcessingWaitingTime, &gmc->m_GlobalTotalProcessingWaitingTime, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_ROSS))
    ispd_error("Global total processing waiting time could not be reduced, exiting...");

  /// Report to the master node the total communication waiting time.
  if (MPI_SUCCESS != MPI_Reduce(&m_NodeTotalCommunicationWaitingTime, &gmc->m_GlobalTotalCommunicationWaitingTime, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_ROSS))
    ispd_error("Global total communication waiting time could not be reduced, exiting...");

  /// Report to the master node the total master services count.
  if (MPI_SUCCESS != MPI_Reduce(&m_NodeTotalMasterServices, &gmc->m_GlobalTotalMasterServices, 1, MPI_UNSIGNED, MPI_SUM, 0, MPI_COMM_ROSS))
    ispd_error("Global total master services could not be reduced, exiting...");
 
  /// Report to the master node the total link services count.
  if (MPI_SUCCESS != MPI_Reduce(&m_NodeTotalLinkServices, &gmc->m_GlobalTotalLinkServices, 1, MPI_UNSIGNED, MPI_SUM, 0, MPI_COMM_ROSS))
    ispd_error("Global total link services could not be reduced, exiting...");
 
  /// Report to the master node the total machine services count.
  if (MPI_SUCCESS != MPI_Reduce(&m_NodeTotalMachineServices, &gmc->m_GlobalTotalMachineServices, 1, MPI_UNSIGNED, MPI_SUM, 0, MPI_COMM_ROSS))
    ispd_error("Global total machine services could not be reduced, exiting...");
 
  /// Report to the master node the total switch services count.
  if (MPI_SUCCESS != MPI_Reduce(&m_NodeTotalSwitchServices, &gmc->m_GlobalTotalSwitchServices, 1, MPI_UNSIGNED, MPI_SUM, 0, MPI_COMM_ROSS))
    ispd_error("Global total switch services could not be reduced, exiting...");
 
  /// Report to the master node the total completed tasks.
  if (MPI_SUCCESS != MPI_Reduce(&m_NodeTotalCompletedTasks, &gmc->m_GlobalTotalCompletedTasks, 1, MPI_UNSIGNED, MPI_SUM, 0, MPI_COMM_ROSS))
    ispd_error("Global total completed tasks could not be reduced, exiting...");

  /// Report to the master node the total computational power.
  if (MPI_SUCCESS != MPI_Reduce(&m_NodeTotalComputationalPower, &gmc->m_GlobalTotalComputationalPower, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_ROSS))
    ispd_error("Global total computational power could not be reduced, exiting...");

  /// Report to the master node the total CPU cores.
  if (MPI_SUCCESS != MPI_Reduce(&m_NodeTotalCpuCores, &gmc->m_GlobalTotalCpuCores, 1, MPI_UNSIGNED, MPI_SUM, 0, MPI_COMM_ROSS))
    ispd_error("Global total cpu cores could not be reduced, exiting...");

  /// Report to the master node the total GPU cores.
  if (MPI_SUCCESS != MPI_Reduce(&m_NodeTotalGpuCores, &gmc->m_GlobalTotalGpuCores, 1, MPI_UNSIGNED, MPI_SUM, 0, MPI_COMM_ROSS))
    ispd_error("Global total gpu cores could not be reduced, exiting...");


  /// Report to the master node the processing time.
  if (MPI_SUCCESS != MPI_Reduce(&m_NodeTotalProcessingTime, &gmc->m_GlobalTotalProcessingTime, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_ROSS))
    ispd_error("Global total processing time could not be reduced, exiting...");

  /// Report to the master node the communication size.
  if (MPI_SUCCESS != MPI_Reduce(&m_NodeTotalCommunicationTime, &gmc->m_GlobalTotalCommunicationTime, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_ROSS))
    ispd_error("Global total communication time could not be reduced, exiting...");

  /// Report to the master node the turnaround time.
  if (MPI_SUCCESS != MPI_Reduce(&m_NodeTotalTurnaroundTime, &gmc->m_GlobalTotalTurnaroundTime, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_ROSS))
    ispd_error("Global total turnaround time could not be reduced, exiting...");
  
  /// Report to the master node the energy consumption.
  if (MPI_SUCCESS != MPI_Reduce(&m_NodeTotalNonIdleEnergyConsumption, &gmc->m_GlobalTotalNonIdleEnergyConsumption, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_ROSS))
    ispd_error("Global total energy consumption could not be reduced, exiting...");

  /// Report to the master node the power idle.
  if (MPI_SUCCESS != MPI_Reduce(&m_NodeTotalPowerIdle, &gmc->m_GlobalTotalPowerIdle, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_ROSS))
    ispd_error("Global total power idle could not be reduced, exiting...");

#ifdef DEBUG_ON
  for (const auto& serviceType : ispd::services::g_ServiceTypes) {
    /// Report to the master node the forward processing time.
    if (MPI_SUCCESS != MPI_Reduce(&m_NodeTotalForwardTime[serviceType],
                                  &gmc->m_GlobalTotalForwardTime[serviceType],
                                  1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_ROSS))
      ispd_error("Global total %s forward time could not be reduced, exiting...",
         ispd::services::getServiceTypeName(serviceType));

    /// Report to the master node the forward events count.
    if (MPI_SUCCESS != MPI_Reduce(&m_NodeTotalForwardEventsCount[serviceType],
                                  &gmc->m_GlobalTotalForwardEventsCount[serviceType],
                                  1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_ROSS))
      ispd_error("Global total %s forward events count could not be reduced, exiting...",
          ispd::services::getServiceTypeName(serviceType));

    /// Report to the master node the reverse processing time.
    if (MPI_SUCCESS != MPI_Reduce(&m_NodeTotalReverseTime[serviceType],
                                  &gmc->m_GlobalTotalReverseTime[serviceType],
                                  1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_ROSS))
      ispd_error("Global total %s reverse time could not be reduced, exiting...",
          ispd::services::getServiceTypeName(serviceType));

    /// Report to the master node the reverse events count.
    if (MPI_SUCCESS != MPI_Reduce(&m_NodeTotalReverseEventsCount[serviceType],
                                  &gmc->m_GlobalTotalReverseEventsCount[serviceType],
                                  1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_ROSS))
      ispd_error("Global total %s reverse events count could not be reduced, exiting...",
          ispd::services::getServiceTypeName(serviceType));
  }
#endif // DEBUG_ON

  reportNodeUserMetrics();
}

void GlobalMetricsCollector::reportGlobalMetrics() {
  /// Check if the current node is not the master one. If so, the global metrics will
  /// not be reported, since only the master node will report the global metrics.
  if (g_tw_mynode)
    return;

  const double avgProcessingTime = m_GlobalTotalProcessingTime / m_GlobalTotalCompletedTasks;
  const double avgProcessingWaitingTime = m_GlobalTotalProcessingWaitingTime / m_GlobalTotalCompletedTasks;
  const double avgCommunicationTime = m_GlobalTotalCommunicationTime / m_GlobalTotalCompletedTasks;
  const double avgCommunicationWaitingTime = m_GlobalTotalCommunicationWaitingTime / m_GlobalTotalCompletedTasks;
  const double avgTotalTurnaroundTime = m_GlobalTotalTurnaroundTime / m_GlobalTotalCompletedTasks;
  const double maxComputationalPower = m_GlobalTotalProcessedMFlops / m_GlobalSimulationTime;

  /// The efficiency is calculated as: Rmax / Rpeak
  const double efficiency = maxComputationalPower / m_GlobalTotalComputationalPower;

  /// The total energy consumption is divided into two main components: dynamic (D) and
  /// static (S) energy consumption. The dynamic energy consumption refers to the energy
  /// consumed while the component is actively processing tasks or performing operations.
  /// In contrast, the static energy consumption corresponds to the energy consumed when
  /// the component is in an idle or low-power state.
  ///
  /// \note The dynamic energy consumption is typically influenced by factors such as
  /// processing load, activity patterns, and utilization. The static energy consumption
  /// often includes power used by components in standby, sleep, or other low-power modes.
  const double totalEnergyConsumption = m_GlobalTotalNonIdleEnergyConsumption +
    m_GlobalTotalPowerIdle * m_GlobalSimulationTime;

  /// Calculates the system average power and the system's energy efficiency.
  const double avgPower = totalEnergyConsumption / m_GlobalSimulationTime;
  const double energyEfficiency = maxComputationalPower / avgPower;

  ispd_info("");
  ispd_info("Global Simulation Time...........: %lf seconds.", m_GlobalSimulationTime);
  ispd_info("");
  ispd_info("Total Metrics");
  ispd_info(" Total Processed MFLOPS..........: %lf MFLOPS.", m_GlobalTotalProcessedMFlops);
  ispd_info(" Total Communicated MBits........: %lf MBits.", m_GlobalTotalCommunicatedMBits);
  ispd_info(" Total Processing Waiting Time...: %lf seconds.", m_GlobalTotalProcessingWaitingTime);
  ispd_info(" Total Communication Waiting Time: %lf seconds.", m_GlobalTotalCommunicationWaitingTime);
  ispd_info(" Total Master Services...........: %u services.", m_GlobalTotalMasterServices);
  ispd_info(" Total Link Services.............: %u services.", m_GlobalTotalLinkServices);
  ispd_info(" Total Machine Services..........: %u services.", m_GlobalTotalMachineServices);
  ispd_info(" Total Switch Services...........: %u services.", m_GlobalTotalSwitchServices);
  ispd_info(" Total Completed Tasks...........: %u tasks.", m_GlobalTotalCompletedTasks);
  ispd_info("");
  ispd_info("Average Metrics");
  ispd_info(" Avg. Processing Time............: %lf seconds.", avgProcessingTime);
  ispd_info(" Avg. Processing Waiting Time....: %lf seconds.", avgProcessingWaitingTime);
  ispd_info(" Avg. Communication Time.........: %lf seconds.", avgCommunicationTime);
  ispd_info(" Avg. Communication Waiting Time.: %lf seconds.", avgCommunicationWaitingTime);
  ispd_info(" Avg. Turnaround Time............: %lf seconds.", avgTotalTurnaroundTime);
  ispd_info("");
  ispd_info("System Metrics");
  ispd_info("");
  ispd_info(" Processing-related metrics");
  ispd_info("  Peak Computational Power........: %lf MFLOPS.", m_GlobalTotalComputationalPower);
  ispd_info("  Max. Computational Power........: %lf MFLOPS.", maxComputationalPower);
  ispd_info("  Efficiency......................: %lf%%.", efficiency * 100.0);
  ispd_info("");
  ispd_info(" Energy-related metrics");
  ispd_info("  Energy Consumption..............: %lf J.", totalEnergyConsumption);
  ispd_info("  Energy Efficiency...............: %lf MFLOPS/W.", energyEfficiency);
  ispd_info("  Avg. Power......................: %lf W.", avgPower);
  ispd_info("  Idle Power......................: %lf W.", m_GlobalTotalPowerIdle);
  ispd_info("");
  ispd_info(" Total CPU Cores.................: %u cores.", m_GlobalTotalCpuCores);
  ispd_info(" Total GPU Cores.................: %u cores.", m_GlobalTotalGpuCores);
  ispd_info("");
  ispd_info("User Metrics");
  
  for (const auto& [id, userMetrics] : m_GlobalUserMetrics) {
    const double userAvgProcTime = userMetrics.m_ProcTime / userMetrics.m_IssuedTasks;
    const double userAvgProcWaitingTime = userMetrics.m_ProcWaitingTime / userMetrics.m_IssuedTasks;
    const double userAvgCommTime = userMetrics.m_CommTime / userMetrics.m_IssuedTasks;
    const double userAvgCommWaitingTime = userMetrics.m_CommWaitingTime / userMetrics.m_IssuedTasks;


    ispd_info("");
    ispd_info(" %s", ispd::this_model::getUserById(id).getName().c_str());
    ispd_info("  Avg. Processing Time...........: %lf seconds.", userAvgProcTime);
    ispd_info("  Avg. Processing Waiting Time...: %lf seconds.", userAvgProcWaitingTime);
    ispd_info("  Avg. Communication Time........: %lf seconds.", userAvgCommTime);
    ispd_info("  Avg. Communication Waiting Time: %lf seconds.", userAvgCommWaitingTime);
    ispd_info("  Issued Tasks...................: %u tasks.", userMetrics.m_IssuedTasks);
    ispd_info("  Completed Tasks................: %u tasks.", userMetrics.m_CompletedTasks);
    ispd_info("  Energy Consumption.............: %lf J.", userMetrics.m_EnergyConsumption);
  }

  ispd_info("");
#ifdef DEBUG_ON
  ispd_info("Service Center Metrics");
  
  /// Calculate the average time taken for forward and reverse processing and, print the information
  /// about the average time of forward and reverse processing as well as the forward and reverse event count.
  for (const auto& serviceType : ispd::services::g_ServiceTypes) {
    /// Fetches the forward and reverse processing time (in nanoseconds) of the service center type.
    const double totalForwardTime = m_GlobalTotalForwardTime[serviceType];
    const double totalReverseTime = m_GlobalTotalReverseTime[serviceType];
    
    /// Fetches the forward and reverse events processed of the service center type.
    const uint64_t forwardEventsCount = m_GlobalTotalForwardEventsCount[serviceType];
    const uint64_t reverseEventsCount = m_GlobalTotalReverseEventsCount[serviceType];   

    /// Calculates the average time taken (in nanoseconds) to forward and reverse processing.
    const double avgForwardTime = totalForwardTime / forwardEventsCount;
    const double avgReverseTime = totalReverseTime / reverseEventsCount;
    
    /// Calculates how many times the reverse processing is faster than the forward processing at average.
    const double forwardAndReverseRelation = avgForwardTime / avgReverseTime;
    
    const char *capitalizedServiceTypeName = ispd::services::getServiceTypeName<true>(serviceType);
    
    ispd_info(" Avg. %s Forward Time........: %lf ns.", capitalizedServiceTypeName, avgForwardTime);
    ispd_info(" Avg. %s Reverse Time........: %lf ns.", capitalizedServiceTypeName, avgReverseTime);
    ispd_info(" %s Forward Events Count.....: %lu events.", capitalizedServiceTypeName, forwardEventsCount);
    ispd_info(" %s Reverse Events Count.....: %lu events.", capitalizedServiceTypeName, reverseEventsCount);
    ispd_info(" Avg. Forward/Reverse........: %lfx.", capitalizedServiceTypeName, forwardAndReverseRelation);
    ispd_info("");   
  }
#endif // DEBUG_ON
}

void GlobalMetricsCollector::reportGlobalMetricsToFile(
    const std::filesystem::path reportFilePath) {
  using json = nlohmann::json;

  json data;
  std::ofstream out(reportFilePath);

  /// Writing the general-related metrics.
  data["simulation_time"] = m_GlobalSimulationTime;

  /// Writing the total-related metrics.
  json total;

  total["processed_mflops"] = m_GlobalTotalProcessedMFlops;
  total["communicated_mbits"] = m_GlobalTotalCommunicatedMBits;
  total["processing_waiting_time"] = m_GlobalTotalProcessingWaitingTime;
  total["master_services"] = m_GlobalTotalMasterServices;
  total["link_serivces"] = m_GlobalTotalLinkServices;
  total["machine_services"] = m_GlobalTotalMachineServices;
  total["switch_services"] = m_GlobalTotalSwitchServices;
  data["total"] = total;

  /// Writing the average-related metrics.
  json average;

  const double avgProcessingTime =
      m_GlobalTotalProcessingTime / m_GlobalTotalCompletedTasks;
  const double avgProcessingWaitingTime =
      m_GlobalTotalProcessingWaitingTime / m_GlobalTotalCompletedTasks;
  const double avgCommunicationTime =
      m_GlobalTotalCommunicationTime / m_GlobalTotalCompletedTasks;
  const double avgCommunicationWaitingTime =
      m_GlobalTotalCommunicationWaitingTime / m_GlobalTotalCompletedTasks;
  const double avgTotalTurnaroundTime =
      m_GlobalTotalTurnaroundTime / m_GlobalTotalCompletedTasks;

  average["processing_time"] = avgProcessingTime;
  average["processing_waiting_time"] = avgProcessingWaitingTime;
  average["communication_time"] = avgCommunicationTime;
  average["communication_waiting_time"] = avgCommunicationWaitingTime;
  average["turnaround_time"] = avgTotalTurnaroundTime;
  data["average"] = average;

  /// Writing the system-related metrics.
  json system;

  /// Maximum computational power obtained in the simulation.
  const double maxComputationalPower =
      m_GlobalTotalProcessedMFlops / m_GlobalSimulationTime;

  {
    /// The efficiency is calculated as: Rmax / Rpeak
    const double efficiency =
        maxComputationalPower / m_GlobalTotalComputationalPower;

    /// Writing the system processing-related metrics.
    json processing;
    processing["peak_computational_power"] = m_GlobalTotalComputationalPower;
    processing["max_computational_power"] = maxComputationalPower;
    processing["efficiency"] = efficiency;
    system["processing"] = processing;
  }

  {
    /// The total energy consumption is divided into two main components:
    /// dynamic (D) and static (S) energy consumption. The dynamic energy
    /// consumption refers to the energy consumed while the component is
    /// actively processing tasks or performing operations. In contrast, the
    /// static energy consumption corresponds to the energy consumed when the
    /// component is in an idle or low-power state.
    ///
    /// \note The dynamic energy consumption is typically influenced by factors
    /// such as processing load, activity patterns, and utilization. The static
    /// energy consumption often includes power used by components in standby,
    /// sleep, or other low-power modes.
    const double totalEnergyConsumption =
        m_GlobalTotalNonIdleEnergyConsumption +
        m_GlobalTotalPowerIdle * m_GlobalSimulationTime;

    /// Calculates the system average power and the system's energy efficiency.
    const double avgPower = totalEnergyConsumption / m_GlobalSimulationTime;
    const double energyEfficiency = maxComputationalPower / avgPower;

    /// Writing the system energy-related metrics.
    json energy;

    energy["energy_consumption"] = totalEnergyConsumption;
    energy["energy_efficiency"] = energyEfficiency;
    energy["average_power"] = avgPower;
    energy["idle_power"] = m_GlobalTotalPowerIdle;
    system["energy"] = energy;
  }

  system["total_cpu_cores"] = m_GlobalTotalCpuCores;
  system["total_gpu_cores"] = m_GlobalTotalGpuCores;
  data["system"] = system;

  /// Writing the user-related metrics.
  json users;

  for (const auto& [id, userMetrics] : m_GlobalUserMetrics) {
    const double userAvgProcTime = userMetrics.m_ProcTime / userMetrics.m_IssuedTasks;
    const double userAvgProcWaitingTime = userMetrics.m_ProcWaitingTime / userMetrics.m_IssuedTasks;
    const double userAvgCommTime = userMetrics.m_CommTime / userMetrics.m_IssuedTasks;
    const double userAvgCommWaitingTime = userMetrics.m_CommWaitingTime / userMetrics.m_IssuedTasks;

    json user;

    user["average_processing_time"] = userAvgProcTime;
    user["average_processing_waiting_time"] = userAvgProcWaitingTime;
    user["average_communication_time"] = userAvgCommTime;
    user["average_communication_waiting_time"] = userAvgCommWaitingTime;
    user["issued_tasks"] = userMetrics.m_IssuedTasks;
    user["completed_tasks"] = userMetrics.m_CompletedTasks;
    user["energy_consumption"] = userMetrics.m_EnergyConsumption;
    
    users[ispd::this_model::getUserById(id).getName()] = user;
  }

  data["users"] = users;

  /// Write the JSON content into the file using the prettified format.
  out << std::setw(2) << data << std::endl;
}

}; // namespace ispd::metrics

namespace ispd::node_metrics {

  ispd::metrics::NodeMetricsCollector *g_NodeMetricsCollector = new ispd::metrics::NodeMetricsCollector();

  nlohmann::json *g_NodeMetricsReport = new nlohmann::json();

  template <>
  void notifyMetric(const enum ispd::metrics::NodeMetricsFlag flag, const double value) {
    /// Forward the notification to the node metrics collector.
    g_NodeMetricsCollector->notifyMetric(flag, value);
  }

  template <>
  void notifyMetric(const enum ispd::metrics::NodeMetricsFlag flag, const unsigned value) {
    /// Forward the notification to the node metrics collector.
    g_NodeMetricsCollector->notifyMetric(flag, value);
  }

  void notifyMetric(const enum ispd::metrics::NodeMetricsFlag flag) {
    /// Forward the notification to the node metrics collector.
    g_NodeMetricsCollector->notifyMetric(flag);
  }

  void notifyReport(const ispd::metrics::MachineMetrics &metrics,
                    const ispd::configuration::MachineConfiguration &configuration,
                    const tw_lpid gid) {
    nlohmann::json report;

    /// Calculates the machine's average processing time, i.e., the average time
    /// spent to process one task.
    const double avgProcTime = metrics.m_ProcTime / metrics.m_ProcTasks;

    /// Calculates the machine's average power consumption.
    const double avgPowerConsumption = metrics.m_EnergyConsumption / metrics.m_ProcTime;
#define MFLOPS_TO_GFLOPS (1e-3)
    /// Calculates the machine's energy efficiency (GFLOPs/watts)
    const double energyEfficiency = (metrics.m_ProcMflops * MFLOPS_TO_GFLOPS) / avgPowerConsumption;
#undef MFLOPS_TO_GFLOPS

    report["processed_mflops"] = metrics.m_ProcMflops;
    report["processed_time"] = metrics.m_ProcTime;
    report["processed_tasks"] = metrics.m_ProcTasks;
    report["forwarded_tasks"] = metrics.m_ForwardedTasks;
    report["energy_consumption"] = metrics.m_EnergyConsumption;
    report["average_processing_time"] = avgProcTime;
    report["idleness"] = metrics.m_Idleness;
    report["average_power_consumption"] = avgPowerConsumption;
    report["energy_efficiency"] = energyEfficiency;
    report["type"] = ispd::services::getServiceTypeName(ispd::services::ServiceType::MACHINE);
    report["simulated_on"] = "node_" + std::to_string(g_tw_mynode);

    /// Write the report of the current machine to the node metrics report.
    g_NodeMetricsReport->emplace(std::to_string(gid), report);
  }

  void notifyReport(const ispd::metrics::LinkMetrics &metrics,
                    const ispd::configuration::LinkConfiguration &configuration,
                    const tw_lpid gid) {
    nlohmann::json report;

    report["upward_communicated_time"] = metrics.upward_comm_time;
    report["downward_communicated_time"] = metrics.downward_comm_time;
    report["upward_communicated_mbits"] = metrics.upward_comm_mbits;
    report["downward_communicated_mbits"] = metrics.downward_comm_mbits;
    report["upward_communicated_packets"] = metrics.upward_comm_packets;
    report["downward_communicated_packets"] = metrics.downward_comm_packets;
    report["upward_waiting_time"] = metrics.upward_waiting_time;
    report["downward_waiting_time"] = metrics.downward_waiting_time;
    report["upward_idleness"] = metrics.m_UpwardIdleness;
    report["downward_idleness"] = metrics.m_DownwardIdleness;
    report["type"] = ispd::services::getServiceTypeName(ispd::services::ServiceType::LINK);
    report["simulated_on"] = "node_" + std::to_string(g_tw_mynode);

    /// Write the report of the current link to the node metrics report.
    g_NodeMetricsReport->emplace(std::to_string(gid), report);
  }

  void notifyReport(const ispd::metrics::MasterMetrics &metrics,
                    const tw_lpid gid) {
    nlohmann::json report;

    /// Calculates the master's average turnaround time.
    const double avgTurnaroundTime = metrics.total_turnaround_time / metrics.completed_tasks;

    report["completed_tasks"] = metrics.completed_tasks;
    report["total_turnaround_time"] = metrics.total_turnaround_time;
    report["average_turnaround_time"] = avgTurnaroundTime;
    report["type"] = ispd::services::getServiceTypeName(ispd::services::ServiceType::MASTER);
    report["simulated_on"] = "node_" + std::to_string(g_tw_mynode);

    /// Write the report of the current master to the node metrics report.
    g_NodeMetricsReport->emplace(std::to_string(gid), report);
  }

  void
  notifyReport(const ispd::metrics::SwitchMetrics &metrics,
               const ispd::configuration::SwitchConfiguration &configuration,
               const tw_lpid gid) {
    nlohmann::json report;

    report["upward_communicated_mbits"] = metrics.m_UpwardCommMbits;
    report["downward_communicated_mbits"] = metrics.m_DownwardCommMbits;
    report["upward_communicated_packets"] = metrics.m_UpwardCommPackets;
    report["downward_communicated_packets"] = metrics.m_DownwardCommPackets;

    /// Write the report of the current switch to the node metrics report.
    g_NodeMetricsReport->emplace(std::to_string(gid), report);
  }

  void reportNodeMetrics() {
    /// Forward the report to the node metrics collector.
    g_NodeMetricsCollector->reportNodeMetrics();
  }

  void reportNodeMetricsToFile() {
    /// The output stream in which the node-level metrics will be written to.
    std::ofstream out("node_" + std::to_string(g_tw_mynode) + ".json");

    /// Write the JSON content into the file using the prettified format.
    out << std::setw(2) << *g_NodeMetricsReport << std::endl;
  }
}; // namespace ispd::node_metrics

namespace ispd::global_metrics {

  ispd::metrics::GlobalMetricsCollector *g_GlobalMetricsCollector = new ispd::metrics::GlobalMetricsCollector();

  void reportGlobalMetrics() {
    /// Forward the report to the global metrics collector.
    g_GlobalMetricsCollector->reportGlobalMetrics();
  }

  void reportGlobalMetricsToFile(const std::filesystem::path reportFilePath) {
    g_GlobalMetricsCollector->reportGlobalMetricsToFile(reportFilePath);
  }
}; // namespace ispd::global_metrics
