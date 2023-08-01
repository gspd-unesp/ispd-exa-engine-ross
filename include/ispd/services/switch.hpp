#ifndef ISPD_SERVICE_SWITCH_HPP
#define ISPD_SERVICE_SWITCH_HPP

#include <ross.h>
#include <ispd/debug/debug.hpp>
#include <ispd/message/message.hpp>
#include <ispd/routing/routing.hpp>

namespace ispd::services {

struct SwitchMetrics {
  double m_UpwardCommMbits;
  double m_DownwardCommMbits;

  unsigned m_UpwardCommPackets;
  unsigned m_DownwardCommPackets;
};

struct SwitchConfiguration {
  double m_Bandwidth;
  double m_Load;
  double m_Latency;
};

struct SwitchState {
  SwitchConfiguration m_Conf;
  SwitchMetrics m_Metrics;
};

struct Switch {
  static inline double timeToComm(const SwitchConfiguration &conf,
                                  const double commSize) {
    return conf.m_Latency + commSize / ((1.0 - conf.m_Load) * conf.m_Bandwidth);
  }

  static void init(SwitchState *s, tw_lp *lp) {
    /// Fetch the service initializer from this logical process.
    const auto &serviceInitializer =
        ispd::this_model::getServiceInitializer(lp->gid);

    /// Call the service initializer for this logical process.
    serviceInitializer(s);

    /// Initialize the switch's metrics.
    s->m_Metrics.m_UpwardCommMbits = 0;
    s->m_Metrics.m_DownwardCommMbits = 0;
    s->m_Metrics.m_UpwardCommPackets = 0;
    s->m_Metrics.m_DownwardCommPackets = 0;

    ispd_debug("Switch %lu has been initialized (B: %lf, L: %lf, LT: %lf).",
               lp->gid, s->m_Conf.m_Bandwidth, s->m_Conf.m_Latency,
               s->m_Conf.m_Latency);
  }

  static void forward(SwitchState *s, tw_bf *bf, ispd_message *msg, tw_lp *lp) {
    ispd_debug("[Forward] Switch %lu received a message at %lf of type (%d) and route offset (%u).",
               lp->gid, tw_now(lp), msg->type, msg->route_offset);

    /// Fetch the communication size and calculate the communication time.
    const double commSize = msg->task.comm_size;
    const double commTime = timeToComm(s->m_Conf, commSize);

    /// Update the switch's metrics.
    if (msg->downward_direction) {
      s->m_Metrics.m_DownwardCommMbits += commSize;
      s->m_Metrics.m_DownwardCommPackets++;
    } else {
      s->m_Metrics.m_UpwardCommMbits += commSize;
      s->m_Metrics.m_UpwardCommPackets++;
    }

    const ispd::routing::route *route =
        g_routing_table.get_route(msg->task.origin, msg->task.dest);

    tw_event *const e = tw_event_new(route->get(msg->route_offset), commTime, lp);
    ispd_message *const m = static_cast<ispd_message *>(tw_event_data(e));

    m->type = message_type::ARRIVAL;
    m->task = msg->task; /// Copies the task information.
    m->task_processed = msg->task_processed;
    m->downward_direction = msg->downward_direction;
    m->route_offset = msg->downward_direction ? (msg->route_offset + 1) : (msg->route_offset -1);
    m->previous_service_id = lp->gid;

    tw_event_send(e);
  }

  static void reverse(SwitchState *s, tw_bf *bf, ispd_message *msg, tw_lp *lp) {
    ispd_debug("[Reverse] Switch %lu received a message at %lf of type (%d).",
               lp->gid, tw_now(lp), msg->type);

    const double commSize = msg->task.comm_size;
    const double commTime = timeToComm(s->m_Conf, commSize);

    /// Reverse the switch's metrics.
    if (msg->downward_direction) {
      s->m_Metrics.m_DownwardCommMbits -= commSize;
      s->m_Metrics.m_DownwardCommPackets--;
    } else {
      s->m_Metrics.m_UpwardCommMbits -= commSize;
      s->m_Metrics.m_UpwardCommPackets--;
    }
  }

  static void finish(SwitchState *s, tw_lp *lp) {
    std::printf("Switch Queue Info & Metrics (%lu)\n"
                " - Downward Communicated Mbits..: %lf Mbits (%lu).\n"
                " - Downward Communicated Packets: %u packets (%lu).\n"
                " - Upward Communicated Mbits....: %lf Mbits (%lu).\n"
                " - Upward Communicated Packets..: %u packets (%lu).\n"
                "\n",
                lp->gid, s->m_Metrics.m_DownwardCommMbits, lp->gid,
                s->m_Metrics.m_DownwardCommPackets, lp->gid,
                s->m_Metrics.m_UpwardCommMbits, lp->gid,
                s->m_Metrics.m_UpwardCommPackets, lp->gid);
  }
};

}; // namespace ispd::services

#endif // ISPD_SERVICE_SWITCH_HPP
