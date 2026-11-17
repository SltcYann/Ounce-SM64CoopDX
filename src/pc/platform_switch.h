#ifndef PLATFORM_SWITCH_H
#define PLATFORM_SWITCH_H

// Switch 2 (kaiHEN / libounce) glue. Kept apart from the game headers:
// libnx's u64/s64 typedefs clash with the game's.

void switch_init(void);
void switch_show_error(const char *msg);

#endif
