#include <iostream>
#include <memory>
#include <ross.h>
#include <ross-extern.h>
#include <ispd/log/log.hpp>
#include <ispd/model/builder.hpp>
#include <ispd/services/link.hpp>
#include <ispd/services/dummy.hpp>
#include <ispd/services/virtual_machine.hpp>
#include <ispd/allocator/first_fit.hpp>
#include <ispd/allocator/first_fit_decreasing.hpp>
#include <ispd/services/master.hpp>
#include <ispd/services/switch.hpp>
#include <ispd/services/machine.hpp>
#include <ispd/message/message.hpp>
#include <ispd/routing/routing.hpp>
#include <ispd/metrics/metrics.hpp>
#include <ispd/workload/workload.hpp>
#include <ispd/workload/interarrival.hpp>
#include <ispd/services/VMM.hpp>
static unsigned g_star_machine_amount = 10;
static unsigned g_star_vm_amount = 13;
static unsigned g_star_task_amount = 0;

tw_peid mapping(tw_lpid gid) { return (tw_peid)gid / g_tw_nlp; }

tw_lptype lps_type[] = {
    {(init_f)ispd::services::VMM::init, (pre_run_f)NULL,
     (event_f)ispd::services::VMM::forward,
     (revent_f)ispd::services::VMM::reverse, (commit_f)NULL,
     (final_f)ispd::services::VMM::finish, (map_f)mapping,
     sizeof(ispd::services::VMM_state)},
    {(init_f)ispd::services::link::init, (pre_run_f)NULL,
     (event_f)ispd::services::link::forward,
     (revent_f)ispd::services::link::reverse, (commit_f)NULL,
     (final_f)ispd::services::link::finish, (map_f)mapping,
     sizeof(ispd::services::link_state)},
    {(init_f)ispd::services::machine::init, (pre_run_f)NULL,
     (event_f)ispd::services::machine::forward,
     (revent_f)ispd::services::machine::reverse, (commit_f)NULL,
     (final_f)ispd::services::machine::finish, (map_f)mapping,
     sizeof(ispd::services::machine_state)},
    {(init_f)ispd::services::virtual_machine::init, (pre_run_f)NULL,
     (event_f)ispd::services::virtual_machine::forward,
     (revent_f)ispd::services::virtual_machine::reverse, (commit_f)NULL,
     (final_f)ispd::services::virtual_machine::finish, (map_f)mapping,
     sizeof(ispd::services::VM_state)},
    {(init_f)ispd::services::dummy::init, (pre_run_f)NULL,
     (event_f)ispd::services::dummy::forward,
     (revent_f)ispd::services::dummy::reverse, (commit_f)NULL,
     (final_f)ispd::services::dummy::finish, (map_f)mapping,
     sizeof(ispd::services::dummy_state)},
    {0},
};

const tw_optdef opt[] = {
    TWOPT_GROUP("iSPD Model"),
    TWOPT_UINT("machine-amount", g_star_machine_amount,
               "number of machines to simulate"),
    TWOPT_UINT("task-amount", g_star_task_amount,
               "number of tasks to simulate"),
    TWOPT_END(),
};

