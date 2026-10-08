#!/usr/bin/env bash
# Usage: scripts/build.sh [debug|debug-san|release|native-test|native-game]
set -euo pipefail
cd "$(dirname "$0")/.."
[ -f "$HOME/emsdk/emsdk_env.sh" ] && source "$HOME/emsdk/emsdk_env.sh" >/dev/null 2>&1 || true
mode="${1:-debug}"
case "$mode" in
  debug)
    emcmake cmake -S . -B build-web-debug -DCMAKE_BUILD_TYPE=Debug
    cmake --build build-web-debug -j ;;
  debug-san)
    emcmake cmake -S . -B build-web-san -DCMAKE_BUILD_TYPE=Debug -DSA_SANITIZE=ON
    cmake --build build-web-san -j ;;
  release)
    emcmake cmake -S . -B build-web-release -DCMAKE_BUILD_TYPE=Release
    cmake --build build-web-release -j ;;
  native-test)
    cmake -S . -B build-native -DSA_SANITIZE=ON
    cmake --build build-native -j
    ctest --test-dir build-native --output-on-failure ;;
  native-game)
    cmake -S . -B build-native-game -DSA_BUILD_NATIVE_GAME=ON -DSA_NATIVE_TESTS=OFF
    cmake --build build-native-game -j ;;
  *) echo "unknown mode $mode"; exit 1 ;;
esac
