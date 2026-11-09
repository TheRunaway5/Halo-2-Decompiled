#!/usr/bin/env python3
"""Same-binary parity and interleaved timing for native machine/FP publication.

Run through tools/run_test.py. Captures, preload libraries and raw timing rows
are disposable; only bounded summaries and the exact commands leave scratch.
This compares the unchanged reference paths, not an independent TU1.5 frame.
"""
import argparse
import csv
import hashlib
import json
import math
import os
from pathlib import Path
import re
import statistics
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parents[1]
MACHINE = 'HALO2_REFERENCE_MACHINE_POSE_PUBLICATION'
FP = 'HALO2_REFERENCE_FP_CONTEXT_PIN'
ROLES = {'reference': {MACHINE: '1', FP: '1'},
         'machine': {FP: '1'}, 'fp': {MACHINE: '1'}, 'native': {}}


def digest(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--runtime', type=Path, required=True)
    parser.add_argument('--maps', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--phase', choices=('verify', 'parity', 'timing'), required=True)
    parser.add_argument('--map', action='append', dest='maps_to_run')
    parser.add_argument('--pairs', type=int, default=3)
    parser.add_argument('--timing-role', action='append', choices=('machine', 'fp', 'native'), dest='timing_roles')
    parser.add_argument('--fixed-ticks', action='store_true', help='require the owned fixed-tick probe cadence')
    parser.add_argument('--max-load', type=float, help='wait for a quiet host and reject timing runs exceeding this load')
    parser.add_argument('--quiet-seconds', type=float, default=20)
    args = parser.parse_args()
    if 'HALO2_TEST_SCRATCH' not in os.environ:
        parser.error('use tools/run_test.py to own generated products')
    if args.pairs < 3:
        parser.error('timing needs at least three interleaved pairs')
    args.runtime = args.runtime.resolve(strict=True)
    args.maps = args.maps.resolve(strict=True)
    args.output = args.output.resolve()
    args.output.mkdir(parents=True, exist_ok=True)
    report = dict(passed=False, phase=args.phase, runtime=str(args.runtime),
                  runtime_sha256=digest(args.runtime), maps=str(args.maps),
                  helper_sha256=digest(Path(__file__)),
                  scratch_owner=os.environ['HALO2_TEST_SCRATCH'], runs=[])
    report_path = args.output / (args.phase + '.json')

    def save():
        report_path.write_text(json.dumps(report, indent=2) + '\n')

    def run(command, env, label):
        if args.phase=='timing' and args.max_load is not None:
            quiet_since=None;announced=False
            while True:
                load=os.getloadavg()[0];now=time.monotonic()
                if load<=args.max_load:
                    if quiet_since is None:quiet_since=now
                    if now-quiet_since>=args.quiet_seconds:break
                else:quiet_since=None
                report['waiting_for_load']=dict(label=label,load=load,limit=args.max_load)
                save()
                if not announced:print('Waiting for quiet host: '+label,flush=True);announced=True
                time.sleep(5)
            report.pop('waiting_for_load',None)
        record = dict(label=label, command=command, environment=env,
                      load_before=os.getloadavg(), exit_code=None)
        report['runs'].append(record)
        save()
        clean = dict(os.environ)
        for name in (MACHINE, FP, 'HALO2_REFERENCE_FP_CONTEXT_MEMORY',
                     'HALO2_NATIVE_FP_CONTEXT_MEMORY', 'HALO2_REFERENCE_FRESH_MACHINE_POSES',
                     'HALO2_VERIFY_MACHINE_POSE_PUBLICATION', 'HALO2_VERIFY_OBJECT_VIEW_REUSE'):
            clean.pop(name, None)
        clean.update(env)
        # Native MCC sound/cache snapshots require disk backing. Keep their
        # unlinked private files in this outer owner's on-disk scratch too.
        clean['XDG_CACHE_HOME'] = str(Path(os.environ['HALO2_TEST_SCRATCH']) / 'native-cache')
        record['snapshot_cache'] = clean['XDG_CACHE_HOME']
        # Each run has its own cleanup owner, including its shader caches.
        owned = [sys.executable, '-B', str(ROOT / 'tools/run_test.py'),
                 '--scratch-root', str(ROOT / '.local/test-scratch'), '--', *command]
        log = Path(os.environ['HALO2_TEST_SCRATCH']) / (label + '.log')
        peak=os.getloadavg()[0]
        with log.open('w') as stream:
            process = subprocess.Popen(owned, env=clean, stdout=stream, stderr=subprocess.STDOUT)
            while True:
                peak=max(peak,os.getloadavg()[0])
                try:result=process.wait(timeout=5);break
                except subprocess.TimeoutExpired:pass
        peak=max(peak,os.getloadavg()[0])
        record.update(exit_code=result, load_after=os.getloadavg(),load_peak=peak)
        if result:
            record['failure_tail'] = log.read_text(errors='replace')[-12000:]
            save()
            raise RuntimeError('validation failed: ' + label)
        if args.phase=='timing' and args.max_load is not None and peak>args.max_load:
            record['sample_accepted']=False;save()
            raise RuntimeError('host load contaminated timing run: '+label)
        record['sample_accepted']=True
        save()
        print(label + ' passed', flush=True)
        return record

    def spec(role, verify=False):
        env = dict(ROLES[role])
        if verify:
            env.update(HALO2_VERIFY_MACHINE_POSE_PUBLICATION='1', HALO2_VERIFY_OBJECT_VIEW_REUSE='1')
        return ','.join(k + '=' + v for k, v in env.items()) + '@' + str(args.runtime)

    try:
        save()
        for map_name in args.maps_to_run or ('coagulation', 'zanzibar'):
            report.setdefault('map_sha256', {})[map_name] = digest(args.maps / (map_name + '.map'))
            if args.phase == 'verify':
                label = map_name + '-verify'
                record = run(['xvfb-run', '--auto-servernum', '--server-args', '-screen 0 1280x720x24',
                              str(ROOT / 'tools/perf_4k60/profile_run.sh'), str(args.runtime),
                              str(args.output), label, '4', '150', ''],
                             dict(H2_MAP=map_name, H2_MAPS=str(args.maps), SDL_VIDEODRIVER='x11',
                                  HALO2_VERIFY_MACHINE_POSE_PUBLICATION='1', HALO2_VERIFY_OBJECT_VIEW_REUSE='1',
                                  H2_KEEP_LINES='Native machine pose publication:'), label)
                lines = (args.output / (label + '.lines.txt')).read_text()
                record['verified_publications'] = max(map(int, re.findall(r'verified=(\d+)', lines)), default=0)
                # Coagulation can have no relevant machine bank. Zanzibar is
                # the declared positive control from the measured audit.
                if map_name == 'zanzibar' and not record['verified_publications']:
                    raise RuntimeError('machine publication verifier never reached a cache hit')
                (args.output / (label + '.timing.tsv')).unlink()
                save()
            elif args.phase == 'parity':
                cohorts = [('static', 'native', ['4:90:0:0', '4:150:10:110', '2:150:10:110', '1:150:10:110'], []),
                           ('motion', 'native', ['4:150:10:110', '2:150:10:110'], ['--motion', '--render-fraction', '0.5']),
                           ('machine', 'machine', ['4:90:0:0'], []),
                           ('fp', 'fp', ['4:150:10:110'], [])]
                for name, role, cases, extra in cohorts:
                    label = map_name + '-' + name
                    output = args.output / (label + '.json')
                    command = [sys.executable, '-B', str(ROOT / 'tools/perf_4k60/parity.py'),
                               str(output), spec('reference'), spec(role, verify=role != 'fp'),
                               *cases, '--maps', str(args.maps), *extra]
                    record = run(command, {'H2_MAP': map_name, 'H2_REPEAT_CANDIDATE': '1'}, label)
                    result = json.loads(output.read_text())
                    record['parity_report'] = str(output)
                    record['cases_passed'] = sum(c['passed'] for c in result['cases'])
                    if not result['passed']:
                        raise RuntimeError('invalid parity result: ' + label)
                    save()
            else:
                measured = {}
                role_names = ['reference', *dict.fromkeys(args.timing_roles or ('machine', 'fp', 'native'))]
                for pair in range(args.pairs):
                    roles = list(role_names)
                    if pair % 2:
                        roles.reverse()
                    elif pair:
                        roles = roles[1:] + roles[:1]
                    for role in roles:
                        label = f'{map_name}-{role}-{pair + 1}'
                        env = dict(ROLES[role], H2_MAP=map_name, H2_MAPS=str(args.maps), H2_CPUMON='1',
                                   SDL_VIDEODRIVER='x11', HALO2_PSTENCIL_CAPTURE_PRESENT_MODE='immediate')
                        if args.fixed_ticks:
                            env.update(HALO2_PROFILE_NATIVE_FP_ADOPT='1',
                                       H2_KEEP_LINES='Native FP adopt CPU:|Multiplayer GPU:')
                        # Isolate presentation from desktop window resizing;
                        # this is diagnostic 4K-internal X11 throughput.
                        record = run(['xvfb-run', '--auto-servernum', '--server-args', '-screen 0 1280x720x24',
                                      str(ROOT / 'tools/perf_4k60/profile_run.sh'), str(args.runtime),
                                      str(args.output), label, '4', '300', ''], env, label)
                        cpu = dict(re.findall(r'(\w+)=(\S+)', (args.output / (label + '.cpu.txt')).read_text()))
                        record['cpu_ms'] = float(cpu['cpu_ms_per_frame'])
                        record['wall_ms'] = float(cpu['wall_ms_per_frame'])
                        if any(not math.isfinite(record[k]) or record[k] <= 0 for k in ('cpu_ms', 'wall_ms')):
                            raise RuntimeError('invalid CPU-monitor interval: ' + label)
                        phases = {}
                        for line in (args.output / (label + '.timing.txt')).read_text().splitlines():
                            match = re.fullmatch(r'(\w+)\s+mean\s+([\d.]+)\s+med\s+([\d.]+)\s+p95\s+([\d.]+)', line)
                            if match:
                                phases[match[1]] = dict(zip(('mean', 'median', 'p95'), map(float, match.groups()[1:])))
                        record['phases'] = phases
                        rows = list(csv.DictReader((args.output / (label + '.timing.tsv')).open(), delimiter='\t'))[30:]
                        record['measured_ticks'] = sum(int(r['host_tick_end']) - int(r['host_tick_begin']) for r in rows)
                        record['measured_frames'] = len(rows)
                        record['tick_endpoint_span'] = int(rows[-1]['host_tick_end']) - int(rows[0]['host_tick_end'])
                        if args.fixed_ticks and record['tick_endpoint_span'] != len(rows)-1:
                            raise RuntimeError('fixed-cadence workload did not advance exactly once per frame: ' + label)
                        if args.fixed_ticks:
                            lines=(args.output/(label+'.lines.txt')).read_text()
                            match=re.search(r'Native FP adopt CPU: calls=(\d+) timed=(\d+) ns=(\d+)',lines)
                            if not match or not int(match[2]) or not int(match[3]):
                                raise RuntimeError('first-person native adoption timing hook was not reached: '+label)
                            record['fp_adoption']=dict(calls=int(match[1]),timed_calls=int(match[2]),cpu_ns=int(match[3]),
                                                       cpu_ns_per_call=int(match[3])/int(match[2]))
                            gpu=re.search(r'Multiplayer GPU: (.+) split_views=(\d+)',lines)
                            if not gpu or int(gpu[2])!=4:
                                raise RuntimeError('missing four-view GPU identity: '+label)
                            record['gpu_name']=gpu[1]
                        measured[pair, role] = record['cpu_ms']
                        # The validated dimensions and summarized phases suffice;
                        # full timing rows are generated products, not retention.
                        (args.output / (label + '.timing.tsv')).unlink()
                        save()
                summary = report.setdefault('timing_summary', {}).setdefault(map_name, {})
                for role in role_names:
                    gains = [measured[p, 'reference'] - measured[p, role] for p in range(args.pairs)]
                    summary[role] = dict(cpu_ms=statistics.mean(measured[p, role] for p in range(args.pairs)),
                                         mean_gain_ms=statistics.mean(gains), paired_gains_ms=gains,
                                         mean_gain_percent=statistics.mean(100 * gains[p] / measured[p, 'reference']
                                                                          for p in range(args.pairs)))
                save()
        report['passed'] = True
    finally:
        save()


if __name__ == '__main__':
    main()
