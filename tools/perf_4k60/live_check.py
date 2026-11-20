#!/usr/bin/env python3
"""Live checks for native replacements (docs/native_replacement/REMAINING_TRANSLATED_20261115.md).

Run under tools/run_test.py (HALO2_TEST_SCRATCH holds every raw output; only
small text results are written to --out).

  live_check.py armory BINARY --out DIR ROLE...
      Armory 01a, 300 frames, 1280x720, deterministic capture. Each ROLE is
      NAME or NAME:K=V,K=V (environment). Prints exit, frame hash (16), trace
      hash (12) and every "... verify:" line. The expected hashes are
      5627a51badf47762 / 8a9b58b38bef.

  live_check.py census BINARY --out DIR --reference K=V[,K=V] [--owner HEX ...]
      Armory census (HALO2_CENSUS_NATIVE_BRIDGES=1), one role with the
      reference switches, then the native role. Writes compare.txt: totals per
      frame, the per-routine instructions of each --owner and the largest
      routines of the native role.

  live_check.py sweep BINARY --out DIR [--env K=V ...] [--workers N] [CASE ...]
      Scripted-input sweep (fire, grenade, jump, use, crouch, move, look).
      CASE is mp:<map>:<players> or sp:<map>:<frames>; the default is every
      MP map at 1p and 4p. Keeps one JSON per case: exit, frame hash, every
      verify line and any difference line.

  live_check.py compare DIR_A DIR_B
      Frame hashes of two sweeps, case by case.
"""
import argparse
import concurrent.futures as cf
import hashlib
import json
import os
import re
import subprocess
import sys
import threading
import time
from collections import Counter
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
CACHE = json.loads((ROOT / 'gameplay-runtime-selection.json').read_text())['cache']
MAPS = ROOT / CACHE / 'halo2/h2_maps_win64_dx11'
MP_MAPS = ('ascension backwash beavercreek burial_mounds coagulation colossus containment cyclotron deltatap '
           'derelict dune elongation foundation gemini headlong highplains lockout midship needle '
           'street_sweeper triplicate turf warlock waterworks zanzibar').split()
KEEP = re.compile(r'[^\n]*(?:verify:|\bdiffer|refus|[Ee]rror)[^\n]*')
FRAMES = 300


def scratch():
    path = os.environ.get('HALO2_TEST_SCRATCH')
    if not path:
        sys.exit('Run through tools/run_test.py')
    return Path(path)


def sha(path, length):
    return hashlib.sha256(path.read_bytes()).hexdigest()[:length] if path.exists() else None


def environment(work, extra):
    env = {k: v for k, v in os.environ.items() if k != 'WAYLAND_DISPLAY'}
    env.update(SDL_VIDEODRIVER='x11', SDL_AUDIODRIVER='dummy', HALO2_MENU_FOG_SEED='1',
               XDG_DATA_HOME=str(work / 'data'), XDG_CONFIG_HOME=str(work / 'config'),
               XDG_CACHE_HOME=str(work / 'cache'), HALO2_PIPELINE_CACHE_DIRECTORY=str(work / 'vk'),
               MESA_SHADER_CACHE_DIR=str(work / 'mesa'), __GL_SHADER_DISK_CACHE_PATH=str(work / 'nv'))
    env.update(extra)
    return env


def launch(binary, arguments, work, extra, size, timeout, server=None):
    """Runs the game under Xvfb; returns (exit status or None on timeout, log text)."""
    work.mkdir(parents=True, exist_ok=True)
    display = ['-n', str(server)] if server is not None else ['-a']
    command = ['xvfb-run', *display, '-s', f'-screen 0 {size[0]}x{size[1]}x24', str(binary), *arguments]
    log = work / 'run.log'
    with log.open('wb') as out:
        process = subprocess.Popen(command, stdout=out, stderr=subprocess.STDOUT, env=environment(work, extra),
                                   cwd=str(Path(binary).parent), start_new_session=True)
        try:
            code = process.wait(timeout=timeout)
        except subprocess.TimeoutExpired:
            code = None
            os.killpg(process.pid, 15)
            time.sleep(3)
            try:
                os.killpg(process.pid, 9)
            except ProcessLookupError:
                pass
            process.wait()
    return code, log.read_bytes().decode(errors='replace')


def armory_arguments(work, frames=FRAMES):
    return ['--campaign', '--map', str(MAPS / '01a_tutorial.map'), '--shared-maps', str(MAPS),
            '--graphics-mode', 'Original', '--viewport', '1280', '720', '--frames', str(frames), '--mute',
            '--screenshot', str(work / 'frame.ppm'),
            '--campaign-reference-capture', '.05', '0', '305419896', str(work / 'trace.jsonl')]


def role(text):
    name, _, assignments = text.partition(':')
    return name, dict(a.split('=', 1) for a in assignments.split(',') if a)


