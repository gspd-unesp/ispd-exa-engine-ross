#ifndef ISPD_SERVICE_LINK_HPP
#define ISPD_SERVICE_LINK_HPP

#include <ross.h>
#include <chrono>
#include <ispd/debug/debug.hpp>
#include <ispd/model/builder.hpp>
#include <ispd/message/message.hpp>
#include <ispd/metrics/metrics.hpp>
#include <ispd/configuration/link.hpp>

extern double g_NodeSimulationTime;

namespace ispd {
namespace services {

struct link_metrics {
  /// \brief The amount of communication time performed by the upward link.
  double upward_comm_time;

  /// \brief The amount of communication time performed by the downward link.
  double downward_comm_time;

  /// \brief The amount of communicated Mbits by the upward link.
  double upward_comm_mbits;

  /// \brief The amount of communicated Mbits by the downward link.
  double downward_comm_mbits;

  /// \brief The amount of communicated packets by the upward link.
  unsigned upward_comm_packets;

  /// \brief The amount of communicated packets by the downward link.
  unsigned downward_comm_packets;

  /// \brief The amount of upward waiting time occurred in this link.
  double upward_waiting_time;

  /// \brief The amount of downward waiting time occurred in this link.
  double downward_waiting_time;
};

struct link_state {
  /// \brief Link's ends.
  tw_lpid from;
  tw_lpid to;

  /// \brief Link's Configuration.
  ispd::configuration::LinkConfiguration conf;

  /// \brief Link's Metrics.
  link_metrics metrics;

  /// \brief Link's Queueing Model Information.
  double upward_next_available_time;
  double downward_next_available_time;
};

struct link {

  static void init(link_state *s, tw_lp *lp) {
    /// Fetch the service initializer from this logical process.
    const auto &service_initializer = ispd::this_model::getServiceInitializer(lp->gid);

    /// Call the service initializer for this logical process.
    service_initializer(s);
    
    /// Initialize link's metrics.
    s->metrics.upward_comm_time = 0;
    s->metrics.downward_comm_time = 0;
    s->metrics.upward_comm_mbits = 0;
    s->metrics.downward_comm_mbits = 0;
    s->metrics.upward_comm_packets = 0;
    s->metrics.downward_comm_packets = 0;
    s->metrics.upward_waiting_time = 0;
    s->metrics.downward_waiting_time = 0;

    /// Initialize queueing model information.
    s->upward_next_available_time = 0;
    s->downward_next_available_time = 0;

    /// Print a debug message.
    ispd_debug("Link %lu has been initialized.", lp->gid);
  }

  static void forward(link_state *s, tw_bf *bf, ispd_message *msg, tw_lp *lp) {
    ispd_debug("[Forward] Link %lu received a message at %lf of type (%d).", lp->gid, tw_now(lp), msg->type);

#ifdef DEBUG_ON
  const auto start = std::chrono::high_resolution_clock::now();
#endif // DEBUG_ON

    /// Fetch the communication size and calculates the communication time.
    const double comm_size = msg->task.m_CommSize;
    const double comm_time = s->conf.timeToCommunicate(comm_size);

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
      s->metrics.downward_comm_time += comm_time;
      s->metrics.downward_comm_mbits += comm_size;
      s->metrics.downward_comm_packets++;
      s->metrics.downward_waiting_time += waiting_delay;
    }
    /// Update the upward link's metrics.
    else {
      s->metrics.upward_comm_time += comm_time;
      s->metrics.upward_comm_mbits += comm_size;
      s->metrics.upward_comm_packets++;
      s->metrics.upward_waiting_time += waiting_delay;
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

    tw_event *const e = tw_event_new(send_to, g_tw_lookahead + departure_delay, lp);
    ispd_message *const m = static_cast<ispd_message *>(tw_event_data(e));

    m->type = message_type::ARRIVAL;
    m->task = msg->task; /// Copy the task's information.
    m->downward_direction = msg->downward_direction;
    m->route_offset = msg->route_offset;
    m->previous_service_id = lp->gid;
    m->is_vm = msg->is_vm;
    m->vm_fit = msg->vm_fit;
    m->vm_memory_space = msg->vm_memory_space;
    m->vm_num_cores = msg->vm_num_cores;
    m->vm_disk_space = msg->vm_disk_space;
    m->vm_id = msg->vm_id;

    /// Save information (for reverse computation).
    msg->saved_link_next_available_time = saved_next_available_time;
    msg->saved_waiting_time = waiting_delay;

    tw_event_send(e);

#ifdef DEBUG_ON
  const auto end = std::chrono::high_resolution_clock::now();
  const auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
  const auto timeTaken = static_cast<double>(duration.count());

  ispd::node_metrics::notifyMetric(ispd::metrics::NodeMetricsFlag::NODE_LINK_FORWARD_TIME, timeTaken);
#endif // DEBUG_ON
  }

