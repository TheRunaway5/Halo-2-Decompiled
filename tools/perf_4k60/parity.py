#!/usr/bin/env python3
"""Deterministic frame parity: scripted input, fixed tick cadence, fixed RNG seed.

parity.py REPORT_JSON REFERENCE_RUNTIME CANDIDATE_RUNTIME [case ...]
Each case is players:capture_tick:fire_first:fire_last. Raw captures stay in
HALO2_TEST_SCRATCH (removed by tools/run_test.py); only hashes are reported.
"""
import argparse
import hashlib
import json
import os
import subprocess
import sys
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
MAPS = ROOT / 'assets/imported/mcc-dire-valley-polish-20261023/halo2/h2_maps_win64_dx11'
SIZE = (3840, 2160)
DEFAULT_CASES = ['4:90:0:0', '4:150:10:110', '1:150:10:110']


def file_sha256(path):
    try:
        with Path(path).open('rb') as stream:
            return hashlib.file_digest(stream, 'sha256').hexdigest()
    except OSError:
        return None


def runtime_spec(text):
    # Optional 'VAR=VALUE,VAR2=VALUE@/path/to/runtime' adds role environment.
    assignments, _, path = text.rpartition('@')
    env = dict(a.split('=', 1) for a in assignments.split(',') if a)
    return Path(path).resolve(), env


def validate_image(payload, size=SIZE):
    """The renderer writes binary P6 with an exact native-size RGB payload."""
    header = payload.split(b'\n', 3)
    if len(header) != 4 or header[0] != b'P6' or header[2] != b'255':
        return 'malformed P6 image header'
    if header[1] != f'{size[0]} {size[1]}'.encode():
        return 'capture dimensions differ from the native render size'
    if len(header[3]) != size[0] * size[1] * 3:
        return 'capture RGB payload length differs from its dimensions'
    return None


def log_tail(path):
    with path.open('rb') as stream:
        stream.seek(max(0, path.stat().st_size - 1500))
        return stream.read().decode(errors='replace')