int main(int argc, char **argv) {
  ispd::log::set_log_file(NULL);

  /// Read the routing table from a specified file.
  ispd::routing_table::load("/home/willao/ispd-exa-engine-ross/routes.route");

  tw_opt_add(opt);
  tw_init(&argc, &argv);

  const tw_lpid highest_machine_id = g_star_machine_amount * 2;
  const tw_lpid highest_link_id = highest_machine_id - 1;
  const tw_lpid highest_vm_id = highest_machine_id + g_star_vm_amount;

  /// Register the user.
  ispd::this_model::registerUser("User1", 100.0);
  /// register vmm
  std::vector<tw_lpid> machines;
  std::vector<tw_lpid> vms_ids;
  std::vector<double> vms_disk;
  std::vector<double> vms_memory;
  std::vector<unsigned> vms_cores;

  for (tw_lpid machine_id = 2; machine_id <= highest_machine_id;
       machine_id += 2)
    machines.emplace_back(machine_id);

  for (tw_lpid vm_id = highest_machine_id + 1; vm_id <= highest_vm_id;
       vm_id++) {

    vms_ids.emplace_back(vm_id);
    vms_disk.emplace_back(10);
    vms_cores.emplace_back(4);
    vms_memory.emplace_back(4);
  }


  ispd::this_model::registerVMM(
      0, std::move(vms_ids), std::move(vms_memory), std::move(vms_disk),
      std::move(vms_cores), std::move(machines), new ispd::allocator::first_fit,
      new ispd::scheduler::round_robin,
      ispd::workload::constant(
          "User1", 0, g_star_vm_amount, 100, 80,
          std::make_unique<ispd::workload::PoissonInterarrivalDistribution>(
              0.1)));

  /// Registers service initializers for the links.
  for (tw_lpid link_id = 1; link_id <= highest_link_id; link_id += 2)
    ispd::this_model::registerLink(link_id, 0, link_id + 1, 50.0, 0.0, 1.0);

  /// Registers serivce initializers for the machines.
  for (tw_lpid machine_id = 2; machine_id <= highest_machine_id;
       machine_id += 2)
    ispd::this_model::registerMachine(machine_id, 20.0, 0.0, 8, 16, 100,0,0);

  /// registers service initializer for the virtual machine.
  for (tw_lpid vm_id = highest_machine_id + 1; vm_id <= highest_vm_id; vm_id++)
    ispd::this_model::registerVM(vm_id, 10, 0.0,4, 4, 10 );

  /// Checks if no user has been registered. If so, the program is immediately
  /// aborted, since at least one user must be registered.
  if (ispd::this_model::getUsers().size() == 0)
    ispd_error("At least one user must be registered.");
  /// The total number of logical processes.
  const unsigned nlp = highest_vm_id + 1;

  if (tw_nnodes() > 1) {
    /// Here, since we are distributing the logical processes through many
    /// nodes, the number of logical processes (LP) per process element (PE)
    /// should be calculated.
    ///
    /// However, will be noted that when multiply the number of logical
    /// processes per process element (nlp_per_pe) by the number of available
    /// nodes (tw_nnodes()) the result may be greater than the total number of
    /// lps (nlp). In this case, in the last node, after all the required
    /// logical processes be created, the remaining logical processes are going
    /// to be set as dummies.
    const unsigned nlp_per_pe = (unsigned)ceil((double)nlp / tw_nnodes());

    /// Set the number of logical processes (LP) per processing element (PE).
    tw_define_lps(nlp_per_pe, sizeof(ispd_message));

    /// Calculate the first logical processes global identifier in this node.
    /// With that, it can be track if the logical process with that global
    /// identifier should be set as a dummy.
    tw_lpid current_gid = g_tw_mynode * nlp_per_pe;

    /// Count the amount of dummies that should be set to this node.
    unsigned dummy_count = 0;

    if (g_tw_mynode == 0) {
      /// Set the master logical process.
      tw_lp_settype(0, &lps_type[0]);

      /// Set the links and machines.
      for (unsigned i = 1; i < nlp_per_pe; i++) {
        if (current_gid > highest_machine_id) {

          if (current_gid <= highest_vm_id)
            tw_lp_settype(i, &lps_type[3]);
          else{
            tw_lp_settype(i, &lps_type[4]);
            dummy_count++;
          }

          dummy_count++;
          current_gid++;
          continue;
        }

        if (i & 1)
          tw_lp_settype(i, &lps_type[1]);
        else
          tw_lp_settype(i, &lps_type[2]);
        current_gid++;
      }
    } else {
      /// Set the links and machines.
      for (unsigned i = 0; i < nlp_per_pe; i++) {
        if (current_gid > highest_machine_id) {

          if (current_gid <= highest_vm_id)
            tw_lp_settype(i, &lps_type[3]);
          else{
            tw_lp_settype(i, &lps_type[4]);
            dummy_count++;
          }

          dummy_count++;
          current_gid++;
          continue;
        }

        if (current_gid & 1)
          tw_lp_settype(i, &lps_type[1]);
        else
          tw_lp_settype(i, &lps_type[2]);
        current_gid++;
      }
    }

    ispd_log(LOG_INFO, "A total of %u dummies have been created at node %d.",
             dummy_count, g_tw_mynode);
  }
  /// Sequential.
  else {
    /// Set the total number of logical processes that should be created.
    tw_define_lps(nlp, sizeof(ispd_message));

    /// The master type is set at the logical process with GID 0.
    tw_lp_settype(0, &lps_type[0]);

    /// Set the logical processes types.
    for (unsigned i = 1; i < highest_machine_id; i += 2) {
      /// Register at odd logical process identifier the link.
      tw_lp_settype(i, &lps_type[1]);

      // Register at even logical process identifier the machine.
      tw_lp_settype(i + 1, &lps_type[2]);
    }
    for (unsigned i = highest_machine_id + 1; i <= highest_vm_id; i++)
      tw_lp_settype(i, &lps_type[3]);


  }

  tw_run();
  ispd::node_metrics::reportNodeMetrics();
  tw_end();

  ispd::global_metrics::reportGlobalMetrics();

  return 0;
}