def armory(args):
    out = Path(args.out)
    out.mkdir(parents=True, exist_ok=True)
    lines = [f'binary {sha(Path(args.binary), 16)} {args.binary}']
    for name, env in map(role, args.roles):
        work = scratch() / f'armory-{name}'
        code, text = launch(args.binary, armory_arguments(work), work, env, (1280, 720), args.timeout)
        rendered = re.findall(r'Rendered \d+ frames', text)
        validation = len(re.findall(r'VUID-|Validation Error', text))
        lines.append(f'{name} exit={code} {validation} validation-msgs {sha(work / "frame.ppm", 16)} '
                     f'trace={sha(work / "trace.jsonl", 12)} {rendered[-1] if rendered else "no frames"}')
        lines += [f'  {line}' for line in KEEP.findall(text)[:40]]
        if code != 0:
            lines.append('  tail: ' + text[-800:].replace('\n', '\n  '))
        subprocess.run(['rm', '-rf', str(work)])
        print(lines[-1] if code != 0 else '\n'.join(l for l in lines if l.startswith(name)), flush=True)
    (out / 'armory.txt').write_text('\n'.join(lines) + '\n')
    print('\n'.join(lines))


def load_census(path):
    census = Counter()
    for line in path.read_text().splitlines():
        parts = line.split()
        if len(parts) == 2:
            census[int(parts[0], 16)] += int(parts[1])
    return census


def census(args):
    out = Path(args.out)
    out.mkdir(parents=True, exist_ok=True)
    reference = dict(a.split('=', 1) for a in ','.join(args.reference).split(',') if a)
    runs = {}
    for name, env in (('reference', reference), ('native', {})):
        work = scratch() / f'census-{name}'
        counts = work / 'census.txt'
        env = {**env, 'HALO2_INSTRUCTION_CENSUS': str(counts), 'HALO2_REFERENCE_INSTRUCTION_CALLBACKS': '1',
               'HALO2_CENSUS_NATIVE_BRIDGES': '1'}
        code, text = launch(args.binary, armory_arguments(work), work, env, (1280, 720), args.timeout)
        runs[name] = dict(exit=code, frame=sha(work / 'frame.ppm', 16), trace=sha(work / 'trace.jsonl', 12),
                          census=load_census(counts) if counts.exists() else Counter())
        print(f'census {name} exit={code} frame={runs[name]["frame"]} trace={runs[name]["trace"]}', flush=True)
    sys.path.insert(0, str(ROOT / 'tools/perf_4k60'))
    from census_common import CensusIndex
    index = CensusIndex.scan(ROOT / 'src', 'tu15')
    per = {name: index.attribute(run['census']) for name, run in runs.items()}
    lines = [f'binary {sha(Path(args.binary), 16)} {args.binary}', f'reference switches {sorted(reference)}']
    for name, run in runs.items():
        total = sum(run['census'].values())
        lines.append(f'{name}: exit={run["exit"]} frame={run["frame"]} trace={run["trace"]} '
                     f'total {total:,} = {total / FRAMES:,.0f}/frame')
    for owner in args.owner:
        entry = int(owner, 16)
        lines.append(f'  {entry:x}: ' + ', '.join(f'{name} {per[name][entry] / FRAMES:,.1f}/frame' for name in per))
    lines.append('largest routines (native role, per frame, units):')
    for entry, count in per['native'].most_common(args.top):
        units = ','.join(sorted(index.units.get(entry, ()))) if entry is not None else ''
        lines.append(f'  {"unassigned" if entry is None else f"{entry:x}"} {count / FRAMES:,.0f} {units[:100]}')
    (out / 'compare.txt').write_text('\n'.join(lines) + '\n')
    print('\n'.join(lines[:4 + len(args.owner)]))


