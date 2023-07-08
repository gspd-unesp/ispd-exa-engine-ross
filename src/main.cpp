#include <iostream>
#include <ross.h>
#include <ross-extern.h>
#include <ispd/services/link.hpp>
#include <ispd/services/dummy.hpp>
#include <ispd/services/master.hpp>
#include <ispd/services/machine.hpp>
#include <ispd/message/message.hpp>
#include <ispd/routing/routing.hpp>

tw_peid mapping(tw_lpid gid)
{
	return (tw_peid)gid / g_tw_nlp;
}

tw_lptype lps_type[] = {
  {
    (init_f)ispd::services::master::init,
    (pre_run_f)NULL,
    (event_f)ispd::services::master::forward,
    (revent_f)ispd::services::master::reverse,
    (commit_f)NULL,
    (final_f)ispd::services::master::finish,
    (map_f)mapping,
    sizeof(ispd::services::master_state)
  },
  {
    (init_f)ispd::services::link::init,
    (pre_run_f)NULL,
    (event_f)ispd::services::link::forward,
    (revent_f)ispd::services::link::reverse,
    (commit_f)NULL,
    (final_f)ispd::services::link::finish,
    (map_f)mapping,
    sizeof(ispd::services::link_state)
  },
  {
    (init_f)ispd::services::machine::init,
    (pre_run_f)NULL,
    (event_f)ispd::services::machine::forward,
    (revent_f)ispd::services::machine::reverse,
    (commit_f)NULL,
    (final_f)ispd::services::machine::finish,
    (map_f)mapping,
    sizeof(ispd::services::machine_state)
  },
  {
    (init_f)ispd::services::dummy::init,
    (pre_run_f)NULL,
    (event_f)ispd::services::dummy::forward,
    (revent_f)ispd::services::dummy::reverse,
    (commit_f)NULL,
    (final_f)ispd::services::dummy::finish,
    (map_f)mapping,
    sizeof(ispd::services::dummy_state)
  },
  {0},
};

const tw_optdef opt[] = {
    TWOPT_GROUP("iSPD Model"),
    TWOPT_END(),
};

/// \brief Global Routing Table.
ispd::routing::routing_table g_routing_table;

int main(int argc, char **argv)
{
  /// Read the routing table from a specified file.
  g_routing_table.load("routes.route");
  
 	tw_opt_add(opt);
	tw_init(&argc, &argv);

  /// Distributed.
  if (tw_nnodes() > 1) {
    if (tw_nnodes() != 3) {
      std::cerr << "It must be executed using 3 nodes." << std::endl;
      abort();
    }

    tw_define_lps(1, sizeof(ispd_message));
    tw_lp_settype(0, &lps_type[g_tw_mynode]);
  } 
  /// Sequential.
  else {
    tw_define_lps(3, sizeof(ispd_message));
    tw_lp_settype(0, &lps_type[0]);
    tw_lp_settype(1, &lps_type[1]);
    tw_lp_settype(2, &lps_type[2]);
  }

	tw_run();
	tw_end();

	return 0;
}
