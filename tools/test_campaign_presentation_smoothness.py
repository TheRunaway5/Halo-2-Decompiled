#!/usr/bin/env python3
"""Measure campaign camera smoothness between simulation ticks on the Armory route.

Replays tests/fixtures/campaign_armory_tram_inputs.json plus a continuous turn
(ticks 2000..2299, while the player idles) with HALO2_CAMPAIGN_PRESENTATION_TRACE,
once with the HALO2_REFERENCE_* switches that restore the old presentation and
once without. Reports, per mode:

- look: frames whose view did not rotate during the turn, and turn-rate spread;
- tram: per-frame jump of the camera relative to the moving tram floor;
- slots: frame-clock-to-draw-end latency split by ticks run in the frame.

Run under tools/run_test.py; traces stay in HALO2_TEST_SCRATCH.
"""

import argparse
import json
import math
import os
from pathlib import Path
import statistics
import subprocess

from physics_test_cache import runtime_cache, runtime_map
from validate_weapon_runtime_tu15 import private_display


ROOT = Path(__file__).resolve().parents[1]
CONTROLS = ROOT / 'tests/fixtures/campaign_armory_tram_inputs.json'
REFERENCE = {'HALO2_REFERENCE_CAMPAIGN_TICK_LOOK': '1',
             'HALO2_REFERENCE_CAMPAIGN_MACHINE_TICK_POSE': '1',
             'HALO2_REFERENCE_OVERLAP_SAMPLE_CLOCK': '1'}
TURN = (2000, 2300)   # The route sets an absolute yaw again at 2700.
TRAM_BOARDED = 9207   # Last route movement; the tram ride follows.


def controls(path):
    rows = json.loads(CONTROLS.read_text())
    taken = {row['tick'] for row in rows}
    for tick in range(*TURN):
        if tick in taken:
            raise ValueError('The turn overlaps a recorded route input')
        rows.append({'tick': tick, 'yaw': 0.8 + 0.03 * (tick - TURN[0])})
    path.write_text(json.dumps(sorted(rows, key=lambda row: row['tick'])))


def replay(runtime, maps, mode, ticks, owner, timeout):
    armory = runtime_map(maps, maps / '01a_tutorial.map')
    inputs, trace, log = owner / 'inputs.json', owner / f'{mode}.jsonl', owner / f'{mode}.log'
    controls(inputs)
    environment = {key: value for key, value in os.environ.items() if not key.startswith('HALO2_')
                   and key not in ('DISPLAY', 'WAYLAND_DISPLAY', 'XAUTHORITY', 'VK_INSTANCE_LAYERS',
                                   'XDG_ACTIVATION_TOKEN')}
    environment.update(HALO2_TEST_SCRATCH=str(owner), HALO2_PIPELINE_CACHE_DIRECTORY=str(owner / 'pipelines'),
                       HALO2_PSTENCIL_CAPTURE_PRESENT_MODE='immediate',
                       HALO2_CAMPAIGN_PRESENTATION_TRACE=str(trace), SDL_VIDEODRIVER='x11', SDL_AUDIODRIVER='dummy')
    if mode == 'reference':
        environment.update(REFERENCE)
    for key in ('XDG_CONFIG_HOME', 'XDG_DATA_HOME', 'XDG_STATE_HOME', 'XDG_CACHE_HOME'):
        environment[key] = str(owner / mode / key.lower())
    command = [str(runtime), '--campaign', '--map', str(armory), '--shared-maps', str(maps), '--hidden',
               '--graphics-mode', 'Original', '--viewport', '640', '360', '--render-size', '640', '360',
               '--campaign-live-input-script', str(inputs), str(ticks), '--mute']
    with private_display(owner) as display, log.open('wb') as output:
        environment['DISPLAY'] = display
        try:
            status = subprocess.run(command, cwd=runtime.parent, stdout=output, stderr=subprocess.STDOUT,
                                    env=environment, timeout=timeout).returncode
        except subprocess.TimeoutExpired:
            status = 124
    lines = log.read_text(errors='replace').splitlines()
    faults = [line[:300] for line in lines if line.startswith('Script fault:')]
    return status, trace, faults, lines[-12:]


def rms(values):
    return math.sqrt(sum(value * value for value in values) / len(values)) if values else None


