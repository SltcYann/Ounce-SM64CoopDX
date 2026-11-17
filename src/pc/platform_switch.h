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

#endif
