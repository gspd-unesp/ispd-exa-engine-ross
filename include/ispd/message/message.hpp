#ifndef ISPD_MESSAGE_H
#define ISPD_MESSAGE_H

#include <ispd/customer/task.hpp>
#include <vector>

enum class message_type {
  GENERATE,
  ARRIVAL
};

struct ispd_message {
  /// \brief The message type.
  message_type type;

  /// \brief The message payload.
  ispd::customer::Task task;

  /// \brief Application for cloud simulation.
  struct ispd_cloud_message application;

  /// \brief Reverse Computational Fields.
  double saved_link_next_available_time;
  unsigned saved_core_index;
  double saved_core_next_available_time;
  double saved_waiting_time;

  /// \brief Route's descriptor.
  int route_offset;
  tw_lpid previous_service_id;



  /// \brief Message flags.
  unsigned int downward_direction: 1;
  unsigned int task_processed: 1;
  unsigned int is_vm: 1;
  unsigned int: 5;
};
/// struct  used in the cloud simulation. It is an extension of the grid workload that allows
/// user to send multiple tasks.
struct ispd_cloud_message{

    /// \brief Tasks inside the application.
    unsigned task_amount;
    std::vector<ispd::customer::Task> tasks;


    /// \brief Virtual machine information
    tw_lpid vm_id;
    unsigned vm_num_cores;
    double vm_memory_space;
    double vm_disk_space;
    tw_lpid allocated_in;

    /// \brief Message flags.
    unsigned int vm_fit;

    /// \brief Origin and destination of this application
    tw_lpid origin;
    tw_lpid dest;



};

#endif // ISPD_MESSAGE_H
