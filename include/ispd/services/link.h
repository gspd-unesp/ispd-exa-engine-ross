#ifndef SERVICE_LINK_H
#define SERVICE_LINK_H

#include <ispd/message.h>
#include <ross.h>

typedef struct link_state {
	tw_lpid from;
	tw_lpid to;
	double bandwidth;
	double latency;
	double load;
	double comm_mbits;
	double comm_time;
	unsigned comm_packets;
} link_state;

extern void link_init(link_state *s, tw_lp *lp);
extern void link_event(link_state *s, tw_bf *bf, ispd_message *msg, tw_lp *lp);
extern void link_rc_event(link_state *s, tw_bf *bf, ispd_message *msg, tw_lp *lp);
extern void link_final(link_state *s, tw_lp *lp);

#endif // SERVICE_LINK_H