def capture(runtime, name, players, tick, first, last, role_env=None, *, scratch,
            map_name='coagulation', maps=MAPS, size=SIZE, timeout_seconds=600):
    runtime = Path(runtime).resolve()
    scratch = Path(scratch).resolve()
    maps = Path(maps).resolve()
    script = scratch / f'{name}.tsv'
    script.write_text('# tick player fire reload\n' + ''.join(
        f'{t} {p} {int(first <= t < last and last > 0)} 0\n' for t in range(tick) for p in range(players)))
    image = scratch / f'{name}.ppm'
    # A repeated helper invocation may reuse a case name inside one owner.
    image.unlink(missing_ok=True)
    command = ['xvfb-run', '--auto-servernum', '--server-args', f'-screen 0 {size[0]}x{size[1]}x24',
               str(runtime), '--multiplayer', '--graphics-mode', 'Original',
               '--map', str(maps / f'{map_name}.map'), '--shared-maps', str(maps),
               '--local-players', str(players), '--viewport', *map(str, size), '--render-size', *map(str, size),
               '--mp-input-script', str(script), '--mp-capture-tick', str(tick),
               '--mp-reference-tick-cadence', '--mp-reference-render-fraction', '0',
               '--mp-reference-auxiliary-seed', '1', '--screenshot', str(image), '--mute']
    log = scratch / f'{name}.log'
    timeout_status = scratch / f'{name}.timeout'
    timeout_status.unlink(missing_ok=True)
    # The nested owner stops xvfb-run and all of its runtime/Xvfb children
    # before returning, including when the timeout driver or caller exits.
    # Its temporary files belong to the same outer capture owner.
    timeout_driver = ('import subprocess,sys\nfrom pathlib import Path\n'
                      'try:\n'
                      ' result=subprocess.run(sys.argv[3:],timeout=float(sys.argv[1]))\n'
                      'except subprocess.TimeoutExpired:\n'
                      ' Path(sys.argv[2]).write_text("timed out")\n'
                      ' print("PARITY_CAPTURE_TIMEOUT",flush=True);raise SystemExit(124)\n'
                      'raise SystemExit(result.returncode)\n')
    owned_command = [sys.executable, '-B', str(ROOT / 'tools/run_test.py'),
                     '--scratch-root', str(scratch), '--', sys.executable, '-B', '-c',
                     timeout_driver, str(timeout_seconds), str(timeout_status), *command]
    started = time.monotonic()
    result = {'name': name, 'exit_code': None, 'valid': False, 'command': command,
              'owned_command': owned_command, 'cwd': str(runtime.parent)}
    try:
        with log.open('w') as out:
            try:
                result['exit_code'] = subprocess.run(
                    owned_command, cwd=runtime.parent, stdout=out, stderr=subprocess.STDOUT,
                    env={**{k: v for k, v in os.environ.items() if k != 'WAYLAND_DISPLAY'},
                         'SDL_VIDEODRIVER': 'x11', 'HALO2_MENU_FOG_SEED': '1',
                         'HALO2_PIPELINE_CACHE_DIRECTORY': str(scratch / 'vulkan-cache'),
                         'MESA_SHADER_CACHE_DIR': str(scratch / 'mesa-cache'),
                         '__GL_SHADER_DISK_CACHE_PATH': str(scratch / 'nvidia-cache'),
                         'XDG_DATA_HOME': str(scratch / 'xdg-data'),
                         'XDG_CONFIG_HOME': str(scratch / 'xdg-config'),
                         **(role_env or {})}).returncode
            except subprocess.TimeoutExpired:
                result['error'] = f'capture timed out after {timeout_seconds} seconds'
                result['timed_out'] = True
            except OSError as error:
                result['error'] = str(error)
        if timeout_status.exists():
            result['error'] = f'capture timed out after {timeout_seconds} seconds'
            result['timed_out'] = True
        if result['exit_code'] != 0:
            result.setdefault('error', 'capture process failed')
        if image.exists():
            result['bytes'] = image.stat().st_size
            # Bound a malformed output before reading it into memory.
            limit = size[0] * size[1] * 3 + 4096
            if result['bytes'] > limit:
                result.setdefault('error', 'capture exceeds native RGB size')
            else:
                payload = image.read_bytes()
                result['sha256'] = hashlib.sha256(payload).hexdigest()
                result['header'] = repr(payload[:128].split(b'\n', 3)[:3])
                error = validate_image(payload, size)
                if error:
                    result.setdefault('error', error)
                    result['wrong_size'] = payload.split(b'\n', 3)[1:2] != [f'{size[0]} {size[1]}'.encode()]
                elif result['exit_code'] == 0:
                    result['valid'] = True
                    result['_payload'] = payload
        else:
            result.setdefault('error', 'capture image was not produced')
        if not result['valid']:
            result['log_tail'] = log_tail(log)
        return result
    finally:
        result['seconds'] = round(time.monotonic() - started, 1)
        image.unlink(missing_ok=True)


