#ifndef DUCKYDD_IO_H
#define DUCKYDD_IO_H

#include <inttypes.h>
#include <stdbool.h>
#include <time.h>
#include <limits.h>
#include <errno.h>

#include "mbuffer.h"
#include "vars.h"

#define LOG(loglvl, format, args...)                                           \
	_logger(loglvl, __func__, __LINE__, format,                            \
		##args) // print function name

#define STOP(function)                                                         \
	_logger(-1, __func__, __LINE__,                                        \
		"%s has failed (%d) -> %s (err: %d)\n", function, errno,       \
		strerror(errno));                                              \
	exit(EXIT_FAILURE)

#define ERR(function)                                                          \
	_logger(-1, __func__, __LINE__,                                        \
		"%s has failed (%d) -> %s (err: %d)\n", function, errno,       \
		strerror(errno));

// holds data read from the config file (mainly used by readconfig)
struct configInfo {
	long int maxcount;

	char logpath[PATH_MAX];

	// kbd
	bool xkeymaps;

	struct timespec minavrg;
};

// holds data that was parsed out by handleargs
struct argInfo {
	char configpath[PATH_MAX];
};

void readconfig(const char path[], struct configInfo *data);
void handleargs(int argc, char *argv[], struct argInfo *data);

// internal logger function
char *binexpand(uint8_t bin, size_t size);
void _logger(short loglevel, const char func[], const int line,
	     const char format[], ...);

const char *find_file(const char *input);
#endif
