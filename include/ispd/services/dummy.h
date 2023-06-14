#ifndef SERVICE_DUMMY_H
#define SERVICE_DUMMY_H

#include <ispd/message.h>
#include <ross.h>

typedef struct dummy_state {
} dummy_state;

extern void dummy_init(dummy_state *s, tw_lp *lp);
extern void other_dummy_init(dummy_state *s, tw_lp *lp);
extern void dummy_event(dummy_state *s, tw_bf *bf, ispd_message *msg, tw_lp *lp);
extern void dummy_rc_event(dummy_state *s, tw_bf *bf, ispd_message *msg, tw_lp *lp);
extern void dummy_final(dummy_state *s, tw_lp *lp);

#endif // SERVICE_DUMMY_H
