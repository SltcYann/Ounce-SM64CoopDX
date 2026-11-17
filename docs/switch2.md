# Switch 2 port (kaiHEN)

This fork builds sm64coopdx as a `.nro` for the Switch 2, to run from hbmenu under [kaiHEN](https://github.com/ouncebrew/kaihen).

It runs. On a Switch 2 under kaiHEN it loads, reaches the menu and plays at full speed (the game logic runs at 30 frames per second, as on N64). The software keyboard works in text fields. Sound and online play haven't been tried yet.

## Installing

1. Get `sm64coopdx.nro` and the `lang`, `mods`, `dynos` and `palettes` folders (from a build, see below).
2. Put them all in `/switch/sm64coopdx/` on the SD card.
3. Copy your own Super Mario 64 (USA) ROM, as a `.z64` file, into that same folder. The file name doesn't matter. The game checks the MD5 and only accepts the US version.
4. Start a game while holding R to get into hbmenu, then launch sm64coopdx.

The ROM is not part of the build. The game reads it on first launch, like the PC version does.

## Building

You need devkitPro with `switch-dev`, `switch-sdl2`, `switch-curl` and `switch-zlib`, plus [ounce-devkit](https://github.com/ouncebrew/ounce-devkit) installed on top of it. On macOS you also need GNU make 4 (`brew install make`), because the Makefile uses `!=`, which Apple's make 3.81 ignores without telling you.

```sh
sudo dkp-pacman -S --needed switch-dev switch-sdl2 switch-curl switch-zlib
sh tools/build_lua_switch.sh     # once, builds lib/lua/switch/liblua53.a
DEVKITPRO=/opt/devkitpro gmake TARGET_SWITCH=1 -j8
```

The output is `build/us_pc/sm64coopdx.nro`. Use `make` instead of `gmake` on Linux.

## What changed for the Switch

- `TARGET_SWITCH=1` in the Makefile: devkitA64 compiler, libounce instead of libnx, SDL2 from the Switch portlibs, and an OpenGL ES 3 context through OunceVK's EGL (the shaders are still GLSL ES 1.00). It wraps the ELF into a `.nro` with an icon and a NACP.
- Discord, CoopNet and the updater are off. They ship as prebuilt PC libraries.
- Lua 5.3 has no prebuilt Switch library, so `tools/build_lua_switch.sh` builds 5.3.6 from lua.org (checksum checked).
- Everything lives in `/switch/sm64coopdx` on the SD card: config, saves, mods and the ROM.
- The Switch only has IPv4 sockets. The game opens an IPv6 socket and keeps v4-mapped addresses everywhere, so on Switch it opens an IPv4 socket and converts addresses in bind, sendto and recvfrom. The rest of the network code is untouched.
- Without CoopNet you join games by IP address.
- The window is fixed at 1280x720, fullscreen.
- The software keyboard only opens when you select a text box (chat, IP field). Upstream starts text input at boot, which on Switch would pop the keyboard right away. What you type replaces the box's content, since the Switch keyboard opens empty and can't erase what was there.
- Mumble positional audio is off (no shared memory).
- Fatal errors show the system error dialog and go into the log file.
- On exit the game calls `SDL_Quit()`, so the window, GL context and audio are released before hbmenu comes back.
- The loading thread gets an 8 MB stack.
- Buttons are mapped by label, not by position: A jumps and confirms, B punches.
- The ROM is used where it is. Upstream copies it to `baserom.us.z64` first, and on Switch that copy came out as an empty file.
- The fragment shaders are valid GLSL ES 1.00 (upstream relies on desktop GLSL converting int to float).

## Not tested yet

- Audio hasn't been listened to yet. The audio device opens fine (no SDL audio error in the log). The `heap.c: Tried to alloc 5184/5760 bytes ... failed` lines are expected: at startup the game fills its sample buffer pool until it is full and stops at the first failure, on PC as well.
- Online play is untested.

## Logs

Each launch writes `/switch/sm64coopdx/switch-log.txt` on the SD card: memory at startup, a timed line for each startup step, the game's errors, the message of a fatal error, and the shutdown. If the game crashes and the loader passes the exception on, the registers end up there too. Attach that file when you report a problem.

Crash addresses show up as `elf+0x...`. With a build of the same commit, `aarch64-none-elf-addr2line -f -e build/us_pc/sm64coopdx.elf 0x...` gives the function.
