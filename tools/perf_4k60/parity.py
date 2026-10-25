#!/usr/bin/env python3
"""Deterministic frame parity: scripted input, fixed tick cadence, fixed RNG seed.

parity.py REPORT_JSON REFERENCE_RUNTIME CANDIDATE_RUNTIME [case ...]
Each case is players:capture_tick:fire_first:fire_last. Raw captures stay in
HALO2_TEST_SCRATCH (removed by tools/run_test.py); only hashes are reported.
"""
import hashlib, json, os, subprocess, sys, time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
MAPS = ROOT / 'assets/imported/mcc-dire-valley-polish-20261023/halo2/h2_maps_win64_dx11'
scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
report_path = Path(sys.argv[1])
def runtime_spec(text):
    # Optional 'VAR=VALUE,VAR2=VALUE@/path/to/runtime' adds role environment.
    assignments, _, path = text.rpartition('@')
    env = dict(a.split('=', 1) for a in assignments.split(',') if a)
    return Path(path), env
(reference, reference_env), (candidate, candidate_env) = runtime_spec(sys.argv[2]), runtime_spec(sys.argv[3])
cases = sys.argv[4:] or ['4:90:0:0', '4:150:10:110', '1:150:10:110']
size = (3840, 2160)
map_name = os.environ.get('H2_MAP', 'coagulation')


def capture(runtime, name, players, tick, first, last, role_env=None):
    script = scratch / f'{name}.tsv'
    script.write_text('# tick player fire reload\n' + ''.join(
        f'{t} {p} {int(first <= t < last and last > 0)} 0\n' for t in range(tick) for p in range(players)))
    image = scratch / f'{name}.ppm'
    command = ['xvfb-run', '--auto-servernum', '--server-args', f'-screen 0 {size[0]}x{size[1]}x24',
               str(runtime), '--multiplayer', '--map', str(MAPS / f'{map_name}.map'), '--shared-maps', str(MAPS),
               '--local-players', str(players), '--viewport', *map(str, size), '--render-size', *map(str, size),
               '--mp-input-script', str(script), '--mp-capture-tick', str(tick),
               '--mp-reference-tick-cadence', '--mp-reference-render-fraction', '0',
               '--mp-reference-auxiliary-seed', '1', '--screenshot', str(image), '--mute']
    log = scratch / f'{name}.log'
    started = time.time()
    with log.open('w') as out:
        code = subprocess.run(command, cwd=ROOT, stdout=out, stderr=subprocess.STDOUT, timeout=600,
                              env={**{k: v for k, v in os.environ.items() if k != 'WAYLAND_DISPLAY'}, 'SDL_VIDEODRIVER': 'x11', 'HALO2_MENU_FOG_SEED': '1', **(role_env or {})}).returncode
    result = {'name': name, 'exit_code': code, 'seconds': round(time.time() - started, 1)}
    if code or not image.exists():
        result['log_tail'] = log.read_text(errors='replace')[-1500:]
        return result
    payload = image.read_bytes()
    if payload.split(b'\n', 3)[1] != f'{size[0]} {size[1]}'.encode():
        result['wrong_size'] = True
    result['sha256'] = hashlib.sha256(payload).hexdigest()
    result['bytes'] = len(payload)
    result['_payload'] = payload
    result['header'] = payload.split(b'\n', 3)[:3].__repr__()
    image.unlink()
    return result


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


report = {'reference': str(reference), 'reference_env': reference_env, 'candidate': str(candidate), 'candidate_env': candidate_env, 'map': map_name, 'cases': []}
for index, case in enumerate(cases):
    players, tick, first, last = map(int, case.split(':'))
    runs = [('reference', reference, reference_env), ('candidate', candidate, candidate_env)]
    if index == 0:
        runs.insert(1, ('reference-repeat', reference, reference_env))
        if os.environ.get('H2_REPEAT_CANDIDATE'):
            runs.append(('candidate-repeat', candidate, candidate_env))
    entry = {'case': case, 'runs': []}
    for role, runtime, role_env in runs:
        r = capture(runtime, f'c{index}-{role}', players, tick, first, last, role_env)
        r['role'] = role
        entry['runs'].append(r)
        print(case, role, r.get('sha256', 'FAILED'), r.get('bytes'), r.get('header'), r['seconds'], 's', flush=True)
    hashes = {r['role']: r.get('sha256') for r in entry['runs']}
    payloads = {r['role']: r.pop('_payload', None) for r in entry['runs']}
    for role in payloads:
        if role != 'reference' and payloads[role] and payloads['reference'] and hashes[role] != hashes['reference']:
            entry.setdefault('difference', {})[role] = describe(payloads['reference'], payloads[role])
            print(case, role, 'difference', entry['difference'][role], flush=True)
    entry['identical'] = hashes['reference'] is not None and hashes['reference'] == hashes['candidate']
    if 'reference-repeat' in hashes:
        entry['reference_deterministic'] = hashes['reference'] == hashes['reference-repeat']
    report['cases'].append(entry)
    report_path.write_text(json.dumps(report, indent=2) + '\n')
report['passed'] = all(c['identical'] for c in report['cases'])
report_path.write_text(json.dumps(report, indent=2) + '\n')
print('PASSED' if report['passed'] else 'FAILED')
