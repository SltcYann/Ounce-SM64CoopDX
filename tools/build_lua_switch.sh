#!/bin/sh
# Builds Lua 5.3.6 for the Switch 2 into lib/lua/switch/liblua53.a.
# The headers in lib/lua/include are from 5.3.5; the API is the same.
set -eu

: "${DEVKITPRO:=/opt/devkitpro}"
CC="$DEVKITPRO/devkitA64/bin/aarch64-none-elf-gcc"
AR="$DEVKITPRO/devkitA64/bin/aarch64-none-elf-ar"
SHA256=fc5fd69bb8736323f026672b1b7235da613d7177e72558893a0bdcd320466d60

ROOT=$(cd "$(dirname "$0")/.." && pwd)
WORK=$(mktemp -d)
trap 'rm -rf "$WORK"' EXIT

curl -fsSL -o "$WORK/lua.tar.gz" https://www.lua.org/ftp/lua-5.3.6.tar.gz
if command -v sha256sum >/dev/null 2>&1; then
    actual=$(sha256sum "$WORK/lua.tar.gz" | cut -d' ' -f1)
else
    actual=$(shasum -a 256 "$WORK/lua.tar.gz" | cut -d' ' -f1)
fi
[ "$actual" = "$SHA256" ] || { echo "lua-5.3.6.tar.gz: wrong checksum $actual" >&2; exit 1; }

tar -xzf "$WORK/lua.tar.gz" -C "$WORK"
cd "$WORK/lua-5.3.6/src"
for f in *.c; do
    case "$f" in lua.c|luac.c) continue ;; esac
    "$CC" -O2 -mcpu=cortex-a78c+crypto -mtp=soft -fPIE -ffunction-sections \
        -DLUA_COMPAT_5_2 -D__SWITCH__ -c "$f" -o "${f%.c}.o"
done

mkdir -p "$ROOT/lib/lua/switch"
rm -f "$ROOT/lib/lua/switch/liblua53.a"
"$AR" rcs "$ROOT/lib/lua/switch/liblua53.a" ./*.o
echo "built lib/lua/switch/liblua53.a"
