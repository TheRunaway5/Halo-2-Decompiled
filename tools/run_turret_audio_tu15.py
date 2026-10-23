#!/usr/bin/env python3
"""Exercise a prepared armed seat with actual SDL fire and audio submissions.

Use run_test.py. Window/frame/audio resources stay in that owned scratch.
SDL dummy output proves native voice/PCM submission, not physical speakers.
"""
import json
import os
from pathlib import Path
import subprocess
import sys

from validate_weapon_runtime_tu15 import private_display
from weapon_test_cache import require_mcc_map


def main():
    executable, map_name, case, report_name, *secondary = sys.argv[1:]
    owner = Path(os.environ['HALO2_TEST_SCRATCH'])
    report = Path(report_name).resolve()
    map_ = require_mcc_map(map_name)
    cache = Path(os.environ.get('HALO2_TURRET_GRAPHICS_CACHE', str(owner / 'cache'))).resolve()
    parents = [owner] + [path for path in owner.parents if path.name.startswith('halo2-test-')]
    if not any(cache.is_relative_to(path) for path in parents):
        raise ValueError('Turret graphics cache must remain inside an active test owner')
    with private_display(owner) as display:
        env = dict(os.environ, DISPLAY=display, SDL_VIDEODRIVER='x11', SDL_AUDIODRIVER='dummy',
                   XDG_CACHE_HOME=str(cache), HALO2_PIPELINE_CACHE_DIRECTORY=str(cache / 'pipelines'),
                   HALO2_TEST_TURRET_AUDIO='1',
                   HALO2_TEST_VEHICLE_WEAPON_HUD_CASE=case,
                   HALO2_VEHICLE_HUD_GPU_NAME='warthog' if case.startswith('warthog-') else case,
                   HALO2_TEST_VEHICLE_WEAPON_HUD_REPORT=str(report))
        if secondary:
            env['HALO2_TEST_VEHICLE_WEAPON_SECONDARY'] = '1'
        with (owner / 'runtime.log').open('w') as log:
            code = subprocess.run([executable, str(map_), str(map_.parent), 'native-mcc',
                                   str(owner / 'frame.ppm')], env=env,
                                  stdout=log, stderr=subprocess.STDOUT, timeout=240).returncode
    data = (owner / 'runtime.log').read_bytes()
    report.with_suffix('.log').write_bytes(data if len(data)<65536 else data[:16384]+b'\n[trimmed]\n'+data[-49152:])
    print(json.dumps({'case': case, 'exit_code': code, 'report': str(report)}))
    return code


if __name__ == '__main__':
    raise SystemExit(main())
