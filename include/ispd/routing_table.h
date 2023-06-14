#ifndef ROUTING_TABLE_H
#define ROUTING_TABLE_H

// @Note When <ross.h> is included, this line should be removed.
typedef unsigned long tw_lpid;

typedef struct route {
  tw_lpid from;
  tw_lpid to;

  /// \brief A vector containing the route between a source and
  /// 	     a destination.
  tw_lpid *route;

  /// \brief The route length.
  unsigned length;
} route;

typedef struct routing_table_entry {
  struct routing_table_entry *next;
  route *value;
} routing_table_entry;

typedef struct routing_table {
  routing_table_entry **buckets;
  unsigned tablesize;
} routing_table;

extern void routing_table_get(routing_table *rt, tw_lpid from, tw_lpid to,
                              route **route);
extern void routing_table_load(routing_table **rt, const char *filepath);

#endif // ROUTING_TABLE_H
