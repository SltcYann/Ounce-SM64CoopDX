#ifdef TARGET_SWITCH

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/iosupport.h>
#include <switch.h>
#include <switch2/memory/heap.h>
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

// The log goes straight to a file descriptor and is synced after every write,
// because when the game dies the whole process is killed and buffers are lost.
static int sLogFd = -1;
static Mutex sLogMutex;

static void switch_log_write(const char *ptr, size_t len) {
    if (sLogFd < 0 || len == 0) { return; }
    mutexLock(&sLogMutex);
    write(sLogFd, ptr, len);
    fsync(sLogFd);
    mutexUnlock(&sLogMutex);
}

// for the crash handler: the crashing thread may already hold the lock
static void switch_log_raw_nolock(const char *line) {
    if (sLogFd < 0) { return; }
    write(sLogFd, line, strlen(line));
    write(sLogFd, "\n", 1);
    fsync(sLogFd);
}

static void switch_log_raw(const char *line) {
    char buf[600];
    int n = snprintf(buf, sizeof(buf), "%s\n", line);
    if (n > (int)sizeof(buf) - 1) { n = sizeof(buf) - 1; }
    if (n > 0) { switch_log_write(buf, (size_t)n); }
}

// stdout and stderr of every thread (printf, the game's LOG_* macros) end up
// here: this is how libnx's own console and nxlink redirect them.
static ssize_t switch_log_devoptab_write(struct _reent *r, void *fd, const char *ptr, size_t len) {
    (void)r; (void)fd;
    switch_log_write(ptr, len);
    return (ssize_t)len;
}

static const devoptab_t sLogDevoptab = {
    .name = "switchlog",
    .write_r = switch_log_devoptab_write,
};

void switch_thread_log_init(void) {
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);
}

void switch_log_step(const char *step) {
    char line[256];
    snprintf(line, sizeof(line), "[step %.3fs] %s", (double)armTicksToNs(armGetSystemTick()) / 1e9, step);
    switch_log_raw(line);
}

void switch_log_memory(void) {
    u64 total = 0, used = 0;
    svcGetInfo(&total, InfoType_TotalMemorySize, CUR_PROCESS_HANDLE, 0);
    svcGetInfo(&used, InfoType_UsedMemorySize, CUR_PROCESS_HANDLE, 0);
    char line[256];
    snprintf(line, sizeof(line),
        "  memory: process %lu/%lu MB used, malloc heap %lu MB with at least %lu MB free (loader override: %s, %lu MB)",
        (unsigned long)(used >> 20), (unsigned long)(total >> 20),
        (unsigned long)(heapGetTotalSize() >> 20), (unsigned long)(heapGetFreeSize() >> 20),
        envHasHeapOverride() ? "yes" : "no", (unsigned long)(envGetHeapOverrideSize() >> 20));
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
    switch_log_raw_nolock(line);
    snprintf(line, sizeof(line), "CRASH: sp=0x%lx far=0x%lx module_base=0x%lx",
        (unsigned long)ctx->sp.x, (unsigned long)ctx->far.x, (unsigned long)base);
    switch_log_raw_nolock(line);
    for (int i = 0; i < 29; i += 4) {
        snprintf(line, sizeof(line), "CRASH: x%d=0x%lx x%d=0x%lx x%d=0x%lx x%d=0x%lx",
            i, (unsigned long)ctx->cpu_gprs[i].x,
            i + 1, i + 1 < 29 ? (unsigned long)ctx->cpu_gprs[i + 1].x : 0UL,
            i + 2, i + 2 < 29 ? (unsigned long)ctx->cpu_gprs[i + 2].x : 0UL,
            i + 3, i + 3 < 29 ? (unsigned long)ctx->cpu_gprs[i + 3].x : 0UL);
        switch_log_raw_nolock(line);
    }
}

// Writes a line every 250 ms, so the log shows exactly when the process dies
// and whether it was still alive while the main thread was busy.
static Thread sHeartbeatThread;
static u64 sStartTick = 0;

static void switch_heartbeat(void *arg) {
    (void)arg;
    for (;;) {
        svcSleepThread(250000000ULL);
        char line[64];
        snprintf(line, sizeof(line), "  heartbeat %.2f s", (double)armTicksToNs(armGetSystemTick() - sStartTick) / 1e9);
        switch_log_raw(line);
    }
}

void switch_init(void) {
    sStartTick = armGetSystemTick();
    mkdir("/switch", 0777);
    mkdir(SWITCH_DATA_DIR, 0777);
    chdir(SWITCH_DATA_DIR);

    mutexInit(&sLogMutex);
    sLogFd = open(SWITCH_LOG_PATH, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (sLogFd >= 0) {
        devoptab_list[STD_OUT] = &sLogDevoptab;
        devoptab_list[STD_ERR] = &sLogDevoptab;
        switch_thread_log_init();
    }
    switch_log_step("sm64coopdx Switch 2 build starting");
    switch_log_memory();

    Result rc = socketInitializeDefault();
    char line[96];
    snprintf(line, sizeof(line), "socketInitializeDefault: 0x%x", rc);
    switch_log_step(line);

    if (R_SUCCEEDED(threadCreate(&sHeartbeatThread, switch_heartbeat, NULL, NULL, 0x4000, 0x2C, -2))) {
        threadStart(&sHeartbeatThread);
    }

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
