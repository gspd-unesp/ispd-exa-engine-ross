#ifndef LOG_H
#define LOG_H

#include <stdio.h>

enum log_level {
	LOG_DEBUG,
	LOG_INFO,
	LOG_ERROR,
};

static const struct {
	const char *name;
	const char *color;
} levels[] = {[LOG_DEBUG] = {.name = "DEBUG", .color = "\x1b[36m"},
    [LOG_INFO] = {.name = "INFO", .color = "\x1b[32m"},
    [LOG_ERROR] = {.name = "ERROR", .color = "\x1b[31m"}};

extern void __ispd_log(enum log_level level, const char *filepath, const unsigned line, const char *fmt, ...);
extern void __ispd_abort(const char *filepath, const unsigned ine, const char *fmt, ...);

#define ispd_log(level, ...) __ispd_log(level, __FILE__, __LINE__, __VA_ARGS__);
#define ispd_error(...) __ispd_log(LOG_ERROR, __FILE__, __LINE__, __VA_ARGS__);

extern void logfile_set(FILE *f);

#endif // LOG_H
