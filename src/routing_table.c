#include <ispd/log.h>
#include <ispd/core.h>
#include <ispd/routing_table.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define IS_POWER_OF_TWO(X) ((!((X) == 0)) && (!((X) & ((X)-1))))
#define MAX_ROUTE_LENGTH_IN_FILE (1024)

static inline route *route_create(tw_lpid from, tw_lpid to, unsigned long length)
{
	route *r;

	// Check if a `route` structure could not be allocated. If so, the
	// program is exited.
	if(!(r = malloc(sizeof(route)))) {
		printf("A route structure could not be allocated.\n");
		exit(EXIT_FAILURE);
	}

	r->from = from;
	r->to = to;
	r->length = length;

	// Check if a route with the specified amount of entries could not be
	// allocated. If so, the program is exited.
	if(!(r->route = malloc(sizeof(tw_lpid) * length))) {
		printf("A route with lenth %lu could not be allocated.\n", length);
		exit(EXIT_FAILURE);
	}

	return r;
}

static inline void route_print(const route *r)
{
	unsigned i;

	printf("[ROUTE]: (%lu, %lu): ", r->from, r->to);
	for(i = 0; i < r->length - 1; i++)
		printf("%lu -> ", r->route[i]);
	printf("%lu.\n", r->route[r->length - 1]);
}

static inline unsigned long next_power_of_twol(unsigned long v)
{
	v--;
	v |= v >> 1L;
	v |= v >> 2L;
	v |= v >> 4L;
	v |= v >> 8L;
	v |= v >> 16L;
	v++;
	return v;
}

static inline size_t hash(tw_lpid from, tw_lpid to)
{
	return from >= to ? from * from + from + to : to * to + from;
}

static inline size_t bucket(const size_t hash, const unsigned tablesize)
{
	return hash & (tablesize - 1);
}

static inline void routing_table_init(routing_table **rt, const unsigned tablesize)
{
	// Check if there is not enough memory to allocate a routing table
	// structure. If so, the program is exited.
	if(!((*rt) = malloc(sizeof(struct routing_table))))
		ispd_error("It was not possible to allocate a "
			   "`routing_table` structure.");

	(*rt)->tablesize = next_power_of_twol(tablesize);
	(*rt)->buckets = malloc(sizeof(routing_table_entry *) * (*rt)->tablesize);

	// Check if there is not enough memory to allocate a routing table with
	// the specified amount of buckets. If so, the program is exited.
	if(!(*rt)->buckets)
		ispd_error("It was not possible to allocate a `routing_table` containing %u "
			   "entries.",
		    (*rt)->tablesize);

	ispd_log(LOG_DEBUG,
	    "A routing table with %u entries has been "
	    "initialized.",
	    (*rt)->tablesize);
}

static inline void routing_table_insert(routing_table *rt, route *r)
{
	size_t bucket_index;
	routing_table_entry *entry;

	// Check if the routing table entry could not be allocated. If so,
	// the program is exited.
	if(!(entry = malloc(sizeof(routing_table_entry))))
		ispd_error("It was not possible to allocate a routing table "
			   "entry.");

	bucket_index = bucket(hash(r->from, r->to), rt->tablesize);

	// The entries are added in a stack way, that is, the last entry
	// that has been added to a bucket is the head of that bucket.
	entry->next = rt->buckets[bucket_index];
	entry->value = r;
	rt->buckets[bucket_index] = entry;

	ispd_log(LOG_DEBUG, "Route [%lu to %lu, L: %lu] has been inserted at bucket %zu.", r->from, r->to, r->length,
	    bucket_index);
}

void routing_table_get(routing_table *rt, tw_lpid from, tw_lpid to, route **route)
{
	size_t bucket_index;
	routing_table_entry *e;

	// Calculate the bucket index in which the route will be. With that,
	// we can access the linked list that represents that bucket. Therefore,
	// traversing that list we can find the route that we are searching for.
	bucket_index = bucket(hash(from, to), rt->tablesize);

	for(e = rt->buckets[bucket_index]; e; e = e->next) {
		if(e->value->from == from && e->value->to == to) {
			*route = e->value;
			break;
		}
	}
}

static inline void routing_table_route_parse(routing_table *rt, char *route_line, unsigned line_number)
{
	char *tok;
	unsigned i, route_length;
	tw_lpid from, to;
	route *r;

	tok = strtok(route_line, " ");
	// Check if the current rouute has not specified the `from` vertex. If so,
	// the program is exited.
	if(!tok)
		ispd_error("Route at line %u has not specified the `from` "
			   "vertex of the route.",
		    line_number);
	from = atol(tok);

	tok = strtok(NULL, " ");
	// Check if the current route has not specified the `to` vertex. If so,
	// the program is exited.
	if(!tok)
		ispd_error("Route at line %u has not specified the `to` "
			   "vertex of the route.",
		    line_number);
	to = atol(tok);

	tok = strtok(NULL, " ");
	// Check if the current route has not specified the `route length`. If so,
	// the program is exited.
	if(!tok)
		ispd_error("Route at line %u has not specified the `route "
			   "length` of the route.",
		    line_number);
	route_length = atol(tok);

	r = route_create(from, to, route_length);

	i = 0;
	while((tok = strtok(NULL, " ")))
		r->route[i++] = atoi(tok);

	routing_table_insert(rt, r);
}

void routing_table_load(routing_table **rt, const char *filepath)
{
	FILE *fp;
	char c, buffer[MAX_ROUTE_LENGTH_IN_FILE];
	long line_count;
	unsigned line_number;

	// Check if the file could not be opened for some reason. If so,
	// the program is exited.
	if(!(fp = fopen(filepath, "r")))
		ispd_error("Routing table file `%s` could not be opened.", filepath);

	line_count = 0;
	while((c = getc(fp)) != EOF)
		if(c == '\n')
			line_count++;

	routing_table_init(rt, line_count);

	// Since we have read the file for couting the amount of routes
	// there are in the file, it is necessary to seek the file again
	// to its beginning to start a new reading. In this case, the
	// reading of each route in the file.
	fseek(fp, 0, SEEK_SET);

	line_number = 0;
	while(fgets(buffer, MAX_ROUTE_LENGTH_IN_FILE, fp) != NULL) {
		line_number++;

		// Make it a NULL-terminated string.
		buffer[strcspn(buffer, "\n")] = 0;

		// Parse the route line that has been previously read from the route file.
		routing_table_route_parse(*rt, buffer, line_number);
	}

	fclose(fp);
}
