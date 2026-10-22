#!/usr/bin/env python3
"""Require actual authored NPC birth and source campaign ticks from MCC inputs.

This is an engine integration probe. It imports/converts no tags, supplies no
actor records, controls, paths or gameplay outcomes, and retains no client,
memory bank or capture. Headless results do not qualify rendered combat/co-op.
Run through tools/run_test.py; raw process traces live only in its scratch.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import time
from physics_test_cache import runtime_cache, runtime_map


def sha(path):
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--runtime', type=Path, required=True)
    ap.add_argument('--mcc-root', type=Path, required=True)
    ap.add_argument('--map', default='01b_spacestation.map')
    ap.add_argument('--squad', default='1st_hum_johnson')
    ap.add_argument('--ticks', type=int, default=4)
    ap.add_argument('--timeout', type=int, default=120)
    ap.add_argument('--report-output', type=Path, required=True)
    args = ap.parse_args()
    if 'HALO2_TEST_SCRATCH' not in os.environ:
        ap.error('Run this probe through tools/run_test.py')
    if not 1 <= args.ticks <= 1800 or not 1 <= args.timeout <= 600:
        ap.error('Require 1..1800 actual ticks and a 1..600 second timeout')
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    runtime = args.runtime.resolve(strict=True)
    maps, inputs = runtime_cache(args.mcc_root)
    map_path = runtime_map(maps, maps/args.map)
    cache = args.mcc_root.resolve(strict=True)
    command = [str(runtime), '--campaign', '--map', str(map_path),
               '--shared-maps', str(maps), '--place-squad', args.squad,
               '--simulate', str(args.ticks), '--mute']
    env = {k: v for k, v in os.environ.items()
           if not k.startswith('HALO2_') or k == 'HALO2_TEST_SCRATCH'}
    for key in ('XDG_CONFIG_HOME', 'XDG_DATA_HOME', 'XDG_CACHE_HOME'):
        env[key] = str(scratch/key.lower())
    report = dict(passed=False, command=command, runtime_sha256=sha(runtime),
        map_sha256=sha(map_path), mcc_inputs=inputs, scope=__doc__,
        tool_sha256=sha(__file__), scratch=str(scratch), retained_large_artifacts=[],
        full_engine_parity=False, rendered_or_cooperative_acceptance=False)
    trace, log = scratch/'campaign-opens.log', scratch/'campaign-native.log'
    started = time.monotonic()
    try:
        with log.open('w') as stream:
            result = subprocess.run(['strace', '--kill-on-exit', '-f', '-e',
                'trace=openat', '-o', str(trace), '--', *command], cwd=scratch,
                env=env, stdout=stream, stderr=subprocess.STDOUT, timeout=args.timeout)
        report['returncode'] = result.returncode
    except subprocess.TimeoutExpired:
        report['timeout'] = True
    report['elapsed_seconds'] = round(time.monotonic()-started, 3)
    with log.open('rb') as stream:
        stream.seek(max(0, log.stat().st_size-32768))
        text = stream.read().decode('utf-8', errors='replace')
    report['log_tail'] = text
    statuses = re.findall(r'tick=(\d+) bsp=(\d+).*?actors=(\d+).*?script_faults=(\d+)', text)
    report['source_status'] = [dict(zip(('tick', 'bsp', 'actors', 'script_faults'), map(int, row)))
                               for row in statuses]
    report['authored_placement_reported'] = bool(re.search(
        r'Placed [1-9]\d* actors from '+re.escape(args.squad)+r'(?:\n|$)', text))
    opens = trace.read_text(errors='replace')
    paths = re.findall(r'openat\([^,]+, "([^"]+)"', opens)
    report['reference_inputs_opened'] = sorted({p for p in paths
        if p.lower().endswith('.xbe') or (p.lower().endswith('.map') and
        not str(Path(p).resolve()).startswith(str(cache)+'/'))})
    report['cache_write_attempts'] = [line[:800] for line in opens.splitlines()
        if str(cache) in line and re.search(r'O_WRONLY|O_RDWR|O_CREAT|O_TRUNC', line)]
    final = report['source_status'][-1] if report['source_status'] else {}
    report['passed'] = (report.get('returncode') == 0 and
        report['authored_placement_reported'] and final.get('tick') == args.ticks and
        final.get('actors', 0) > 0 and final.get('script_faults') == 0 and
        not report['reference_inputs_opened'] and not report['cache_write_attempts'])
    args.report_output.parent.mkdir(parents=True, exist_ok=True)
    args.report_output.write_text(json.dumps(report, indent=2)+'\n')
    print(json.dumps(dict(passed=report['passed'], runtime=str(runtime),
        source_status=final, report=str(args.report_output), log_tail=text[-1800:])))
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
