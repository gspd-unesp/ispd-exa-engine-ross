#include <iostream>
#include <memory>
#include <ross.h>
#include <ross-extern.h>
#include <ispd/log/log.hpp>
#include <ispd/model/builder.hpp>
#include <ispd/services/link.hpp>
#include <ispd/services/dummy.hpp>
#include <ispd/services/master.hpp>
#include <ispd/services/switch.hpp>
#include <ispd/services/machine.hpp>
#include <ispd/message/message.hpp>
#include <ispd/routing/routing.hpp>
#include <ispd/metrics/metrics.hpp>
#include <ispd/workload/workload.hpp>
#include <ispd/workload/interarrival.hpp>
#include <ispd/model_loader/model_loader.hpp>

static unsigned g_star_machine_amount = 10;
static unsigned g_star_task_amount = 100;

tw_peid mapping(tw_lpid gid) { return (tw_peid)gid / g_tw_nlp; }

tw_lptype lps_type[] = {
    {(init_f)ispd::services::master::init, (pre_run_f)NULL,
     (event_f)ispd::services::master::forward,
     (revent_f)ispd::services::master::reverse,
     (commit_f)ispd::services::master::commit,
     (final_f)ispd::services::master::finish, (map_f)mapping,
     sizeof(ispd::services::master_state)},
    {(init_f)ispd::services::link::init, (pre_run_f)NULL,
     (event_f)ispd::services::link::forward,
     (revent_f)ispd::services::link::reverse, (commit_f)NULL,
     (final_f)ispd::services::link::finish, (map_f)mapping,
     sizeof(ispd::services::link_state)},
    {(init_f)ispd::services::machine::init, (pre_run_f)NULL,
     (event_f)ispd::services::machine::forward,
     (revent_f)ispd::services::machine::reverse,
     (commit_f)ispd::services::machine::commit,
     (final_f)ispd::services::machine::finish, (map_f)mapping,
     sizeof(ispd::services::machine_state)},
    {(init_f)ispd::services::Switch::init, (pre_run_f)NULL,
     (event_f)ispd::services::Switch::forward,
     (revent_f)ispd::services::Switch::reverse, (commit_f)NULL,
     (final_f)ispd::services::Switch::finish, (map_f)mapping,
     sizeof(ispd::services::Switch)},
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
  ispd::log::setOutputFile(nullptr);

  /// Read the routing table from a specified file.
  ispd::routing_table::load("routes.route");
  
  /// @Temporary: Must be removed.
  ispd::model_loader::loadModel("model.json");

  tw_opt_add(opt);
  tw_init(&argc, &argv);

  // If the synchronization protocol is different from conservative then,
  // there is no need to have a conservative lookahead different from 0.
  if (g_tw_synchronization_protocol != CONSERVATIVE)
    g_tw_lookahead = 0;

  /// Checks if no user has been registered. If so, the program is immediately
  /// aborted, since at least one user must be registered.
  if (ispd::this_model::getUsers().size() == 0)
    ispd_error("At least one user must be registered.");

  /// The amount of services to have its logical process type to be set.
  const auto servicesSize = ispd::model_loader::getServicesSize();

  /// Distributed.
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
    const unsigned nlp_per_pe =
        (unsigned)ceil((double)servicesSize / tw_nnodes());

    /// Set the number of logical processes (LP) per processing element (PE).
    tw_define_lps(nlp_per_pe, sizeof(ispd_message));

    /// Calculate the first logical processes global identifier in this node.
    /// With that, it can be track if the logical process with that global
    /// identifier should be set as a dummy.
    tw_lpid current_gid = g_tw_mynode * nlp_per_pe;

    /// Count the amount of dummies that should be set to this node.
    unsigned dummy_count = 0;

    /// Set the links and machines.
    for (unsigned i = 0; i < nlp_per_pe; i++) {
      if (current_gid >= servicesSize) {
        tw_lp_settype(i,
                      &lps_type[ispd::model_loader::LogicalProcessType::DUMMY]);

        dummy_count++;
        current_gid++;
        continue;
      }

      /// The correspondent logical process type for the given logical process
      /// global identifier.
      const ispd::model_loader::LogicalProcessType type =
          ispd::model_loader::getLogicalProcessType(current_gid);

      /// Set the logical process type.
      tw_lp_settype(i, &lps_type[type]);

      current_gid++;
    }

    ispd_info("A total of %u dummies have been created at node %d.",
              dummy_count, g_tw_mynode);
  }
  /// Sequential.
  else {
    /// Set the total number of logical processes that should be created.
    tw_define_lps(servicesSize, sizeof(ispd_message));

    for (size_t i = 0; i < servicesSize; i++) {
      /// The logical process global identifier to be registered.
      const tw_lpid gid = static_cast<tw_lpid>(i);

      /// The correspondent logical process type for the given logical process
      /// global identifier.
      const ispd::model_loader::LogicalProcessType type =
          ispd::model_loader::getLogicalProcessType(gid);

      /// Set the logical process type.
      tw_lp_settype(gid, &lps_type[type]);
    }
  }

  tw_run();
  ispd::node_metrics::reportNodeMetrics();
  tw_end();

  ispd::global_metrics::reportGlobalMetrics();
  ispd::global_metrics::reportGlobalMetricsToFile("results.json");

  return 0;
}