def observe(trace):
    cameras, draws, machines = {}, {}, {}
    for line in trace.open():
        event = json.loads(line)
        if event['event'] == 'camera':
            cameras[event['frame']] = event
        elif event['event'] == 'draw':
            draws[event['frame']] = event
        else:
            machines.setdefault(event['object'], {})[event['frame']] = event
    frames = sorted(cameras)
    result = {'camera_frames': len(frames)}

    turn = [cameras[f] for f in frames
            if TURN[0] + 10 <= cameras[f]['tick'] < TURN[1] - 10 and not cameras[f]['vehicle']]
    steps, rates = [], []
    for a, b in zip(turn, turn[1:]):
        step = math.remainder(math.atan2(b['forward'][1], b['forward'][0]) -
                              math.atan2(a['forward'][1], a['forward'][0]), math.tau)
        steps.append(step)
        if b['clock_ms'] > a['clock_ms']:
            rates.append(step / (b['clock_ms'] - a['clock_ms']))
    if rates:
        mean = statistics.fmean(rates)
        result['look'] = {'frames': len(turn), 'stalled_share': sum(abs(s) < 1e-6 for s in steps) / len(steps),
                          'rate_rad_per_s': mean * 1000,
                          'rate_cv': statistics.pstdev(rates) / abs(mean) if mean else None}

    tram, travel = None, 0.0
    for owner, rows in machines.items():
        ride = [rows[f] for f in sorted(rows) if rows[f]['tick'] >= TRAM_BOARDED]
        if len(ride) > 30 and math.dist(ride[0]['completed'], ride[-1]['completed']) > travel:
            tram, travel = owner, math.dist(ride[0]['completed'], ride[-1]['completed'])
    if tram is not None:
        rows = machines[tram]
        shared = [f for f in frames if f in rows and cameras[f]['tick'] >= TRAM_BOARDED]
        relative = [[c - m for c, m in zip(cameras[f]['position'], rows[f]['shown'])] for f in shared]
        jumps = [math.dist(a, b) for a, b in zip(relative, relative[1:])]
        result['tram'] = {'object': tram, 'travel_wu': travel, 'frames': len(shared),
                          'relative_jump_rms_wu': rms(jumps), 'relative_jump_max_wu': max(jumps, default=None)}

    slots = {}
    for a, b in zip(frames, frames[1:]):
        draw = draws.get(b)
        if b != a + 1 or not draw or not draw['overlap']:
            continue
        slot = slots.setdefault(str(min(cameras[b]['tick'] - cameras[a]['tick'], 2)), [])
        slot.append(draw['end_ms'] - cameras[b]['clock_ms'])
    result['slots'] = {key: {'frames': len(values), 'clock_to_draw_end_ms': statistics.fmean(values)}
                       for key, values in sorted(slots.items())}
    if '0' in result['slots'] and '1' in result['slots']:
        result['tick_frame_bias_ms'] = (result['slots']['1']['clock_to_draw_end_ms'] -
                                        result['slots']['0']['clock_to_draw_end_ms'])
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--runtime', type=Path, required=True, help='halo2_native inside its build directory')
    parser.add_argument('--mcc-root', type=Path, required=True)
    parser.add_argument('--ticks', type=int, default=12000, help='3300 covers the turn only; 12000 includes the tram')
    parser.add_argument('--timeout', type=int, default=1500)
    parser.add_argument('--report-output', type=Path, required=True)
    args = parser.parse_args()
    owner = Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    runtime = args.runtime.resolve(strict=True)
    maps, _ = runtime_cache(args.mcc_root)
    report = {'runtime': str(runtime), 'ticks': args.ticks, 'retained_large_artifacts': []}
    for mode in ('reference', 'fixed'):
        status, trace, faults, tail = replay(runtime, maps, mode, args.ticks, owner, args.timeout)
        entry = {'exit_code': status, 'script_faults': faults[:8], 'log_tail': tail}
        try:
            entry.update(observe(trace))
        except (OSError, ValueError, KeyError) as failure:
            entry['observation_error'] = str(failure)
        report[mode] = entry
    fixed = report['fixed']
    checks = {'completed': all(report[m]['exit_code'] == 0 and not report[m]['script_faults']
                               for m in ('reference', 'fixed')),
              'look_never_stalls': fixed.get('look', {}).get('stalled_share') == 0}
    if args.ticks > TRAM_BOARDED:
        checks['tram_follows_camera'] = (fixed.get('tram', {}).get('relative_jump_rms_wu') or 1) < 0.002
    report['checks'] = checks
    report['passed'] = all(checks.values())
    args.report_output.parent.mkdir(parents=True, exist_ok=True)
    args.report_output.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({key: report[key] for key in ('passed', 'checks')} |
                     {mode: {k: v for k, v in report[mode].items() if k != 'log_tail'} for mode in ('reference', 'fixed')},
                     indent=1))
    return int(not report['passed'])


if __name__ == '__main__':
    raise SystemExit(main())