def script(players, ticks):
    rows = ['# tick player fire reload [grenade yaw pitch move x y jump]\n']
    for t in range(ticks):
        for p in range(players):
            fire = int(60 <= t < 220 or 400 <= t < 480)
            grenade = int(t % 45 == 20)
            words = []
            if t % 60 == 30:
                words.append('jump')
            if (t % 120) < 40:
                words.append('use')
            if t % 150 == 75:
                words.append('crouch')
            mx = 0.3 * ((t // 90) % 3 - 1)
            rows.append(f'{t} {p} {fire} 0 {grenade} {0.015 * t + 1.5 * p:.4f} {0.2 if (t // 100) % 2 else -0.1} '
                        f'move {mx:.1f} 1.0' + ''.join(' ' + w for w in words) + '\n')
    return ''.join(rows)


def sweep(args):
    out = Path(args.out)
    out.mkdir(parents=True, exist_ok=True)
    cases = args.cases or [f'mp:{m}:{n}' for m in MP_MAPS for n in (1, 4)]
    extra = dict(e.split('=', 1) for e in args.env)
    servers = list(range(args.server_base, args.server_base + 2 * args.workers))
    lock = threading.Lock()
    size = (args.width, args.height)

    def run(case):
        kind, name, n = case.split(':')
        n = int(n)
        label = f'{kind}-{name}-{n}'
        work = scratch() / f'sweep-{label}'
        work.mkdir(parents=True, exist_ok=True)
        if kind == 'mp':
            (work / 'input.tsv').write_text(script(n, args.ticks))
            arguments = ['--multiplayer', '--graphics-mode', 'Original', '--map', str(MAPS / f'{name}.map'),
                         '--shared-maps', str(MAPS), '--local-players', str(n), '--viewport', *map(str, size),
                         '--render-size', *map(str, size), '--mp-input-script', str(work / 'input.tsv'),
                         '--mp-capture-tick', str(args.ticks), '--mp-reference-tick-cadence',
                         '--mp-reference-render-fraction', '0', '--mp-reference-auxiliary-seed', '1',
                         '--screenshot', str(work / 'frame.ppm'), '--mute']
        else:
            arguments = ['--campaign', '--map', str(MAPS / f'{name}.map'), '--shared-maps', str(MAPS),
                         '--graphics-mode', 'Original', '--viewport', *map(str, size), '--frames', str(n), '--mute',
                         '--screenshot', str(work / 'frame.ppm'),
                         '--campaign-reference-capture', '.05', '0', '305419896', str(work / 'trace.jsonl')]
        with lock:
            server = servers.pop(0)
        started = time.monotonic()
        try:
            code, text = launch(args.binary, arguments, work, extra, size, args.timeout, server)
        finally:
            with lock:
                servers.append(server)
        kept = KEEP.findall(text)
        verify = [line for line in kept if 'verify:' in line]
        differs = [line for line in kept if 'differ' in line and 'verify:' not in line and 'differed=0' not in line]
        result = dict(case=case, exit=code, seconds=round(time.monotonic() - started, 1),
                      frame=sha(work / 'frame.ppm', 16), verify=verify[-8:], differs=differs[:40],
                      rendered=re.findall(r'Rendered \d+ frames', text)[-1:],
                      tail=text[-1500:] if code != 0 else '')
        (out / f'{label}.json').write_text(json.dumps(result, indent=1))
        subprocess.run(['rm', '-rf', str(work)])
        print(f'{label} exit={code} t={result["seconds"]} frame={result["frame"]} differs={len(differs)} '
              + ' | '.join(verify[-3:]), flush=True)
        return result

    with cf.ThreadPoolExecutor(args.workers) as pool:
        results = list(pool.map(run, cases))
    summary = dict(binary=str(args.binary), binary_sha=sha(Path(args.binary), 16), env=extra,
                   cases=len(results), exit_zero=sum(r['exit'] == 0 for r in results),
                   differing_cases=[r['case'] for r in results if r['differs']],
                   failed=[(r['case'], r['exit']) for r in results if r['exit'] != 0])
    (out / 'summary.json').write_text(json.dumps(summary, indent=1))
    print(json.dumps(summary, indent=1))


def compare(args):
    a, b = Path(args.a), Path(args.b)
    same = differ = 0
    for path in sorted(a.glob('*-*.json')):
        if path.name == 'summary.json' or not (b / path.name).exists():
            continue
        x, y = json.loads(path.read_text()), json.loads((b / path.name).read_text())
        equal = x['frame'] == y['frame'] and x['frame'] is not None
        same += equal
        differ += not equal
        print(f'{"same" if equal else "DIFF"} {x["case"]} {x["frame"]} {y["frame"]} exit {x["exit"]}/{y["exit"]}')
    print(f'{same} identical, {differ} different')


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    commands = parser.add_subparsers(dest='command', required=True)
    p = commands.add_parser('armory')
    p.add_argument('binary')
    p.add_argument('--out', required=True)
    p.add_argument('--timeout', type=int, default=900)
    p.add_argument('roles', nargs='+')
    p.set_defaults(run=armory)
    p = commands.add_parser('census')
    p.add_argument('binary')
    p.add_argument('--out', required=True)
    p.add_argument('--reference', action='append', default=[])
    p.add_argument('--owner', action='append', default=[])
    p.add_argument('--top', type=int, default=60)
    p.add_argument('--timeout', type=int, default=1800)
    p.set_defaults(run=census)
    p = commands.add_parser('sweep')
    p.add_argument('binary')
    p.add_argument('--out', required=True)
    p.add_argument('--env', action='append', default=[])
    p.add_argument('--workers', type=int, default=2)
    p.add_argument('--ticks', type=int, default=600)
    p.add_argument('--width', type=int, default=960)
    p.add_argument('--height', type=int, default=540)
    p.add_argument('--timeout', type=int, default=1800)
    p.add_argument('--server-base', type=int, default=420)
    p.add_argument('cases', nargs='*')
    p.set_defaults(run=sweep)
    p = commands.add_parser('compare')
    p.add_argument('a')
    p.add_argument('b')
    p.set_defaults(run=compare)
    args = parser.parse_args()
    if getattr(args, 'binary', None):
        args.binary = str(Path(args.binary).resolve())
    args.run(args)


if __name__ == '__main__':
    main()
