
#ifndef DUCKYDD_MAIN_H
#define DUCKYDD_MAIN_H

#include <stdio.h>
#include <stdbool.h>
#include <time.h>

#include <xkbcommon/xkbcommon.h>

#include "io.h"
#include "mbuffer.h"

#define MAX_SIZE_EVENTS 40

bool brexit;
bool reloadconfig;
bool daemonize;
short loglvl;

struct udevInfo {
        int udevfd;
        struct udev *udev;
        struct udev_device *dev;
        struct udev_monitor *mon;
};

struct device {
        char openfd[MAX_SIZE_PATH];
        int fd;

        struct managedBuffer devlog;
        struct timespec time_added;

        struct xkb_state *state;

        int score;
};
#endif
