#include <iostream>
#include <ross.h>
#include <ross-extern.h>
#include <ispd/log/log.hpp>
#include <ispd/model/builder.hpp>
#include <ispd/services/link.hpp>
#include <ispd/services/dummy.hpp>
#include <ispd/services/master.hpp>
#include <ispd/services/machine.hpp>
#include <ispd/message/message.hpp>
#include <ispd/routing/routing.hpp>

tw_peid mapping(tw_lpid gid) { return (tw_peid)gid / g_tw_nlp; }

tw_lptype lps_type[] = {
    {(init_f)ispd::services::master::init, (pre_run_f)NULL,
     (event_f)ispd::services::master::forward,
     (revent_f)ispd::services::master::reverse, (commit_f)NULL,
     (final_f)ispd::services::master::finish, (map_f)mapping,
     sizeof(ispd::services::master_state)},
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
    {(init_f)ispd::services::dummy::init, (pre_run_f)NULL,
     (event_f)ispd::services::dummy::forward,
     (revent_f)ispd::services::dummy::reverse, (commit_f)NULL,
     (final_f)ispd::services::dummy::finish, (map_f)mapping,
     sizeof(ispd::services::dummy_state)},
    {0},
};

const tw_optdef opt[] = {
    TWOPT_GROUP("iSPD Model"),
    TWOPT_END(),
};

/// \brief Global Routing Table.
ispd::routing::routing_table g_routing_table;

/// \brief Global Built Model.
ispd::model::built_model g_built_model;

int main(int argc, char **argv) {
  ispd::log::set_log_file(NULL);

  /// Read the routing table from a specified file.
  g_routing_table.load("routes.route");

  tw_opt_add(opt);
  tw_init(&argc, &argv);

  /// @Temporary:
  ispd::model::builder::register_service_initializer(0, [](void *state) {
    ispd::services::master_state *s =
        static_cast<ispd::services::master_state *>(state);

    /// Add the slaves.
    s->slaves.reserve(2);
    s->slaves.emplace_back(2);
    s->slaves.emplace_back(4);

    s->scheduler = new ispd::scheduler::round_robin;
    s->workload = new ispd::workload::workload_constant(10, 200.0, 80.0);
  });

  /// Distributed.
  if (tw_nnodes() > 1) {

    /// @Temporary: This should be removed later.
    if (tw_nnodes() != 2)
      ispd_error(
          "The distributed simulation must be executed with only 2 nodes.");

    tw_define_lps(3, sizeof(ispd_message));

    switch (g_tw_mynode) {
    case 0:
      tw_lp_settype(0, &lps_type[0]);
      tw_lp_settype(1, &lps_type[1]);
      tw_lp_settype(2, &lps_type[2]);
      break;
    case 1:
      tw_lp_settype(0, &lps_type[1]);
      tw_lp_settype(1, &lps_type[2]);
      tw_lp_settype(2, &lps_type[3]);
      break;
    default:
      std::cerr << "Unknown node (" << g_tw_mynode << ")" << std::endl;
      abort();
    }
  }
  /// Sequential.
  else {
    tw_define_lps(2 * 2 + 1, sizeof(ispd_message));
    tw_lp_settype(0, &lps_type[0]);
    for (int i = 1; i < 5; i += 2) {
      tw_lp_settype(i, &lps_type[1]);
      tw_lp_settype(i + 1, &lps_type[2]);
    }
  }
  /// @Temporary: End

  tw_run();
  tw_end();

  return 0;
}
