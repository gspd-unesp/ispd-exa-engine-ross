//
// Created by willao on 24/02/24.
//

#ifndef ISPD_EXA_ENGINE_ROSS_DYNAMIC_MASTER_HPP
#define ISPD_EXA_ENGINE_ROSS_DYNAMIC_MASTER_HPP
#include <ispd/services/master.hpp>

namespace ispd{

namespace services{

///\brief The DynamicMaster struct is responsible for static schedulers.
///
/// Dynamic schedulers are schedulers which the resource allocation is done
/// during the execution of tasks,allowing for adjustments and optimizations
/// based on real-time feedback and changing conditions


struct DynamicMaster{
  static void init(master_state *s, tw_lp *lp) {
    /// Fetch the service initializer from this logical process.
    const auto &service_initializer = ispd::this_model::getServiceInitializer(lp->gid);

    /// Call the service initializer for this logical process.
    service_initializer(s);

    /// Initialize the scheduler.
    s->scheduler->initScheduler(s->slaves);

    const uint32_t registered_routes_count = ispd::routing_table::countRoutes(lp->gid);

    /// Early sanity check if the routes has been registered correctly. If not,
    /// the program is immediately aborted.
    if (registered_routes_count != s->slaves.size())
      ispd_error("There are %u registered routes starting from master with GID %lu but there are %lu slaves.", registered_routes_count, lp->gid, s->slaves.size());

    /// Initialize the metrics.
    s->metrics.completed_tasks = 0;
    s->metrics.total_turnaround_time = 0;

    /// Checks if the specified workload has remaining tasks. If so, a generate message
    /// will be sent to the master itself to start generating the workload. Otherwise,
    /// no workload is generate at all, since at initialization it has been identified
    /// that the specified workload has no tasks.
    if (s->workload->getRemainingTasks() > 0) {
      double offset;

      s->workload->generateInterarrival(lp->rng, offset);

      /// Send a generate message to itself.
      tw_event *const e = tw_event_new(lp->gid, g_tw_lookahead + offset, lp);
      ispd_message *const m = static_cast<ispd_message *>(tw_event_data(e));

      m->type = message_type::GENERATE;

      tw_event_send(e);
    }

    /// Print a debug message.
    ispd_debug("Master %lu has been initialized.", lp->gid);
  }

  static void forward(master_state *s, tw_bf *bf, ispd_message *msg, tw_lp *lp) {
    ispd_debug("[Forward] Master %lu received a message at %lf of type (%d).", lp->gid, tw_now(lp), msg->type);

    switch (msg->type) {
    case message_type::GENERATE:
      generate(s, bf, msg, lp);
      break;
    case message_type::ARRIVAL:
      arrival(s, bf, msg, lp);
      break;
    default:
      std::cerr << "Unknown message type " << static_cast<int>(msg->type) << " at Master LP forward handler." << std::endl;
      abort();
      break;
    }

  }

  static void reverse(master_state *s, tw_bf *bf, ispd_message *msg, tw_lp *lp) {
    ispd_debug("[Reverse] Master %lu received a message at %lf of type (%d).", lp->gid, tw_now(lp), msg->type);

    switch (msg->type) {
    case message_type::GENERATE:
      generate_rc(s, bf, msg, lp);
      break;
    case message_type::ARRIVAL:
      arrival_rc(s, bf, msg, lp);
      break;
    default:
      std::cerr << "Unknown message type " << static_cast<int>(msg->type) << " at Master LP reverse handler." << std::endl;
      abort();
      break;
    }
  }

  static void commit(master_state *s, tw_bf *bf, ispd_message *msg, tw_lp *lp) {
    if (msg->type == message_type::GENERATE) {
      auto& userMetrics = ispd::this_model::getUserById(msg->task.m_Owner).getMetrics();

      /// Update the user's metrics.
      userMetrics.m_IssuedTasks++;
    }
  }

