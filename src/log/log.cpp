#include <fstream>
#include <ispd/log/log.hpp>
#include <cstdarg>

namespace ispd::log {

/// \brief The logging file in which the log will be stored.
FILE *logfile;

/// \brief The log function is used to log messages at different log levels.
///
/// This function logs messages with the specified log level, along with the
/// source file location and additional formatted message content. The log
/// messages are written to the output log file, which should be set using the
/// setOutputFile function before calling this function.
///
/// \param level The log level of the message. Can be LOG_DEBUG, LOG_INFO, or
/// LOG_ERROR. \param filepath The path of the source file where the log message
/// is generated. \param line The line number in the source file where the log
/// message is generated. \param fmt The format string for the log message,
/// similar to printf-style formatting. \param ... Additional arguments to be
/// formatted according to the format string.
///
/// \note For DEBUG_ON compilation, if the log level is LOG_DEBUG and DEBUG_ON
/// is not defined, the log message will not be generated.
///
/// \note If the log file has not been set using the setOutputFile function, an
/// error message will be printed to stderr and the program will be aborted.
///
/// \note After logging the message, the log file will be flushed to ensure the
/// message is written immediately.
///
/// \note If the log level is LOG_ERROR, the program will be aborted after
/// logging the message.
///
/// \note The function uses variadic arguments for formatting the log message
/// content.
void log(const LogLevel level, std::string_view filepath, const unsigned line,
         const std::string_view fmt, ...) noexcept {
#ifndef DEBUG_ON
  // Check if the log level is LOG_DEBUG and DEBUG_ON is not defined, in which
  // case, return.
  if (level == LogLevel::LOG_DEBUG)
    return;
#endif // DEBUG_ON

  va_list args; // Variable argument list.
  const auto integerLevelValue =
      static_cast<int>(level); // Convert LogLevel to integer.

  // Check if the log file has been set.
  if (!logfile) {
    fprintf(stderr, "You are trying to log without setting a log file. Use the "
                    "set_log_file function!\n");
    abort(); // Abort the program.
  }

  // Write log message header with log level, source file location, and line
  // number.
  fprintf(logfile, "%s%-5s\x1b[0m \x1b[90m%s:%u:\x1b[0m ",
          levels[integerLevelValue].color, levels[integerLevelValue].name,
          filepath.data(), line);

  va_start(args, fmt);                 // Initialize variable argument list.
  vfprintf(logfile, fmt.data(), args); // Format and write additional arguments.
  va_end(args);                        // End variable argument processing.
  fprintf(logfile, "\n");              // Add a newline to the log message.
  fflush(logfile); // Flush the log file to ensure immediate writing.

  // If the log level is LOG_ERROR, abort the program after logging the message.
  if (level == LogLevel::LOG_ERROR)
    abort();
}

/// \brief Sets the output log file for logging messages.
///
/// This function allows you to specify the output log file to which log
/// messages will be written. The provided file pointer will be used as the log
/// file for all subsequent log messages. If the provided file pointer is
/// `nullptr`, the log messages will be directed to the standard output (stdout)
/// by default.
///
/// \param f A pointer to the file where log messages should be written. Can be
/// nullptr to
///          use the standard output.
///
/// \note The setOutputFile function provides flexibility in directing log
/// messages to a specific file or the standard output.
///
/// \note If you want to log messages to a custom file, call this function
/// before using the log function to ensure that the log messages are written to
/// the desired file.
///
/// \note If the provided file pointer is nullptr, log messages will be written
/// to the standard output.
void setOutputFile(FILE *const f) noexcept {
  // If the provided file pointer is nullptr, set the log file to the standard
  // output.
  logfile = f == nullptr ? stdout : f;
}

} // namespace ispd::log
