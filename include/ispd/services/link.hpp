#ifndef ISPD_SERVICE_LINK_HPP
#define ISPD_SERVICE_LINK_HPP

#include <ross.h>
#include <ispd/debug/debug.hpp>
#include <ispd/message/message.hpp>

namespace ispd {
namespace services {

struct link_metrics {
  double comm_mbits;
  unsigned comm_packets;
};

struct link_configuration {
  double bandwidth;
  double load;
  double latency;
};

struct link_state {
  /// \brief Link's ends.
  tw_lpid from;
  tw_lpid to;

  /// \brief Link's Configuration.
  link_configuration conf;

  /// \brief Link's Metrics.
  link_metrics metrics;

  /// \brief Link's Queueing Model Information.
  double upward_next_available_time;
  double downward_next_available_time;
};

struct link {

  static double time_to_comm(const link_configuration *const conf,
                           const double comm_size) {
    return conf->latency + comm_size / ((1.0 - conf->load) * conf->bandwidth);
  }

  static void init(link_state *s, tw_lp *lp) {
    /// @Todo: Initialize the link configuration dynamically
    ///        using a model builder.

    /// @Temporary:
    s->from = 0;
    s->to = 2;
    s->conf.bandwidth = 50;
    s->conf.load = 0.0;
    s->conf.latency = 2;
    /// @Temporary: End
    
    /// Initialize link's metrics.
    s->metrics.comm_mbits = 0;
    s->metrics.comm_packets = 0;

    /// Initialize queueing model information.
    s->upward_next_available_time = 0;
    s->downward_next_available_time = 0;
  }

  static void forward(link_state *s, tw_bf *bf, ispd_message *msg, tw_lp *lp) {
    /// Fetch the communication size and calculates the communication time.
    const double comm_size = msg->task.comm_size;
    const double comm_time = time_to_comm(&s->conf, comm_size);

    /// Here is selected which available time should be used, i.e., if the
    /// messages is being sent from the master to the slave, then the downward
    /// link is being used and, therefore, the downward next available time
    /// is used, otherwise, if the slave is sent the results to the master,
    /// then the upward link is being used.
    double next_available_time;

    if (msg->downward_direction)
      next_available_time = s->downward_next_available_time;
    else
      next_available_time = s->upward_next_available_time;

    /// Calculate the waiting delay and the departure delay.
    const double waiting_delay = ROSS_MAX(0.0, next_available_time - tw_now(lp));
    const double departure_delay = waiting_delay + comm_time;

    /// Update the link's metrics.
    s->metrics.comm_mbits += comm_size;
    s->metrics.comm_packets++;

    tw_lpid send_to;

    /// Update the link's queueing model information.
    if (msg->downward_direction) {
      s->downward_next_available_time = next_available_time;
      send_to = s->to;
    } else {
      s->upward_next_available_time = next_available_time;
      send_to = s->from;
    }

    DEBUG({
        /// Checks if the incoming messages has been arrived
        /// from a logical process that differs from the link's ends.
        /// If so, the program is immediately aborted.
        if (msg->previous_service_id != s->to &&
            msg->previous_service_id != s->from) {
            std::printf("Link with GID %lu has received a packet from a service different from its ends (%lu).", lp->gid, msg->previous_service_id);
            abort();
        }
    });

    tw_event *const e = tw_event_new(send_to, departure_delay, lp);
    ispd_message *const m = static_cast<ispd_message *>(tw_event_data(e));

    m->type = message_type::ARRIVAL;
    m->task = msg->task; /// Copy the task's information.
    m->downward_direction = 1;
    m->saved_link_next_available_time = next_available_time;
    m->route_offset = msg->route_offset;
    m->previous_service_id = lp->gid;

    tw_event_send(e);
  }

  static void reverse(link_state *s, tw_bf *bf, ispd_message *msg, tw_lp *lp) {
    /// Fetch the communication size and calculates the communication time.
    const double comm_size = msg->task.comm_size;
    const double comm_time = time_to_comm(&s->conf, comm_size);

    /// Checks if the message is being sent from the master to the slave. Therefore,
    /// the downward next available time should be reverse processed.
    if (msg->downward_direction)
      s->downward_next_available_time = msg->saved_link_next_available_time;
    /// Otherwise, if the message is being sent from the slae to the master. Therefore
    /// the upward next available time should be reverse processed.
    else
      s->upward_next_available_time = msg->saved_link_next_available_time;

    /// Reverse the link's metrics.
    s->metrics.comm_mbits -= comm_size;
    s->metrics.comm_packets--;
  }

  static void finish(link_state *s, tw_lp *lp) {
    DEBUG({
        std::printf(
            "Link Metrics (%lu)\n"
            " - Communicated Mbits..: %lf Mbits (%lu).\n"
            " - Communicated Packets: %u packets (%lu).\n"
            "\n",
            lp->gid, 
            s->metrics.comm_mbits, lp->gid,
            s->metrics.comm_packets, lp->gid
        );
    });
  }
};

}; // namespace services
}; // namespace ispd

#endif // ISPD_SERVICE_LINK_HPP
