#ifndef ISPD_SERVICE_SWITCH_HPP
#define ISPD_SERVICE_SWITCH_HPP

#include <ross.h>
#include <chrono>
#include <ispd/debug/debug.hpp>
#include <ispd/model/builder.hpp>
#include <ispd/message/message.hpp>
#include <ispd/routing/routing.hpp>
#include <ispd/metrics/metrics.hpp>
#include <ispd/configuration/switch.hpp>

namespace ispd::services {

struct SwitchMetrics {
  double m_UpwardCommMbits;
  double m_DownwardCommMbits;

  unsigned m_UpwardCommPackets;
  unsigned m_DownwardCommPackets;
};

struct SwitchState {
  ispd::configuration::SwitchConfiguration m_Conf;
  SwitchMetrics m_Metrics;
};

struct Switch {
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
               lp->gid, s->m_Conf.getBandwidth(), s->m_Conf.getLoad(),
               s->m_Conf.getLatency());
  }

  static void forward(SwitchState *s, tw_bf *bf, ispd_message *msg, tw_lp *lp) {
    ispd_debug("[Forward] Switch %lu received a message at %lf of type (%d) "
               "and route offset (%u).",
               lp->gid, tw_now(lp), msg->type, msg->route_offset);

#ifdef DEBUG_ON
    const auto start = std::chrono::high_resolution_clock::now();
#endif // DEBUG_ON

    /// Fetch the communication size and calculate the communication time.
    const double commSize = msg->task.m_CommSize;
    const double commTime = s->m_Conf.timeToCommunicate(commSize);

    /// Update the switch's metrics.
    if (msg->downward_direction) {
      s->m_Metrics.m_DownwardCommMbits += commSize;
      s->m_Metrics.m_DownwardCommPackets++;
    } else {
      s->m_Metrics.m_UpwardCommMbits += commSize;
      s->m_Metrics.m_UpwardCommPackets++;
    }

    const ispd::routing::Route *route =
        ispd::routing_table::getRoute(msg->task.m_Origin, msg->task.m_Dest);

    tw_event *const e = tw_event_new(route->get(msg->route_offset),
                                     g_tw_lookahead + commTime, lp);
    ispd_message *const m = static_cast<ispd_message *>(tw_event_data(e));

    m->type = message_type::ARRIVAL;
    m->task = msg->task; /// Copies the task information.
    m->task_processed = msg->task_processed;
    m->downward_direction = msg->downward_direction;
    m->route_offset = msg->downward_direction ? (msg->route_offset + 1)
                                              : (msg->route_offset - 1);
    m->previous_service_id = lp->gid;

    tw_event_send(e);

#ifdef DEBUG_ON
    const auto end = std::chrono::high_resolution_clock::now();
    const auto duration =
        std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    const auto timeTaken = static_cast<double>(duration.count());

    ispd::node_metrics::notifyMetric(
        ispd::metrics::NodeMetricsFlag::NODE_SWITCH_FORWARD_TIME, timeTaken);
#endif // DEBUG_ON
  }

  static void reverse(SwitchState *s, tw_bf *bf, ispd_message *msg, tw_lp *lp) {
    ispd_debug("[Reverse] Switch %lu received a message at %lf of type (%d).",
               lp->gid, tw_now(lp), msg->type);

#ifdef DEBUG_ON
    const auto start = std::chrono::high_resolution_clock::now();
#endif // DEBUG_ON

    const double commSize = msg->task.m_CommSize;
    const double commTime = s->m_Conf.timeToCommunicate(commSize);

    /// Reverse the switch's metrics.
    if (msg->downward_direction) {
      s->m_Metrics.m_DownwardCommMbits -= commSize;
      s->m_Metrics.m_DownwardCommPackets--;
    } else {
      s->m_Metrics.m_UpwardCommMbits -= commSize;
      s->m_Metrics.m_UpwardCommPackets--;
    }

#ifdef DEBUG_ON
    const auto end = std::chrono::high_resolution_clock::now();
    const auto duration =
        std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    const auto timeTaken = static_cast<double>(duration.count());

    ispd::node_metrics::notifyMetric(
        ispd::metrics::NodeMetricsFlag::NODE_SWITCH_REVERSE_TIME, timeTaken);
#endif // DEBUG_ON
  }

  static void finish(SwitchState *s, tw_lp *lp) {
    ispd::node_metrics::notifyMetric(
        ispd::metrics::NodeMetricsFlag::NODE_TOTAL_MASTER_SERVICES);

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
