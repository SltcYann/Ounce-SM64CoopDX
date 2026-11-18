#!/bin/sh
# Builds CoopNet and libjuice for the Switch 2 into lib/coopnet/switch/.
# Upstream only ships CoopNet for PC, so the sources are downloaded at the
# versions below, checked against their SHA-256, patched for the Switch
# (patches/libjuice-switch.patch, patches/coopnet-switch.patch) and compiled
# with devkitA64.
set -eu

: "${DEVKITPRO:=/opt/devkitpro}"
BIN="$DEVKITPRO/devkitA64/bin"
CC="$BIN/aarch64-none-elf-gcc"
CXX="$BIN/aarch64-none-elf-g++"
AR="$BIN/aarch64-none-elf-ar"

JUICE_URL=https://github.com/paullouisageneau/libjuice/archive/refs/tags/v1.6.2.tar.gz
JUICE_SHA256=5078176d55042f3ccf3999c2556d84903f7edf80177ce4a7bf59507541e93938
COOPNET_COMMIT=9d9b3dd4e87dba2fa3ca542ae32b73f43df32b0e
COOPNET_URL=https://github.com/coop-deluxe/coopnet/archive/$COOPNET_COMMIT.tar.gz
COOPNET_SHA256=8847aadf4bfac6d0b6002432810f1d30c365d1d1df0596d10488298a56601e05

ROOT=$(cd "$(dirname "$0")/.." && pwd)
WORK=$(mktemp -d)
trap 'rm -rf "$WORK"' EXIT

sha256() {
    if command -v sha256sum >/dev/null 2>&1; then sha256sum "$1" | cut -d' ' -f1
    else shasum -a 256 "$1" | cut -d' ' -f1; fi
}

fetch() { # url sha256 output
    curl -fsSL -o "$3" "$1"
    actual=$(sha256 "$3")
    [ "$actual" = "$2" ] || { echo "$1: wrong checksum $actual" >&2; exit 1; }
}

fetch "$JUICE_URL" "$JUICE_SHA256" "$WORK/juice.tar.gz"
fetch "$COOPNET_URL" "$COOPNET_SHA256" "$WORK/coopnet.tar.gz"
tar -xzf "$WORK/juice.tar.gz" -C "$WORK"
tar -xzf "$WORK/coopnet.tar.gz" -C "$WORK"
JUICE="$WORK/libjuice-1.6.2"
COOPNET="$WORK/coopnet-$COOPNET_COMMIT"
patch -d "$JUICE" -p1 < "$ROOT/patches/libjuice-switch.patch"
patch -d "$COOPNET" -p1 < "$ROOT/patches/coopnet-switch.patch"

ARCH="-mcpu=cortex-a78c+crypto -mtp=soft -fPIE -ffunction-sections -O2"
DEFS="-D__SWITCH__ -D__SWITCH2__ -DJUICE_STATIC"
INC="-I$DEVKITPRO/libounce/include -I$DEVKITPRO/portlibs/switch/include"

# libjuice: NO_IFADDRS uses its own local address lookup instead of
# getifaddrs(), which the Switch doesn't have. Its socket.h still includes
# <ifaddrs.h>, so an empty one stands in for it.
mkdir -p "$WORK/o/juice" "$WORK/shim"
: > "$WORK/shim/ifaddrs.h"
for f in "$JUICE"/src/*.c; do
    "$CC" $ARCH $DEFS $INC -I"$WORK/shim" -DNO_IFADDRS -D__LITTLE_ENDIAN__ -I"$JUICE/include" -I"$JUICE/include/juice" \
        -c "$f" -o "$WORK/o/juice/$(basename "${f%.c}").o"
done

# CoopNet: the client side only (common/), as upstream's libcoopnet. LOGGING
# sends its connection messages to stdout, which is switch-log.txt on Switch.
mkdir -p "$WORK/o/coopnet"
for f in "$COOPNET"/common/*.cpp; do
    "$CXX" $ARCH $DEFS $INC -DLOGGING -std=gnu++17 -include climits -include sys/time.h -include sys/select.h -I"$COOPNET/common" -I"$JUICE/include" \
        -c "$f" -o "$WORK/o/coopnet/$(basename "${f%.cpp}").o"
done

OUT="$ROOT/lib/coopnet/switch"
mkdir -p "$OUT"
rm -f "$OUT/libjuice.a" "$OUT/libcoopnet.a"
"$AR" rcs "$OUT/libjuice.a" "$WORK"/o/juice/*.o
"$AR" rcs "$OUT/libcoopnet.a" "$WORK"/o/coopnet/*.o
echo "built lib/coopnet/switch/libcoopnet.a and libjuice.a"
