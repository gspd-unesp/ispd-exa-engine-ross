#ifndef ISPD_SERVICE_DUMMY_HPP
#define ISPD_SERVICE_DUMMY_HPP

#include <ispd/debug/debug.hpp>
#include <ispd/message/message.hpp>

namespace ispd {
namespace services {

struct dummy_state {
  /// \brief A count of how many events have been forward
  ///        event handled.
  unsigned forward_event_count;

  /// \brief A count of how many events have been reverse
  ///        event handled.
  unsigned reverse_event_count;
};

/// \brief A dummy service. Basically, it is used for
///        debugging purposes.
struct dummy {

  static void init(dummy_state *s, tw_lp *lp) {
    /// @Note: The dummy does not need to be initialized dynamicaly
    ///        by the model builder.

    /// Initialize the dummy state.
    s->forward_event_count = 0;
    s->reverse_event_count = 0;

#ifdef DUMMY_SEND_BATCH_TO_ITSELF
    tw_event *e;
    ispd_message *m;

    /// Mean exponential distribution.
    const double mean = 0.1;

    /// Schedule 5 events to itself.
    const unsigned count = 5;

    for (int i = 0; i < count; i++) {
      e = tw_event_new(lp->gid, g_tw_lookahead + tw_rand_exponential(lp->rng, mean), lp);
      m = static_cast<ispd_message *>(tw_event_data(e));

      /// Add some information to the message.
      m->type = message_type::GENERATE;

      /// Send the message to itself.
      tw_event_send(e);
    }
#endif // DUMMY_SEND_BATCH_TO_ITSELF

    /// Print a debug message to the standard output.
    DEBUG({
      std::cout << "Dummy with GID " << lp->gid << " has been initialized."
                << std::endl;
    });
  }

  static void forward(dummy_state *s, tw_bf *bf, ispd_message *msg, tw_lp *lp) {
    /// Update the dummy metrics.
    s->forward_event_count++;

    /// Print a debug message to the standard output.
    DEBUG({
      std::cout << "Dummy with GID " << lp->gid << " at " << tw_now(lp)
                << " has forward processed an event." << std::endl;
    });
  }

  static void reverse(dummy_state *s, tw_bf *bf, ispd_message *msg, tw_lp *lp) {
    /// Update the dummy metrics.
    s->reverse_event_count++;

    /// Print a debug message to the standard output.
    DEBUG({
      std::cout << "Dummy with GID " << lp->gid
                << " has reverse processed an event." << std::endl;
    });
  }

  static void finish(dummy_state *s, tw_lp *lp) {
    /// Print a debug message to the standard output.
    DEBUG({
      std::cout << "Dummy with GID " << lp->gid
                << " (F: " << s->forward_event_count
                << ", R: " << s->reverse_event_count << ")." << std::endl;
    });
  }
};

}; // namespace services
}; // namespace ispd

#endif // ISPD_SERVICE_DUMMY_HPP