  static void reverse(link_state *s, tw_bf *bf, ispd_message *msg, tw_lp *lp) {
    ispd_debug("[Reverse] Link %lu received a message at %lf of type (%d).", lp->gid, tw_now(lp), msg->type);

#ifdef DEBUG_ON
  const auto start = std::chrono::high_resolution_clock::now();
#endif // DEBUG_ON

    /// Fetch the communication size and calculates the communication time.
    const double comm_size = msg->task.m_CommSize;
    const double comm_time = s->conf.timeToCommunicate(comm_size);
    const double next_available_time = msg->saved_link_next_available_time;
    const double waiting_delay = msg->saved_waiting_time;

    /// Checks if the message is being sent from the master to the slave. Therefore,
    /// the downward next available time should be reverse processed.
    if (msg->downward_direction) {
      s->downward_next_available_time = next_available_time;

      /// Reverse the downward link's metrics.
      s->metrics.downward_comm_time -= comm_time;
      s->metrics.downward_comm_mbits -= comm_size;
      s->metrics.downward_comm_packets--;
      s->metrics.downward_waiting_time -= waiting_delay;
    }
    /// Otherwise, if the message is being sent from the slae to the master. Therefore
    /// the upward next available time should be reverse processed.
    else {
      s->upward_next_available_time = next_available_time;

      /// Reverse the upward link's metrics.
      s->metrics.upward_comm_time -= comm_time;
      s->metrics.upward_comm_mbits -= comm_size;
      s->metrics.upward_comm_packets--;
      s->metrics.upward_waiting_time -= waiting_delay;
    }

#ifdef DEBUG_ON
  const auto end = std::chrono::high_resolution_clock::now();
  const auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
  const auto timeTaken = static_cast<double>(duration.count());

  ispd::node_metrics::notifyMetric(ispd::metrics::NodeMetricsFlag::NODE_LINK_REVERSE_TIME, timeTaken);
#endif // DEBUG_ON
  }

  static void finish(link_state *s, tw_lp *lp) {
    const double lastActivityTime = std::max(s->downward_next_available_time,
        s->upward_next_available_time);
    const double linkTotalCommunicatedMBits = s->metrics.downward_comm_mbits +
        s->metrics.upward_comm_mbits;
    const double linkTotalCommunicationTime = s->metrics.downward_comm_time +
        s->metrics.upward_comm_time;
    const double linkTotalCommunicationWaitingTime = s->metrics.downward_waiting_time +
        s->metrics.upward_waiting_time;
    const double downwardIdleness = 1.0 - (s->metrics.downward_comm_time - s->metrics.downward_waiting_time) / s->metrics.downward_comm_time;
    const double upwardIdleness = 1.0 - (s->metrics.upward_comm_time - s->metrics.upward_waiting_time) / s->metrics.upward_comm_time;


    /// Report to the node`s metrics collector the last activity time
    /// of the machine in the simulation.
    ispd::node_metrics::notifyMetric(ispd::metrics::NodeMetricsFlag::NODE_SIMULATION_TIME, lastActivityTime);
    ispd::node_metrics::notifyMetric(ispd::metrics::NodeMetricsFlag::NODE_TOTAL_COMMUNICATED_MBITS, linkTotalCommunicatedMBits);
    ispd::node_metrics::notifyMetric(ispd::metrics::NodeMetricsFlag::NODE_TOTAL_COMMUNICATION_WAITING_TIME, linkTotalCommunicationWaitingTime);
    ispd::node_metrics::notifyMetric(ispd::metrics::NodeMetricsFlag::NODE_TOTAL_LINK_SERVICES);
    ispd::node_metrics::notifyMetric(ispd::metrics::NodeMetricsFlag::NODE_TOTAL_COMMUNICATION_TIME, linkTotalCommunicationTime);

    std::printf(
        "Link Queue Info & Metrics (%lu)\n"
        " - Downward Communicated Mbits..: %lf Mbits (%lu).\n"
        " - Downward Communicated Packets: %u packets (%lu).\n"
        " - Downward Waiting Time........: %lf seconds (%lu).\n"
        " - Downward Idleness............: %lf% (%lu).\n"
        " - Downward Next Avail. Time....: %lf seconds (%lu).\n"
        " - Upward Communicated Mbits....: %lf Mbits (%lu).\n"
        " - Upward Communicated Packets..: %u packets (%lu).\n"
        " - Upward Waiting Time..........: %lf seconds (%lu).\n"
        " - Upward Idleness..............: %lf% (%lu).\n"
        " - Upward Next Avail. Time......: %lf seconds (%lu).\n"
        "\n",
        lp->gid, 
        s->metrics.downward_comm_mbits, lp->gid,
        s->metrics.downward_comm_packets, lp->gid,
        s->metrics.downward_waiting_time, lp->gid,
        downwardIdleness * 100.0, lp->gid,
        s->downward_next_available_time, lp->gid,
        s->metrics.upward_comm_mbits, lp->gid,
        s->metrics.upward_comm_packets, lp->gid,
        s->metrics.upward_waiting_time, lp->gid,
        upwardIdleness * 100.0, lp->gid,
        s->upward_next_available_time, lp->gid
    );
  }
};

}; // namespace services
}; // namespace ispd

#endif // ISPD_SERVICE_LINK_HPP
