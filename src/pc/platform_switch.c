#ifdef TARGET_SWITCH

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <switch.h>
#include <SDL2/SDL.h>

#include "platform_switch.h"

#define SWITCH_DATA_DIR "/switch/sm64coopdx"
#define SWITCH_LOG_PATH SWITCH_DATA_DIR "/switch-log.txt"

// Start of this module, from libnx's crt0. Crash addresses are printed
// relative to it so they can be looked up in sm64coopdx.elf.
extern char __start__[];

// libnx calls __libnx_exception_handler on this stack when the game crashes.
alignas(16) u8 __nx_exception_stack[0x4000];
u64 __nx_exception_stack_size = sizeof(__nx_exception_stack);

static FILE *sLog = NULL;

static void switch_log_raw(const char *line) {
    if (!sLog) { return; }
    fputs(line, sLog);
    fputc('\n', sLog);
    fflush(sLog);
    fsync(fileno(sLog));
}

void switch_log_step(const char *step) {
    char line[256];
    snprintf(line, sizeof(line), "[step %.3fs] %s", (double)armTicksToNs(armGetSystemTick()) / 1e9, step);
    switch_log_raw(line);
}

static void switch_shutdown(void) {
    switch_log_step("exit: shutting down SDL and sockets");
    // the game process keeps running hbmenu after us, so give back the
    // window, GL context, audio and controllers before leaving
    SDL_Quit();
    socketExit();
    switch_log_step("exit: done");
}

void __libnx_exception_handler(ThreadExceptionDump *ctx) {
    char line[256];
    u64 base = (u64)__start__;
    snprintf(line, sizeof(line), "CRASH: error_desc=0x%x pc=0x%lx (elf+0x%lx) lr=0x%lx (elf+0x%lx)",
        ctx->error_desc,
        (unsigned long)ctx->pc.x, (unsigned long)(ctx->pc.x - base),
        (unsigned long)ctx->lr.x, (unsigned long)(ctx->lr.x - base));
    switch_log_raw(line);
    snprintf(line, sizeof(line), "CRASH: sp=0x%lx far=0x%lx module_base=0x%lx",
        (unsigned long)ctx->sp.x, (unsigned long)ctx->far.x, (unsigned long)base);
    switch_log_raw(line);
    for (int i = 0; i < 29; i += 4) {
        snprintf(line, sizeof(line), "CRASH: x%d=0x%lx x%d=0x%lx x%d=0x%lx x%d=0x%lx",
            i, (unsigned long)ctx->cpu_gprs[i].x,
            i + 1, i + 1 < 29 ? (unsigned long)ctx->cpu_gprs[i + 1].x : 0UL,
            i + 2, i + 2 < 29 ? (unsigned long)ctx->cpu_gprs[i + 2].x : 0UL,
            i + 3, i + 3 < 29 ? (unsigned long)ctx->cpu_gprs[i + 3].x : 0UL);
        switch_log_raw(line);
    }
}

void switch_init(void) {
    mkdir("/switch", 0777);
    mkdir(SWITCH_DATA_DIR, 0777);
    chdir(SWITCH_DATA_DIR);

    // stdout and stderr (the game's LOG_* output) go to a file on the SD card
    sLog = fopen(SWITCH_LOG_PATH, "w");
    if (sLog) {
        setvbuf(sLog, NULL, _IONBF, 0);
        freopen(SWITCH_LOG_PATH, "a", stdout);
        freopen(SWITCH_LOG_PATH, "a", stderr);
        setvbuf(stdout, NULL, _IOLBF, 0);
        setvbuf(stderr, NULL, _IONBF, 0);
    }
    switch_log_step("sm64coopdx Switch 2 build starting");

    Result rc = socketInitializeDefault();
    char line[96];
    snprintf(line, sizeof(line), "socketInitializeDefault: 0x%x", rc);
    switch_log_step(line);

    atexit(switch_shutdown);
}

void switch_show_error(const char *msg) {
    char line[512];
    snprintf(line, sizeof(line), "FATAL: %s", msg);
    switch_log_raw(line);

    ErrorApplicationConfig c;
    if (R_SUCCEEDED(errorApplicationCreate(&c, "sm64coopdx: fatal error", msg))) {
        errorApplicationShow(&c);
    }
}

#endif
