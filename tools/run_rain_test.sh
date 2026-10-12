#!/usr/bin/env bash
set -euo pipefail
project_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
weather_map=warlock
case "${1:-}" in
    warlock|backwash) weather_map="$1"; shift ;;
    ""|--*) ;;
    *) echo "Usage: $0 [warlock|backwash] [native options]" >&2; exit 2 ;;
esac
cd -- "$project_root"
exec "$project_root/Halo2-Launcher" \
    --multiplayer --map "$project_root/assets/map-pack/maps/$weather_map.map" \
    --mp-starting-weapon 5 "$@"
