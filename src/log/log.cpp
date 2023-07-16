#include <fstream>
#include <ispd/log/log.hpp>
#include <cstdarg>

namespace ispd {
namespace log {

/// \brief The logging file in which the log will be stored.
FILE *logfile;

void __ispd_log(enum log_level level, const char *filepath, const unsigned line, const char *fmt, ...)
{
#ifndef DEBUG_ON
	if (level == LOG_DEBUG)
		return;
#endif // DEBUG_ON
	va_list args;

	if (!logfile) {
		fprintf(stderr, "You are trying to log without set a log file. Use set_log_file function!\n");
		abort();
	}

	fprintf(logfile, "%s%-5s\x1b[0m \x1b[90m%s:%u:\x1b[0m ", levels[level].color, levels[level].name, filepath,
	    line);

	va_start(args, fmt);
	vfprintf(logfile, fmt, args);
	va_end(args);
	fprintf(logfile, "\n");
	fflush(logfile);

	if(level == LOG_ERROR)
		abort();
}

void set_log_file(FILE *f) {
  logfile = f == NULL ? stdout : f;
}

}; // namespace log
}; // namespace ispd
