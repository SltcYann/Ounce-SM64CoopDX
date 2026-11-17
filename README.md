![sm64coopdx Logo](textures/segment2/custom_coopdx_logo.rgba32.png)

# Ounce-SM64CoopDX

My port of [sm64coopdx](https://github.com/coop-deluxe/sm64coopdx), the online multiplayer Super Mario 64 PC port, to the Nintendo Switch 2. It builds as a `.nro` that runs from hbmenu under [kaiHEN](https://github.com/ouncebrew/kaihen), using [ounce-devkit](https://github.com/ouncebrew/ounce-devkit).

This is an unofficial fork. The game itself is the work of the Coop Deluxe team; what this repo adds is everything it took to get it running on the console.

## Where it's at

On my Switch 2 it loads, gets to the menu and plays at full speed: the game logic holds 30 frames per second, as on N64. Sound works, hosting a game works, the built-in mods load, the software keyboard opens in text fields, and the buttons follow the Switch labels (A jumps, B punches).

Vsync at 120 Hz and rumble are fixed in the code but not yet confirmed on the console. Joining someone else's game, touch in the menus and docked mode haven't been tried yet. If you do, I'd like to know how it goes.

## What I had to change

The PC code mostly compiles as is. Getting it to actually run took a few fixes:

- **A Switch 2 build target.** `make TARGET_SWITCH=1` builds with devkitA64 and libounce, renders with OpenGL ES through OunceVK, and packs a `.nro`. Discord, CoopNet and the updater are off, since they only ship as PC binaries. Lua 5.3 gets built from source.
- **The ROM copy.** On first launch the game copies your ROM to `baserom.us.z64`. On the Switch that copy came out empty, and every launch after that loaded the game's assets from an empty file and died partway through. It now uses the ROM where it finds it.
- **Networking.** The Switch only has IPv4 sockets and the game opens an IPv6 one. On Switch it opens IPv4 and converts addresses at the edges, so the rest of the network code is untouched. Without CoopNet, you join games by IP address.
- **Shaders.** A couple of the game's shaders mixed integers and floats, which desktop OpenGL tolerates and OpenGL ES rejects.
- **The screen.** SDL's Switch driver was written for the Switch 1 and kept resizing the picture behind the game's back. The game now draws to a fixed 1920x1080 surface, and vsync follows the display's real refresh rate instead of SDL's fixed 60 Hz, which should reach 120 Hz on the handheld screen.
- **Console details.** The keyboard only opens when you pick a text box and its text replaces the box's content, buttons are mapped by label instead of position, rumble talks to the controllers directly, mods load once instead of twice, and the game shuts SDL down on exit so hbmenu comes back cleanly.

Getting there took a log file on the SD card, since there is no console output. Each launch still writes `/switch/sm64coopdx/switch-log.txt`, which helps if something goes wrong.

## Installing

1. Download `Ounce-SM64CoopDX-switch2.zip` from the [latest release](https://github.com/SltcYann/Ounce-SM64CoopDX/releases/latest), or build it yourself (below).
2. Unzip it at the root of the SD card. It creates `/switch/sm64coopdx/` with the `.nro` and the `lang`, `mods`, `dynos` and `palettes` folders.
3. Add your own Super Mario 64 (USA) ROM in `.z64` format to the same folder. Any file name works. No ROM or game asset is included here.
4. Open hbmenu with kaiHEN (start a game while holding R) and launch sm64coopdx.

## Building

Every push to `main` is built by GitHub Actions, and the zip is in the run's artifacts. To build it yourself, you need devkitPro with the Switch packages, ounce-devkit on top of it, and GNU make 4. The full steps, the complete list of changes and how to read the log are in [docs/switch2.md](docs/switch2.md).

```sh
sudo dkp-pacman -S --needed switch-dev switch-sdl2 switch-curl switch-zlib
sh tools/build_lua_switch.sh
DEVKITPRO=/opt/devkitpro gmake TARGET_SWITCH=1 -j8
```

## Credits

- The [Coop Deluxe team](https://github.com/coop-deluxe/sm64coopdx) for sm64coopdx, and djoslin0 for sm64ex-coop before it.
- The [ouncebrew](https://github.com/ouncebrew) folks for kaiHEN, libounce, OunceVK and ounce-devkit, which do the heavy lifting on the Switch 2 side.
- The Super Mario 64 decompilation project, which all of this is built on.

For Switch problems, ask here, not upstream. For anything about the game itself, the [sm64coopdx repo](https://github.com/coop-deluxe/sm64coopdx) and its [Discord](https://discord.gg/TJVKHS4) are the place.

Not affiliated with Nintendo or the Coop Deluxe team.
