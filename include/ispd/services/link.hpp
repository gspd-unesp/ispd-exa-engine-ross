#ifndef ISPD_SERVICE_LINK_HPP
#define ISPD_SERVICE_LINK_HPP

#include <ross.h>
#include <ispd/debug/debug.hpp>
#include <ispd/message/message.hpp>
#include <ispd/model/builder.hpp>

namespace ispd {
namespace services {

struct link_metrics {
  /// \brief The amount of communicated Mbits by the upward link.
  double upward_comm_mbits;

  /// \brief The amount of communicated Mbits by the downward link.
  double downward_comm_mbits;

  /// \brief The amount of communicated packets by the upward link.
  unsigned upward_comm_packets;

  /// \brief The amount of communicated packets by the downward link.
  unsigned downward_comm_packets;
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
    /// Fetch the service initializer from this logical process.
    const auto &service_initializer = ispd::model::builder::get_service_initializer(lp->gid);

    /// Call the service initializer for this logical process.
    service_initializer(s);
    
    /// Initialize link's metrics.
    s->metrics.upward_comm_mbits = 0;
    s->metrics.downward_comm_mbits = 0;
    s->metrics.upward_comm_packets = 0;
    s->metrics.downward_comm_packets = 0;

    /// Initialize queueing model information.
    s->upward_next_available_time = 0;
    s->downward_next_available_time = 0;

    /// Print a debug message.
    ispd_debug("Link %lu has been initialized.", lp->gid);
  }

  static void forward(link_state *s, tw_bf *bf, ispd_message *msg, tw_lp *lp) {
    ispd_debug("[Forward] Link %lu received a message at %lf of type (%d).", lp->gid, tw_now(lp), msg->type);

    /// Fetch the communication size and calculates the communication time.
    const double comm_size = msg->task.comm_size;
    const double comm_time = time_to_comm(&s->conf, comm_size);

    /// Here is selected which available time should be used, i.e., if the
    /// messages is being sent from the master to the slave, then the downward
    /// link is being used and, therefore, the downward next available time
    /// is used, otherwise, if the slave is sent the results to the master,
    /// then the upward link is being used.
    double next_available_time;
    double saved_next_available_time;

    if (msg->downward_direction)
      next_available_time = s->downward_next_available_time;
    else
      next_available_time = s->upward_next_available_time;
    saved_next_available_time = next_available_time;

    /// Calculate the waiting delay and the departure delay.
    const double waiting_delay = ROSS_MAX(0.0, next_available_time - tw_now(lp));
    const double departure_delay = waiting_delay + comm_time;

    /// Update the downward link's metrics.
    if (msg->downward_direction) {
      s->metrics.downward_comm_mbits += comm_size;
      s->metrics.downward_comm_packets++;
    }
    /// Update the upward link's metrics.
    else {
      s->metrics.upward_comm_mbits += comm_size;
      s->metrics.upward_comm_packets++;
    }

    next_available_time = tw_now(lp) + departure_delay;

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
    m->route_offset = msg->route_offset;
    m->previous_service_id = lp->gid;

    /// Save information (for reverse computation).
    msg->saved_link_next_available_time = saved_next_available_time;

    tw_event_send(e);
  }

  static void reverse(link_state *s, tw_bf *bf, ispd_message *msg, tw_lp *lp) {
    ispd_debug("[Reverse] Link %lu received a message at %lf of type (%d).", lp->gid, tw_now(lp), msg->type);

    /// Fetch the communication size and calculates the communication time.
    const double comm_size = msg->task.comm_size;
    const double comm_time = time_to_comm(&s->conf, comm_size);

    /// Checks if the message is being sent from the master to the slave. Therefore,
    /// the downward next available time should be reverse processed.
    if (msg->downward_direction) {
      s->downward_next_available_time = msg->saved_link_next_available_time;

      /// Reverse the downward link's metrics.
      s->metrics.downward_comm_mbits -= comm_size;
      s->metrics.downward_comm_packets--;
    }
    /// Otherwise, if the message is being sent from the slae to the master. Therefore
    /// the upward next available time should be reverse processed.
    else {
      s->upward_next_available_time = msg->saved_link_next_available_time;

      /// Reverse the upward link's metrics.
      s->metrics.upward_comm_mbits -= comm_size;
      s->metrics.upward_comm_packets--;
    }
  }

  static void finish(link_state *s, tw_lp *lp) {
        std::printf(
            "Link Queue Info & Metrics (%lu)\n"
            " - Downward Communicated Mbits..: %lf Mbits (%lu).\n"
            " - Downward Communicated Packets: %u packets (%lu).\n"
            " - Downward Next Avail. Time....: %lf seconds (%lu).\n"
            " - Upward Communicated Mbits....: %lf Mbits (%lu).\n"
            " - Upward Communicated Packets..: %u packets (%lu).\n"
            " - Upward Next Avail. Time......: %lf seconds (%lu).\n"
            "\n",
            lp->gid, 
            s->metrics.downward_comm_mbits, lp->gid,
            s->metrics.downward_comm_packets, lp->gid,
            s->downward_next_available_time, lp->gid,
            s->metrics.upward_comm_mbits, lp->gid,
            s->metrics.upward_comm_packets, lp->gid,
            s->upward_next_available_time, lp->gid
        );
  }
};

}; // namespace services
}; // namespace ispd

#endif // ISPD_SERVICE_LINK_HPP
