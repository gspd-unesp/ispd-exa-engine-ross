#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ispd/builder/builder.h>
#include <ispd/log.h>

lp_model_state *g_model;
size_t g_model_nlp;

void model_set_nlp(size_t nlp)
{
	// Checks if the model has previously been initialized.
	if(g_model)
		ispd_error("Model has already set its LPs amount (%zu).", g_model_nlp);

	g_model_nlp = nlp;

	// Check if was not possible to allocate a model with that amount of LPs.
	if(!(g_model = malloc(sizeof(lp_model_state) * g_model_nlp)))
		ispd_error("Model with %zu LPs could not be allocated.", g_model_nlp);

	ispd_log(LOG_DEBUG, "Model with %zu LPs has been initialized.", g_model_nlp);
}

void model_lp_settype(tw_lpid gid, lp_type type, void *state, size_t state_sz)
{
	// Check if the LP Global ID exceeds the maximum amount of LPs in the model.
	if(gid >= g_model_nlp)
		ispd_error("LP Global ID (%lu) exceeds the maximum amount of LPs in the model (%zu)", gid, g_model_nlp);

	lp_model_state *lp_state = &g_model[gid];

	// Check if a state with the specified state size could not be allocated.
	if(!(lp_state->state = malloc(state_sz)))
		ispd_error("LP state with size (%zu) could not be allocated.", state_sz);

	lp_state->type = type;
	memcpy(lp_state->state, state, state_sz);

	/*ispd_log(LOG_DEBUG, "Model set LP @ %lu with type %s.\n", gid, lp_type_names[type]);*/
}
