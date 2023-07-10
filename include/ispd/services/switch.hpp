

#ifndef ISPD_SERVICE_SWITCH_HPP
#define ISPD_SERVICE_SWITCH_HPP

#include <ross.h>
#include <ispd/debug/debug.hpp>
#include <ispd/message/message.hpp>
#include <ispd/routing/routing.hpp>

namespace ispd {
    namespace services {

        struct switch_metrics {
            double comm_mbits;
            unsigned comm_packets;
        };

        struct switch_configuration{
            double bandwidth;
            double latency;
            double load_factor;
        };

        struct switch_state{
            switch_configuration conf;
            switch_metrics metrics;

            // Switch's queuing model
            double upward_next_avaliable_time;
            double downward_next_available_time;



        };

// the struct's name has to be in PascalCase due to the fact the word "switch" is a keyword in C/C++, therefor it cannot be used.
        struct Switch{

            static double time_to_communicate(const switch_configuration *const conf, const double comm_size)
            {
                return conf->latency + comm_size / (1.0 - conf->load_factor) * conf->bandwidth;
            }

            static void init(switch_state *s, tw_lp *lp)
            {
                // @TODO: Initialize the switch configuration dynamically using a model builder

                // @Temporary
                s->conf.bandwidth = 50;
                s->conf.load_factor = 0.0;
                s->conf.latency = 2;

                // initialize the metrics
                s->metrics.comm_mbits = 0;
                s->metrics.comm_packets = 0;

                // initialize switch's model information
                s->upward_next_avaliable_time = 0;
                s->downward_next_available_time = 0;

            }

            static void forward(switch_state *s, tw_bf *bf, ispd_message *msg, tw_lp *lp)
            {
                const double comm_size = msg->task.comm_size;
                const double comm_time = time_to_communicate(&s->conf, comm_size);

                /* Similar to the link scheme.
                 *
                 * Here is selected which avaliable time should be used. If the message is being sent from the master
                 * to the slave, then the downward link is being us. Otherwise, if the slave is sending the results
                 * to master, then the upward link is being used.
                 */
                double next_avaliable_time;

                if(msg->downward_direction)
                    next_avaliable_time = s->downward_next_available_time;
                else
                    next_avaliable_time = s->upward_next_avaliable_time;

                // calculate the waiting delay and the departure delay
                const double waiting_delay = ROSS_MAX(0.0, next_avaliable_time - tw_now(lp));
                const double departure_delay = waiting_delay + comm_time;

                // update the switch's metrics
                s->metrics.comm_packets++;
                s->metrics.comm_mbits += comm_size;

                next_avaliable_time = tw_now(lp) + departure_delay;

                //updates the switch's model information
                if (msg->downward_direction)
                    s->downward_next_available_time = next_avaliable_time;
                else
                    s->upward_next_avaliable_time = next_avaliable_time;

                // fowards the package to its link
                const ispd::routing::route *route  = g_routing_table.get_route(msg->task.origin, msg->task.dest);

                tw_event *const e = tw_event_new(msg->previous_service_id, 0.0, lp );
                ispd_message *const m = static_cast<ispd_message *>(tw_event_data(e));

                m->type = message_type::ARRIVAL;
                m->task = msg->task; //copies the task information
                m->task_processed = msg->task_processed;
                m->downward_direction = msg->downward_direction;
                m->route_offset += msg->downward_direction ? 1 : -1;
                m->previous_service_id = lp->gid;

                tw_event_send(e);
            }

            static void reverse(switch_state *s, tw_bf *bf, ispd_message *msg, tw_lp *lp)
            {
                const double comm_size = msg->task.comm_size;
                const double comm_time = time_to_communicate(&s->conf, comm_size);

                // reverse the process in the case of a downward message or upward message
                if(msg->downward_direction)
                    s->downward_next_available_time = msg->saved_link_next_available_time;
                else
                    s->upward_next_avaliable_time = msg->saved_link_next_available_time;

                // reverse the metrics
                s->metrics.comm_packets--;
                s->metrics.comm_mbits -= comm_size;
            }

            static void finish(switch_state *s, tw_lp *lp)
            {
                DEBUG({
                              std::printf(
                                      "Switch Queue Info & Metrics (%lu) \n"
                                      " - Communicated Mbits.......: %lf Mbits (%lu).\n"
                                      " - Communicated Packets.....: %u packets (%lu).\n"
                                      " - Downward Next Avail. Time: %lf seconds (%lu).\n"
                                      " - Upward Next Avail. Time..: %lf seconds (%lu).\n"
                                      "\n",
                                      lp->gid,
                                      s->metrics.comm_mbits, lp->gid,
                                      s->metrics.comm_packets,lp->gid,
                                      s->downward_next_available_time,lp->gid,
                                      s->upward_next_avaliable_time,lp->gid
                              );
                      });
            }


        };


    } // namespace services
} // namespace ispd

#endif // ISPD_SERVICE_SWITCH_HPP