def describe(a, b):
    """Differing pixels per 2x2 quadrant, max channel delta, bounding box."""
    ha, hb = a.split(b'\n', 3), b.split(b'\n', 3)
    w, h = map(int, ha[1].split())
    pa, pb = ha[3], hb[3]
    if len(pa) != len(pb):
        return {'size_mismatch': True}
    quads = [0, 0, 0, 0]; maxd = 0; box = [w, h, -1, -1]; count = 0
    row = w * 3
    for y in range(h):
        ra, rb = pa[y*row:(y+1)*row], pb[y*row:(y+1)*row]
        if ra == rb:
            continue
        for x in range(w):
            i = x * 3
            if ra[i:i+3] != rb[i:i+3]:
                count += 1
                d = max(abs(ra[i+k] - rb[i+k]) for k in range(3))
                maxd = max(maxd, d)
                quads[(2 if y >= h // 2 else 0) + (1 if x >= w // 2 else 0)] += 1
                box = [min(box[0], x), min(box[1], y), max(box[2], x), max(box[3], y)]
    return {'pixels': count, 'quadrants_tl_tr_bl_br': quads, 'max_channel_delta': maxd, 'bbox': box}


def compare_runs(entry):
    hashes = {r['role']: r.get('sha256') for r in entry['runs']}
    payloads = {r['role']: r.pop('_payload', None) for r in entry['runs']}
    for role in payloads:
        if role != 'reference' and payloads[role] and payloads['reference'] and hashes[role] != hashes['reference']:
            entry.setdefault('difference', {})[role] = describe(payloads['reference'], payloads[role])
            print(entry['case'], role, 'difference', entry['difference'][role], flush=True)
    entry['captures_valid'] = all(r['valid'] for r in entry['runs'])
    entry['identical'] = hashes['reference'] is not None and hashes['reference'] == hashes['candidate']
    if 'reference-repeat' in hashes:
        entry['reference_deterministic'] = hashes['reference'] is not None and hashes['reference'] == hashes['reference-repeat']
    if 'candidate-repeat' in hashes:
        entry['candidate_deterministic'] = hashes['candidate'] is not None and hashes['candidate'] == hashes['candidate-repeat']
    entry['passed'] = (entry['captures_valid'] and entry['identical'] and
                       entry.get('reference_deterministic', True) and entry.get('candidate_deterministic', True))


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('report', type=Path)
    parser.add_argument('reference')
    parser.add_argument('candidate')
    parser.add_argument('cases', nargs='*')
    parser.add_argument('--maps', type=Path, default=Path(os.environ.get('H2_MAPS') or MAPS),
                        help='MCC map directory (default: H2_MAPS or the legacy imported cache)')
    args = parser.parse_intermixed_args(argv)
    if 'HALO2_TEST_SCRATCH' not in os.environ:
        parser.error('run through tools/run_test.py to own capture storage')
    try:
        reference, reference_env = runtime_spec(args.reference)
        candidate, candidate_env = runtime_spec(args.candidate)
        cases = [(case, tuple(map(int, case.split(':')))) for case in (args.cases or DEFAULT_CASES)]
        if any(len(values) != 4 or not 1 <= values[0] <= 4 or values[1] <= 0 or
               values[2] < 0 or values[3] < values[2] for _, values in cases):
            raise ValueError('cases require players1..4, a positive capture tick and an ordered nonnegative firing interval')
    except ValueError as error:
        parser.error(str(error))
    map_name = os.environ.get('H2_MAP', 'coagulation')
    maps = args.maps.resolve()
    map_path = maps / f'{map_name}.map'
    report_path = args.report.resolve()
    report_path.parent.mkdir(parents=True, exist_ok=True)
    report = {'reference': str(reference), 'reference_env': reference_env, 'candidate': str(candidate),
              'candidate_env': candidate_env, 'map': map_name, 'native_size': list(SIZE),
              'maps': str(maps), 'map_path': str(map_path), 'map_sha256': file_sha256(map_path),
              'reference_sha256': file_sha256(reference), 'candidate_sha256': file_sha256(candidate),
              'helper_sha256': file_sha256(__file__),
              'cases': [], 'passed': False}
    report_path.write_text(json.dumps(report, indent=2) + '\n')
    for index, (case, values) in enumerate(cases):
        runs = [('reference', reference, reference_env), ('candidate', candidate, candidate_env)]
        if index == 0:
            runs.insert(1, ('reference-repeat', reference, reference_env))
            if os.environ.get('H2_REPEAT_CANDIDATE'):
                runs.append(('candidate-repeat', candidate, candidate_env))
        entry = {'case': case, 'runs': []}
        for role, runtime, role_env in runs:
            result = capture(runtime, f'c{index}-{role}', *values, role_env,
                             scratch=os.environ['HALO2_TEST_SCRATCH'], map_name=map_name, maps=maps)
            result['role'] = role
            entry['runs'].append(result)
            print(case, role, result.get('sha256', 'FAILED'), result.get('bytes'), result.get('header'), result['seconds'], 's', flush=True)
        compare_runs(entry)
        report['cases'].append(entry)
        report_path.write_text(json.dumps(report, indent=2) + '\n')
    report['passed'] = all(case['passed'] for case in report['cases'])
    report_path.write_text(json.dumps(report, indent=2) + '\n')
    print('PASSED' if report['passed'] else 'FAILED')
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
