#pragma once

#include <cstdio>
#include <string_view>
#include <ispd/debug/debug.hpp>

// The ispd_log macro provides a concise way to log messages with different log levels.
#define ispd_log(level, ...) \
  ispd::log::log(level, __FILE__, __LINE__, __VA_ARGS__);

// The ispd_info macro logs messages at the INFO log level.
#define ispd_info(...) \
  ispd::log::log(ispd::log::LogLevel::LOG_INFO, __FILE__, __LINE__, __VA_ARGS__);

// The ispd_error macro logs messages at the ERROR log level.
#define ispd_error(...) \
  ispd::log::log(ispd::log::LogLevel::LOG_ERROR, __FILE__, __LINE__, __VA_ARGS__);

// The ispd_debug macro logs messages at the DEBUG log level, but only when DEBUG_ON is defined.
#ifdef DEBUG_ON
#define ispd_debug(...) \
  ispd::log::log(ispd::log::LogLevel::LOG_DEBUG, __FILE__, __LINE__, __VA_ARGS__);
#else
#define ispd_debug(...)
#endif // DEBUG_ON

/// The ispd::log namespace contains utilities for logging messages.
namespace ispd::log {

/// The LogLevel enum class represents different log levels.
enum class LogLevel {
  LOG_DEBUG, ///< Debug log level.
  LOG_INFO,  ///< Info log level.
  LOG_ERROR, ///< Error log level.
};

/// The levels array contains information about log levels and their associated colors.
constexpr static const struct {
  const char *name;  ///< Name of the log level.
  const char *color; ///< Associated color code for formatting.
} levels[] = {[static_cast<int>(LogLevel::LOG_DEBUG)] = {.name = "DEBUG",
                                                         .color = "\x1b[36m"},
              [static_cast<int>(LogLevel::LOG_INFO)] = {.name = "INFO",
                                                        .color = "\x1b[32m"},
              [static_cast<int>(LogLevel::LOG_ERROR)] = {.name = "ERROR",
                                                         .color = "\x1b[31m"}};

/// The log function is used to log messages at different log levels.
///
/// \param level The log level of the message.
/// \param filepath The path of the source file where the log message is generated.
/// \param line The line number in the source file where the log message is generated.
/// \param fmt The format string for the log message.
/// \param ... Additional arguments to be formatted according to the format string.
void log(const LogLevel level, std::string_view filepath, const unsigned line, const std::string_view fmt, ...) noexcept;

/// The setOutputFile function is used to set the output file for log messages.
///
/// \param f A pointer to the output file where log messages will be written.
void setOutputFile(FILE *const f) noexcept;

} // namespace ispd::log
