#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
: "${SDK_PREFIX:?Set SDK_PREFIX to an absolute path containing the Windows SDL/FreeType SDK}"
export SDK_PREFIX
make -j"${JOBS:-2}" CROSS="${CROSS:-x86_64-w64-mingw32-}" NATIVE=win64  PKG_CONFIG="$PWD/fpv/win-pkg-config" TILES=1 SOUND=1 LOCALIZE=1 BUILD_PREFIX=ru-  RELEASE=1 OPTLEVEL=-O1 BACKTRACE=0 DYNAMIC_LINKING=1 LINTJSON=0 ASTYLE=0 TESTS=0 PCH=1 CXX_WARNINGS=-w "$@"
make -C lang LANGUAGES=ru
