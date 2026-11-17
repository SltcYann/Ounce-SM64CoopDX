#ifdef TARGET_SWITCH

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <switch.h>

#include "platform_switch.h"

#define SWITCH_DATA_DIR "/switch/sm64coopdx"

static void switch_shutdown(void) {
    socketExit();
}

void switch_init(void) {
    // sockets for online play
    socketInitializeDefault();
    // stdout/stderr go to the Mac when `nxlink -s` listens; harmless otherwise
    nxlinkStdio();
    atexit(switch_shutdown);

    mkdir("/switch", 0777);
    mkdir(SWITCH_DATA_DIR, 0777);
    chdir(SWITCH_DATA_DIR);
}

void switch_show_error(const char *msg) {
    ErrorApplicationConfig c;
    if (R_SUCCEEDED(errorApplicationCreate(&c, "sm64coopdx: fatal error", msg))) {
        errorApplicationShow(&c);
    }
}

#endif
