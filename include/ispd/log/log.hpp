#ifndef ISPD_LOG_HPP
#define ISPD_LOG_HPP

#define ispd_log(level, ...) ispd::log::__ispd_log(level, __FILE__, __LINE__, __VA_ARGS__);
#define ispd_error(...) ispd::log::__ispd_log(LOG_ERROR, __FILE__, __LINE__, __VA_ARGS__);
#define ispd_debug(...) ispd::log::__ispd_log(LOG_DEBUG, __FILE__, __LINE__, __VA_ARGS__);

enum log_level {
	LOG_DEBUG,
	LOG_INFO,
	LOG_ERROR,
};

static const struct {
	const char *name;
	const char *color;
} levels[] = {
  [LOG_DEBUG] = {.name = "DEBUG", .color = "\x1b[36m"},
  [LOG_INFO] = {.name = "INFO", .color = "\x1b[32m"},
  [LOG_ERROR] = {.name = "ERROR", .color = "\x1b[31m"}
};

namespace ispd {
namespace log {

void __ispd_log(enum log_level level, const char *filepath, const unsigned line, const char *fmt, ...);


void set_log_file(FILE *f);

}; // namespace log
}; // namespace ispd

#endif // ISPD_LOG_HPP
