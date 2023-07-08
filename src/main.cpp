#include <iostream>
#include <ross.h>
#include <ross-extern.h>
#include <ispd/services/dummy.hpp>
#include <ispd/message/message.hpp>

tw_peid mapping(tw_lpid gid)
{
	return (tw_peid)gid / g_tw_nlp;
}

tw_lptype lps_type[] = {
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

int main(int argc, char **argv)
{
  std::cout << "Printing" << std::endl;

	tw_opt_add(opt);
	tw_init(&argc, &argv);

  tw_define_lps(1, sizeof(ispd_message));
  tw_lp_settype(0, &lps_type[0]);

	tw_run();
	tw_end();
	return 0;
}
