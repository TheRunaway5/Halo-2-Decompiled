#!/usr/bin/env python3
"""Finite same-engine original/native31650 GPU, full command-bank and FP check.

Use run_test.py. Captures, command banks and previews belong to its scratch;
only bounded hashes, counts and failure diagnostics survive.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
from perf_4k60 import parity


def sha(path):
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--runtime', type=Path, required=True)
    parser.add_argument('--maps', type=Path, required=True)
    parser.add_argument('--map', default='warlock')
    parser.add_argument('--case', default='1:150:10:110')
    parser.add_argument('--starting-weapon', type=int, choices=(0, *range(3, 21)), default=0)
    parser.add_argument('--grenade-tick', type=int)
    parser.add_argument('--look', type=float, nargs=2, metavar=('YAW', 'PITCH'))
    parser.add_argument('--probe-only', action='store_true', help='One genuine run with no parity acceptance')
    parser.add_argument('--report', type=Path, required=True)
    parser.add_argument('--require-f8', action='store_true')
    a = parser.parse_args()
    players, tick, first, last = map(int, a.case.split(':'))
    if not (1 <= players <= 4 and tick > 0 and 0 <= first <= last):
        parser.error('Invalid players:capture_tick:fire_first:fire_last')
    if a.grenade_tick is not None and not 0 <= a.grenade_tick < tick:
        parser.error('Grenade tick must occur before capture')
    if a.look and not (-1.5 <= a.look[1] <= 1.5):
        parser.error('Pitch must be in the native input range')
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    runtime = a.runtime.resolve(strict=True)
    maps = a.maps.resolve(strict=True)
    pins = {str(path): sha(path) for path in (runtime, maps / (a.map + '.map'), Path(parity.__file__))}
    entry = dict(case=a.case, runs=[])
    banks = []
    script_text = None
    if a.grenade_tick is not None or a.look:
        look = a.look or (1.35, -0.6)
        script_text = '# tick player fire reload grenade yaw pitch\n' + ''.join(
            f'{t} {p} {int(first <= t < last and last > 0)} 0 '
            f'{int(p == 0 and t == a.grenade_tick)} {look[0]} {look[1]}\n'
            for t in range(tick) for p in range(players))
    for role in ('reference', 'reference-repeat', 'candidate', 'candidate-repeat'):
        name = 'light-flags-' + role
        bank = scratch / (name + '.banks')
        reference = role.startswith('reference')
        env = {'HALO2_REFERENCE_LIGHT_FLAGS_31650': '1' if reference else '',
               'HALO2_NATIVE_LIGHT_FLAGS_STATS': '1',
               'HALO2_COMMAND_BANKS_PATH': str(bank),
               'HALO2_REFERENCE_BANK_DRAW_UNITS': '',
               'HALO2_REFERENCE_MODEL_DRAW_UNITS': '',
               'HALO2_REFERENCE_LIGHT_PARENT_BIPED': ''}
        run = parity.capture(runtime, name, players, tick, first, last, env,
                             scratch=scratch, map_name=a.map, maps=maps,
                             size=(1920, 1080), timeout_seconds=240,
                             extra_arguments=['--mp-starting-weapon', str(a.starting_weapon)] if a.starting_weapon else [],
                             script_text=script_text)
        run.update(role=role, environment=env)
        rgb = run.get('_payload', b'').split(b'\n', 3)[-1]
        run['nonuniform_image'] = bool(rgb) and any(
            rgb[i:i + 3] != rgb[:3] for i in range(3, len(rgb), 3))
        if role == 'candidate' and run.get('_payload'):
            preview = scratch / 'light-flags-candidate-preview.ppm'
            preview.write_bytes(run['_payload'])
            run['scratch_preview'] = str(preview)
        log = scratch / (name + '.log')
        with log.open('rb') as stream:
            stream.seek(max(0, log.stat().st_size - 4 * 1024 * 1024))
            text = stream.read().decode(errors='replace')
        sphere = re.findall(r'Sphere31650 stats: calls=(\d+) bsp_rows=(\d+) '
                            r'object_rows=(\d+) changed_objects=(\d+) none=(\d+) '
                            r'root=(\d+) f8=(\d+) reference=(\d+)', text)
        for n, key in enumerate(('calls', 'bsp_rows', 'object_rows', 'changed_objects',
                                 'none_passes', 'root_passes', 'f8', 'reference_calls')):
            run[key] = sum(int(row[n]) for row in sphere)
        world = re.findall(r'Native31650 stats: native=(\d+) reference=(\d+) '
                           r'bsp_rows=(\d+) object_rows=(\d+) changed=(\d+) f8=(\d+)', text)
        run['world_stats'] = [list(map(int, row)) for row in world]
        run['grenade_observations'] = len(re.findall(r'^pStencil grenade input: ', text, re.MULTILINE))
        detonations = re.findall(r'^Input capture tick=.*?\bdetonations=(\d+)\b', text, re.MULTILINE)
        run['detonations'] = int(detonations[-1]) if detonations else 0
        data = bank.read_bytes() if bank.is_file() else b''
        banks.append(data)
        run.update(bank_bytes=len(data), bank_sha256=hashlib.sha256(data).hexdigest())
        entry['runs'].append(run)
        print(role, run['valid'], run.get('sha256'), run['calls'], 'flag calls',
              run['changed_objects'], 'changed object rows', len(data), 'bank bytes', flush=True)
        if a.probe_only:
            run.pop('_payload', None)
            report = dict(passed=False, validation_not_run=True, probe_completed=run['valid'], case=entry,
                          source_runtime_map_sha256=pins, helper_sha256=sha(__file__),
                          grenade_tick=a.grenade_tick, look=a.look, input_script=script_text,
                          retained_large_artifacts=[], scope='Single genuine input coverage probe; no native/GPU parity acceptance.')
            a.report.parent.mkdir(parents=True, exist_ok=True)
            a.report.write_text(json.dumps(report, indent=2) + '\n')
            return int(not run['valid'])
    parity.compare_runs(entry)
    entry['all_images_nonuniform'] = all(r['nonuniform_image'] for r in entry['runs'])
    entry['positive_work'] = all(r['calls'] > 0 and r['changed_objects'] > 0 for r in entry['runs'])
    entry['reference_selected'] = all(r['reference_calls'] == r['calls'] and r['calls'] > 0
                                      for r in entry['runs'] if r['role'].startswith('reference'))
    entry['native_selected'] = all(r['reference_calls'] == 0 and r['calls'] > 0
                                   for r in entry['runs'] if r['role'].startswith('candidate'))
    entry['f8_exercised'] = all(r['f8'] > 0 for r in entry['runs'])
    entry['grenade_exercised'] = all(r['grenade_observations'] > 1 and r['detonations'] > 0 for r in entry['runs'])
    entry['complete_banks_and_fp_equal'] = bool(banks[0]) and all(b == banks[0] for b in banks)
    entry['bank_first_differences'] = []
    for run, data in zip(entry['runs'], banks, strict=True):
        if data == banks[0]:
            continue
        offset = next((i for i, (x, y) in enumerate(zip(banks[0], data)) if x != y),
                      min(len(banks[0]), len(data)))
        entry['bank_first_differences'].append(dict(
            role=run['role'], offset=offset,
            reference=banks[0][max(0, offset - 16):offset + 32].hex(),
            actual=data[max(0, offset - 16):offset + 32].hex()))
    unchanged = all(sha(path) == digest for path, digest in pins.items())
    entry['passed'] = (entry['passed'] and entry['all_images_nonuniform'] and entry['positive_work']
                       and entry['reference_selected'] and entry['native_selected']
                       and entry['complete_banks_and_fp_equal'] and unchanged
                       and (not a.require_f8 or entry['f8_exercised'])
                       and (a.grenade_tick is None or entry['grenade_exercised']))
    report = dict(passed=entry['passed'], case=entry, inputs_unchanged=unchanged,
                  source_runtime_map_sha256=pins, helper_sha256=sha(__file__),
                  f8_required=a.require_f8, starting_weapon=a.starting_weapon, retained_large_artifacts=[],
                  grenade_tick=a.grenade_tick, look=a.look, input_script=script_text,
                  whole_renderer_xbox_visual_parity_verified=False,
                  scope='Four finite same-engine GPU runs. Only31650 reference selection differs: '
                        'reference executes unchanged TU1.5 code on the actual pre-edit receiver queues '
                        'and validates native typed results; candidate uses typed native flags and F8 '
                        'store. Every run requires changed live object flags, nonuniform images, '
                        'identical repeated images and complete command histories plus main FP. '
                        'This is bounded integration evidence, not physical Xbox framebuffer parity.')
    a.report.parent.mkdir(parents=True, exist_ok=True)
    a.report.write_text(json.dumps(report, indent=2) + '\n')
    return int(not report['passed'])


if __name__ == '__main__':
    raise SystemExit(main())
