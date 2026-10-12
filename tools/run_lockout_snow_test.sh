#!/usr/bin/env bash
set -euo pipefail
project_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
cd -- "$project_root"
exec "$project_root/Halo2-Launcher" \
    --multiplayer --map "$project_root/assets/disc/maps/lockout.map" \
    --mp-starting-weapon 5 "$@"
