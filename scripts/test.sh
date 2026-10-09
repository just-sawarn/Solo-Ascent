#!/usr/bin/env bash
# Native unit tests (ASan/UBSan) then browser smoke test against the debug web build.
set -euo pipefail
cd "$(dirname "$0")/.."
scripts/build.sh native-test
scripts/build.sh debug
(cd tools && node smoke.mjs ../build-web-debug)
(cd tools && node smoke_pwa.mjs ../build-web-debug)
