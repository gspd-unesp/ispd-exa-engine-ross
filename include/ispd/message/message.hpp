#ifndef ISPD_MESSAGE_H
#define ISPD_MESSAGE_H

#include <ispd/customer/task.hpp>
#include <vector>
#define  TASK_FOR_APP 20 //this will be for a while.

enum class message_type {
  GENERATE,
  ARRIVAL
};

/// struct  used in the cloud simulation. It is an extension of the grid workload that allows
/// user to send multiple tasks.
/// this type of message is used for virtual machines.
struct ispd_cloud_message{

    /// \brief Tasks inside the application.
    unsigned task_amount;

    /// \brief Origin and destination of this application
    tw_lpid m_Origin;
    tw_lpid m_Dest;

    double
            m_SubmitTime; ///< The time at which the application was submitted (in seconds).
    double m_EndTime; ///< The time at which the application completed execution (in
    ///< seconds).

    double m_ProcSize; ///< The processing size of the entire application, which is the sum of the processing size of each task
    double m_CommSize; ///< The communication size of the entire application, which is the sum of the comm size of each task.

    ispd::model::User::uid_t
            m_Owner; ///< The unique identifier of the application owner.


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



    /// \brief Virtual machine information
    tw_lpid vm_id;
    unsigned vm_num_cores;
    double vm_memory_space;
    double vm_disk_space;
    tw_lpid allocated_in;



  /// \brief Message flags.
  unsigned int downward_direction: 1;
  unsigned int task_processed: 1;
  unsigned int is_vm: 1;
  unsigned int vm_fit: 1;
  unsigned int: 4;
};


#endif // ISPD_MESSAGE_H
