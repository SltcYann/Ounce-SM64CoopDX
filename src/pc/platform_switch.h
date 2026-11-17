#ifndef PLATFORM_SWITCH_H
#define PLATFORM_SWITCH_H

// Switch 2 (kaiHEN / libounce) glue. Kept apart from the game headers:
// libnx's u64/s64 typedefs clash with the game's.

void switch_init(void);
void switch_show_error(const char *msg);
// one line in /switch/sm64coopdx/switch-log.txt, flushed right away
void switch_log_step(const char *step);
// process memory and heap use, to spot running out of memory
void switch_log_memory(void);
// make printf unbuffered on the calling thread, so nothing is lost if the process dies
void switch_thread_log_init(void);
// Vsync at the display's own refresh rate (120 Hz on the handheld screen,
// with VRR) instead of libounce's swap interval, which always means N/60 s.
void switch_set_vsync(int enabled);
// The display's current refresh rate as the window presents at it: 120 or 60.
unsigned int switch_display_refresh_rate(void);

#endif
