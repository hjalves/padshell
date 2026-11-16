#!/bin/sh
# Build the Switch .nro inside devkitPro's official devkitA64 container.
# Works with podman or docker. Output: build-switch/padshell.nro
set -eu
cd "$(dirname "$0")/.."
ENGINE=$(command -v podman || command -v docker)
exec "$ENGINE" run --rm -v "$PWD":/src:Z -w /src docker.io/devkitpro/devkita64:latest sh -c '
  /opt/devkitpro/portlibs/switch/bin/aarch64-none-elf-cmake -S . -B build-switch -DCMAKE_BUILD_TYPE=Release &&
  cmake --build build-switch -j"$(nproc)"'
