#!/usr/bin/env bash
# Dev server with live reload over the debug build: rebuild in another terminal (scripts/build.sh debug).
cd "$(dirname "$0")/../tools" && exec node dev_server.mjs "${1:-../build-web-debug}" "${2:-8080}"
