#ifndef ISPD_MESSAGE_H
#define ISPD_MESSAGE_H

#include <ispd/customer/task.hpp>

enum class message_type {
  GENERATE,
  ARRIVAL
};

struct ispd_message {
  /// \brief The message type.
  message_type type;

  /// \brief The message payload.
  ispd::customer::Task task;

  /// \brief Reverse Computational Fields.
  double saved_link_next_available_time;
  unsigned saved_core_index;
  double saved_core_next_available_time;
  double saved_waiting_time;

  /// \brief Route's descriptor.
  int route_offset;
  tw_lpid previous_service_id;


  /// \brief Sender information
  tw_lpid service_id;

  /// \brief Message flags.
  unsigned int downward_direction: 1;
  unsigned int task_processed: 1;
  tw_lpid machine_id;
  unsigned int: 5; /// Reversed flags.
};

#endif // ISPD_MESSAGE_H

