#ifndef PLATFORM_SWITCH_H
#define PLATFORM_SWITCH_H

// Switch 2 (kaiHEN / libounce) glue. Kept apart from the game headers:
// libnx's u64/s64 typedefs clash with the game's.

// Everything the game reads and writes lives here on the SD card, next to the NRO.
// libnx makes the SD card the default device, so plain absolute paths work.
#define SWITCH_DATA_DIR "/switch/sm64coopdx"

// Creates SWITCH_DATA_DIR, opens the log, starts sockets. First thing in main().
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
// The display's refresh rate: what the window presents at with vsync on,
// otherwise 120 in handheld mode and 60 docked.
unsigned int switch_display_refresh_rate(void);
// Rumble through libounce's HID directly: SDL's Switch joystick driver
// reports no rumble capability, so the game never asked it to vibrate.
void switch_rumble_play(float strength, float seconds);
void switch_rumble_stop(void);
// Stops the rumble once its duration has passed. Call it every frame.
void switch_rumble_update(void);

#endif
