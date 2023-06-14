#include <ispd/debug.h>
#include <ispd/log.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

/// \brief This is the file that will contain all the loggings.
static FILE *logfile = NULL;

void __ispd_log(enum log_level level, const char *filepath, const unsigned line, const char *fmt, ...)
{
#ifndef DEBUG_ON
	if(level == LOG_DEBUG)
		return;
#endif // DEBUG_ON
	va_list args;

	if(!logfile) {
		fprintf(stderr, "You are trying to log without set a log file. Use logfile_set function!\n");
		abort();
	}

	// fprintf(logfile, "%s:%u [%s] ", filepath, line, levels[level].name);
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

void logfile_set(FILE *f)
{
	logfile = f == NULL ? stdout : f;
}
