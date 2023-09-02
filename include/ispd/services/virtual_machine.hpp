
#ifndef ISPD_VIRTUAL_MACHINE_HPP
#define ISPD_VIRTUAL_MACHINE_HPP
#include <ross.h>
#include <vector>
#include <numeric>
#include <ispd/message/message.hpp>
#include <ispd/routing/routing.hpp>
#include <ispd/model/builder.hpp>
#include <ispd/metrics/metrics.hpp>
#include <ispd/metrics/user_metrics.hpp>
#include <ispd/services/VMM.hpp>
#include <ispd/configuration/virtual_machine.hpp>
#include <ispd/metrics/virtual_machine_metrics.hpp>

namespace ispd {

namespace services {
struct VM_state {
  ispd::metrics::virtual_machine_metrics metrics;
  ispd::configuration::VmConfiguration conf;
  std::vector<double> cores_free_time;
};

struct virtual_machine {

  static double least_core_time(const std::vector<double> &cores_free_time,
                                unsigned &core_index) {
    double candidate = std::numeric_limits<double>::max();
    unsigned candidate_index;

    for (unsigned i = 0; i < cores_free_time.size(); i++) {
      if (candidate > cores_free_time[i]) {
        candidate = cores_free_time[i];
        candidate_index = i;
      }
    }

    /// Update the core index with the least free time.
    core_index = candidate_index;

    /// Return the least core's free time.
    return candidate;
  }

  static void init(VM_state *s, tw_lp *lp) {
    // fetch the service initializer
    const auto &service_initializer =
        ispd::this_model::getServiceInitializer(lp->gid);

    service_initializer(s);

    s->metrics.m_ProcMFlops = 0;
    s->metrics.m_Proc_Tasks = 0;
    s->metrics.m_ProcTime += 0;
    s->metrics.m_ProcWaitingTime = 0;

    ispd_debug("Virtual machine %lu has been initialized.", lp->gid);
  }

  static void forward(VM_state *s, tw_bf *bf, ispd_message *msg, tw_lp *lp) {
    ispd_debug(
        "[Forward] Virtual Machine %lu received a message at %lf of type (%d) "
        "and route offset (%u).",
        lp->gid, tw_now(lp), msg->type, msg->route_offset);

    const double proc_size = msg->task.proc_size;
    const double proc_time = s->conf.timeToProcess(proc_size);
    unsigned core_index;
    const double least_free_time =
        least_core_time(s->cores_free_time, core_index);
    const double waiting_delay = ROSS_MAX(0.0, least_free_time - tw_now(lp));
    const double departure_delay = waiting_delay + proc_time;

    s->metrics.m_Proc_Tasks++;
    s->metrics.m_ProcMFlops += proc_size;
    s->metrics.m_ProcTime = proc_time;

    s->cores_free_time[core_index] = tw_now(lp) + departure_delay;

    // acknowledge message to VMM
    tw_event *const e = tw_event_new(msg->task.origin, departure_delay, lp);

    ispd_message *const m = static_cast<ispd_message *>(tw_event_data(e));
    *m = *msg;
    m->task = msg->task;
    m->type = message_type::ARRIVAL;
    m->task_processed = 1;
    m->saved_core_index = core_index;
    m->saved_core_next_available_time = least_free_time;
    m->previous_service_id = lp->gid;

    tw_event_send(e);
  }

  static void commit(VM_state *s, tw_bf *bf, ispd_message *msg, tw_lp *lp) {
    const double proc_size = msg->task.proc_size;
    const double proc_time = s->conf.timeToProcess(proc_size);

    const double least_free_time = msg->saved_core_next_available_time;
    const double waiting_delay = ROSS_MAX(0.0, least_free_time - tw_now(lp));

    ispd::metrics::UserMetrics &userMetrics =
        ispd::this_model::getUserById(msg->task.owner).getMetrics();

    userMetrics.m_ProcTime += proc_time;
    userMetrics.m_ProcWaitingTime += waiting_delay;
    userMetrics.m_CompletedTasks++;
  }
  static void reverse(VM_state *s, tw_bf *bf, ispd_message *msg, tw_lp *lp) {
    const double proc_size = msg->task.proc_size;
    const double proc_time = s->conf.timeToProcess(proc_size);

    const double least_free_time = msg->saved_core_next_available_time;
    const double waiting_delay = ROSS_MAX(0.0, least_free_time - tw_now(lp));

    s->metrics.m_Proc_Tasks--;
    s->metrics.m_ProcMFlops -= proc_size;
    s->metrics.m_ProcTime -= proc_time;

    s->cores_free_time[msg->saved_core_index] = least_free_time;
  }

  static void finish(VM_state *s, ispd_message *msg, tw_lp *lp) {
    const double last_activity_time = *std::max_element(
        s->cores_free_time.cbegin(), s->cores_free_time.cend());
    const double totalCpuTime = std::accumulate(s->cores_free_time.cbegin(),
                                                s->cores_free_time.cend(), 0.0);
    const double idleness =
        (totalCpuTime - s->metrics.m_ProcTime) / totalCpuTime;

    ispd::node_metrics::notifyMetric(
        ispd::metrics::NodeMetricsFlag::NODE_SIMULATION_TIME,
        last_activity_time);

    ispd::node_metrics::notifyMetric(
        ispd::metrics::NodeMetricsFlag::NODE_TOTAL_PROCESSED_MFLOPS,
        s->metrics.m_ProcMFlops);

    ispd::node_metrics::notifyMetric(
        ispd::metrics::NodeMetricsFlag::NODE_TOTAL_PROCESSING_WAITING_TIME,
        s->metrics.m_ProcWaitingTime);

    ispd::node_metrics::notifyMetric(
        ispd::metrics::NodeMetricsFlag::NODE_TOTAL_MACHINE_SERVICES);

    ispd::node_metrics::notifyMetric(
        ispd::metrics::NodeMetricsFlag::NODE_TOTAL_CPU_CORES,
        static_cast<unsigned>(s->cores_free_time.size()));
    ispd::node_metrics::notifyMetric(
        ispd::metrics::NodeMetricsFlag::NODE_TOTAL_PROCESSING_TIME,
        s->metrics.m_ProcTime);

    std::printf("Virtual machine metrics (%lu) \n"
                " - Last Activity Time..: %lf seconds (%lu).\n"
                " - Processed MFLOPS:...: %lf MFLOPS (%lu). \n"
                " - Processed Tasks.....: %u tasks (%lu).\n"
                " - Waiting Time........: %lf seconds (%lu).\n"
                " - Avg. Processing Time: %lf seconds (%lu).\n"
                " - Idleness............: %lf%% (%lu).\n",
                lp->gid, last_activity_time, lp->gid, s->metrics.m_ProcMFlops,
                lp->gid, s->metrics.m_Proc_Tasks, lp->gid,
                s->metrics.m_ProcTime, lp->gid,
                s->metrics.m_ProcTime / s->metrics.m_Proc_Tasks, lp->gid,
                idleness * 100, lp->gid);
  }
};
}; // namespace services
}; // namespace ispd

#endif