  static void finish(master_state *s, tw_lp *lp) {
    ispd::node_metrics::notifyMetric(ispd::metrics::NodeMetricsFlag::NODE_TOTAL_COMPLETED_TASKS, s->metrics.completed_tasks);
    ispd::node_metrics::notifyMetric(ispd::metrics::NodeMetricsFlag::NODE_TOTAL_MASTER_SERVICES);
    ispd::node_metrics::notifyMetric(ispd::metrics::NodeMetricsFlag::NODE_TOTAL_TURNAROUND_TIME, s->metrics.total_turnaround_time);

    const double avgTurnaroundTime = s->metrics.total_turnaround_time / s->metrics.completed_tasks;

    /// Report to the node's metrics reports file this master's metrics.
    ispd::node_metrics::notifyReport(s->metrics, lp->gid);

    std::printf(
        "Master Metrics (%lu)\n"
        " - Completed Tasks.....: %u tasks (%lu).\n"
        " - Avg. Turnaround Time: %lf seconds (%lu).\n"
        "\n",
        lp->gid,
        s->metrics.completed_tasks, lp->gid,
        avgTurnaroundTime, lp->gid
    );
  }

private:
  static void generate(master_state *s, tw_bf *bf, ispd_message *msg, tw_lp *lp) {
    ispd_debug("Master %lu will generate a task at %lf, remaining %u.", lp->gid, tw_now(lp), s->workload->getRemainingTasks());

#ifdef DEBUG_ON
    const auto start = std::chrono::high_resolution_clock::now();
#endif // DEBUG_ON

    if (s->workload->getRemainingTasks() > 0){
      const tw_lpid scheduled_slave_id = s->scheduler->forwardSchedule(s->slaves,bf,msg,lp);

      const ispd::routing::Route *route = ispd::routing_table::getRoute(lp->gid, scheduled_slave_id);

      tw_event *const e = tw_event_new(route->get(0), g_tw_lookahead, lp);
      ispd_message *const m = static_cast<ispd_message *>(tw_event_data(e));

      m->type = message_type::ARRIVAL;

      s->workload->generateWorkload(lp->rng, m->task.m_ProcSize, m->task.m_CommSize);

      m->task.m_Offload = s->workload->getComputingOffload();

      /// Task information specification.
      m->task.m_Origin = lp->gid;
      m->task.m_Dest = scheduled_slave_id;
      m->task.m_SubmitTime = tw_now(lp);
      m->task.m_Owner = s->workload->getOwner();

      m->route_offset = 1;
      m->previous_service_id = lp->gid;
      m->downward_direction = 1;
      m->task_processed = 0;
      m->service_id = 0;
      tw_event_send(e);


    }


#ifdef DEBUG_ON
    const auto end = std::chrono::high_resolution_clock::now();
    const auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    const auto timeTaken = static_cast<double>(duration.count());

    ispd::node_metrics::notifyMetric(ispd::metrics::NodeMetricsFlag::NODE_MASTER_FORWARD_TIME, timeTaken);
#endif // DEBUG_ON
  }

  static void generate_rc(master_state *s, tw_bf *bf, ispd_message *msg, tw_lp *lp) {
#ifdef DEBUG_ON
    const auto start = std::chrono::high_resolution_clock::now();
#endif // DEBUG_ON
    s->scheduler->reverseSchedule(s->slaves, bf, msg, lp);

    s->workload->reverseGenerateWorkload(lp->rng);

    if(s->workload->getRemainingTasks() > 0)
    {
      s->workload->reverseGenerateWorkload(lp->rng);
    }


#ifdef DEBUG_ON
    const auto end = std::chrono::high_resolution_clock::now();
    const auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    const auto timeTaken = static_cast<double>(duration.count());

    ispd::node_metrics::notifyMetric(ispd::metrics::NodeMetricsFlag::NODE_MASTER_REVERSE_TIME, timeTaken);
#endif // DEBUG_ON
  }

  static void arrival(master_state *s, tw_bf *bf, ispd_message *msg, tw_lp *lp) {
    msg->task.m_EndTime = tw_now(lp);

    const double turnaround_time = msg->task.m_EndTime - msg->task.m_SubmitTime;

    s->metrics.completed_tasks++;
    s->metrics.total_turnaround_time += turnaround_time;

    /// if there are tasks to execute, send the feedback for master.
    if(s->workload->getRemainingTasks() > 0){
      double offset;

      s->workload->generateInterarrival(lp->rng, offset);
      /// Send a generate message to itself.
      tw_event *const e = tw_event_new(lp->gid, g_tw_lookahead + offset, lp);
      ispd_message *const m = static_cast<ispd_message *>(tw_event_data(e));
      m->service_id = msg->service_id;
      m->type = message_type::GENERATE;
      ispd_debug("Returned machine: %lu", msg->service_id);

      tw_event_send(e);
    }

  }

  static void arrival_rc(master_state *s, tw_bf *bf, ispd_message *msg, tw_lp *lp) {
    ispd_debug("Arrival reverse");
    /// Calculate the task`s turnaround time.
    const double turnaround_time = msg->task.m_EndTime - msg->task.m_SubmitTime;

    /// Reverse the master's metrics.
    s->metrics.completed_tasks--;
    s->metrics.total_turnaround_time -= turnaround_time;

  }

};


};

};

#endif // ISPD_EXA_ENGINE_ROSS_DYNAMIC_MASTER_HPP
