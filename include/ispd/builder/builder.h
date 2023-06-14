#ifndef BUILDER_H
#define BUILDER_H

#include <ross.h>

typedef enum _lp_type {
	MASTER,
	LINK,
	MACHINE,
	DUMMY,
	OTHER_DUMMY,
	LP_TYPE_COUNT,
} lp_type;

typedef struct _lp_model_state {
	lp_type type;
	void *state;
} lp_model_state;

static const char *lp_type_names[LP_TYPE_COUNT] = {
    [MASTER] = "MASTER",
    [LINK] = "LINK",
    [MACHINE] = "MACHINE",
    [DUMMY] = "DUMMY",
    [OTHER_DUMMY] = "OTHER_DUMMY",
};

extern lp_model_state *g_model;
extern size_t g_model_nlp;

extern void model_set_nlp(size_t nlp);
extern void model_lp_settype(tw_lpid gid, lp_type type, void *state, size_t state_sz);

#endif // BUILDER_H
