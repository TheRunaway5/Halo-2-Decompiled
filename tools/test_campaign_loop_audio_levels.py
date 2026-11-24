#!/usr/bin/env python3
"""Start every campaign level and check its loop-sound audio stays healthy.

Each level runs for --ticks simulation ticks from its authored start (no
player input) with HALO2_SCRIPT_CALL_TRACE, so the campaign World's 18EE50
object-audio frame drives BSP background sound (tracks and random details)
and object loops through the native loop lane, and the World's own sound
requests (footsteps, foley, devices, effects) play. A level passes when the run
completes with no object-audio fault, script fault or audio fault.

Reported per level: ambient packets and the distinct sounds they played, and
any loop sound the native lane rejected ('Campaign loop sound ...'; the
sound stays silent, as before). --reference-ambient-lane replays with
HALO2_REFERENCE_CAMPAIGN_AMBIENT_LANE and HALO2_REFERENCE_CAMPAIGN_NO_SOURCE_SOUNDS
(the earlier campaign audio) for comparison.

Run under tools/run_test.py; logs stay in HALO2_TEST_SCRATCH.
"""

import argparse
import json
import os
from pathlib import Path
import re
import subprocess

from physics_test_cache import runtime_cache, runtime_map
from validate_weapon_runtime_tu15 import private_display


LEVELS = ('00a_introduction', '01a_tutorial', '01b_spacestation', '03a_oldmombasa', '03b_newmombasa',
          '04a_gasgiant', '04b_floodlab', '05a_deltaapproach', '05b_deltatowers', '06a_sentinelwalls',
          '06b_floodzone', '07a_highcharity', '07b_forerunnership', '08a_deltacliffs', '08b_deltacontrol')
FAULTS = ('Campaign object audio: ', 'Audio fault: ', 'Campaign ambience: ', 'Script fault:')


def fields(text):
    return dict(re.findall(r'(\w+)=([^ ]+)', text))


def run_level(runtime, maps, level, ticks, reference, owner, timeout):
    work = owner / ('reference' if reference else 'native') / level
    work.mkdir(parents=True)
    controls = work / 'controls.json'
    controls.write_text(json.dumps([{'tick': 1, 'pitch': 0.0, 'look_stick': 0.0}]))
    environment = {key: value for key, value in os.environ.items() if not key.startswith('HALO2_')
                   and key not in ('DISPLAY', 'WAYLAND_DISPLAY', 'XAUTHORITY', 'VK_INSTANCE_LAYERS',
                                   'XDG_ACTIVATION_TOKEN')}
    environment.update(HALO2_TEST_SCRATCH=str(owner), HALO2_PIPELINE_CACHE_DIRECTORY=str(owner / 'pipelines'),
                       HALO2_PSTENCIL_CAPTURE_PRESENT_MODE='immediate', HALO2_SCRIPT_CALL_TRACE='1',
                       SDL_VIDEODRIVER='x11', SDL_AUDIODRIVER='dummy')
    if reference:
        environment['HALO2_REFERENCE_CAMPAIGN_AMBIENT_LANE'] = '1'
        environment['HALO2_REFERENCE_CAMPAIGN_NO_SOURCE_SOUNDS'] = '1'
    for key in ('XDG_CONFIG_HOME', 'XDG_DATA_HOME', 'XDG_STATE_HOME', 'XDG_CACHE_HOME'):
        environment[key] = str(work / key.lower())
    command = [str(runtime), '--campaign', '--map', str(runtime_map(maps, maps / f'{level}.map')),
               '--shared-maps', str(maps), '--hidden', '--graphics-mode', 'Original',
               '--viewport', '640', '360', '--render-size', '640', '360',
               '--campaign-live-input-script', str(controls), str(ticks)]
    log = work / 'runtime.log'
    with private_display(work) as display, log.open('wb') as output:
        environment['DISPLAY'] = display
        try:
            status = subprocess.run(command, cwd=runtime.parent, stdout=output, stderr=subprocess.STDOUT,
                                    env=environment, timeout=timeout).returncode
        except subprocess.TimeoutExpired:
            status = 124
    lines = log.read_text(errors='replace').splitlines()
    ambient = [fields(line[len('Script audio ambience: '):]) for line in lines
               if line.startswith('Script audio ambience: ')]
    faults = [line[:300] for line in lines if line.startswith(FAULTS)]
    world = [line for line in lines if line.startswith('Source audio packet: ')]
    gaps = sorted({line[:300] for line in lines if line.startswith(('Campaign loop sound ', 'Campaign source sound '))})
    completed = any(line.startswith('Campaign live input complete: ') for line in lines)
    return {'exit_code': status, 'completed': completed, 'faults': faults[:8], 'loop_sound_gaps': gaps[:16],
            'ambient_packets': len(ambient), 'ambient_sounds': len({row.get('sound') for row in ambient}),
            'world_sound_packets': len(world),
            'log_tail': lines[-6:]}


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--runtime', type=Path, required=True, help='halo2_native inside its build directory')
    parser.add_argument('--mcc-root', type=Path, required=True)
    parser.add_argument('--ticks', type=int, default=900)
    parser.add_argument('--levels', default=','.join(LEVELS))
    parser.add_argument('--reference-ambient-lane', action='store_true')
    parser.add_argument('--timeout', type=int, default=360, help='Seconds per level')
    parser.add_argument('--report-output', type=Path, required=True)
    args = parser.parse_args()
    owner = Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    runtime = args.runtime.resolve(strict=True)
    maps, _ = runtime_cache(args.mcc_root)
    report = {'runtime': str(runtime), 'ticks': args.ticks, 'reference_ambient_lane': args.reference_ambient_lane,
              'retained_large_artifacts': [], 'levels': {}}
    for level in args.levels.split(','):
        result = run_level(runtime, maps, level, args.ticks, args.reference_ambient_lane, owner, args.timeout)
        result['passed'] = result['exit_code'] == 0 and result['completed'] and not result['faults']
        report['levels'][level] = result
        print(json.dumps({level: {key: value for key, value in result.items() if key != 'log_tail'}}), flush=True)
    report['passed'] = all(result['passed'] for result in report['levels'].values())
    args.report_output.parent.mkdir(parents=True, exist_ok=True)
    args.report_output.write_text(json.dumps(report, indent=2) + '\n')
    return int(not report['passed'])


if __name__ == '__main__':
    raise SystemExit(main())
