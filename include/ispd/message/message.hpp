#ifndef ISPD_MESSAGE_H
#define ISPD_MESSAGE_H

enum class message_type {
  GENERATE,
  ARRIVAL
};

struct ispd_message {
  /// \brief The message type.
  message_type type;
};

#endif // ISPD_MESSAGE_H
