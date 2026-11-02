#!/usr/bin/env python3
"""Reproduce the actual Armory material failure with a pinned native client.

This direct-map invocation is diagnosis only. It does not exercise New Game,
the difficulty menu, the natural opening handoff or native-only acceptance.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import time

SHADER = (r"scenarios\objects\solo\spacestation\ss_exterior_gun" "\\"
          r"ss_exterior_gun_hinge\shaders\panel_mech_tile_ext")
ERROR = "Campaign cinematic model shader is unresolved: " + SHADER
SKY_ERROR = "Sky material/texture semantics differ from the authenticated projection input"


def sha(path):
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--runtime', type=Path, required=True)
    parser.add_argument('--runtime-sha256', required=True)
    parser.add_argument('--borrow-receipt', type=Path, required=True)
    parser.add_argument('--mcc-root', type=Path, required=True)
    parser.add_argument('--report-output', type=Path, required=True)
    parser.add_argument('--timeout', type=int, default=180)
    parser.add_argument('--failure', choices=('machine', 'sky'), default='machine')
    args = parser.parse_args()
    expected_error = ERROR if args.failure == 'machine' else SKY_ERROR
    if 'HALO2_TEST_SCRATCH' not in os.environ:
        parser.error('Run through tools/run_test.py')
    if not re.fullmatch('[0-9a-f]{64}', args.runtime_sha256) or not 1 <= args.timeout <= 300:
        parser.error('Require the admitted runtime SHA256 and1..300 seconds')
    runtime = args.runtime.resolve(strict=True)
    receipt = args.borrow_receipt.resolve(strict=True)
    cache = args.mcc_root.resolve(strict=True)
    maps = cache / 'h2_maps_win64_dx11'
    armory = maps / '01a_tutorial.map'
    if not armory.is_file() or sha(runtime) != args.runtime_sha256:
        parser.error('Actual MCC Armory input or admitted runtime differs')
    if shutil.which('strace') is None:
        parser.error('strace is required to audit actual runtime inputs')
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    log, trace = scratch / 'armory.log', scratch / 'opens.log'
    command = [str(runtime), '--campaign', '--map', str(armory), '--shared-maps',
               str(maps), '--frames', '1', '--hidden', '--mute', '--viewport', '640', '360']
    invocation = ['strace', '--kill-on-exit', '-f', '-e', 'trace=openat',
                  '-o', str(trace), '--', *command]
    env = {key: value for key, value in os.environ.items()
           if not key.startswith('HALO2_') or key == 'HALO2_TEST_SCRATCH'}
    for key in ('XDG_CONFIG_HOME', 'XDG_DATA_HOME', 'XDG_CACHE_HOME'):
        env[key] = str(scratch / key.lower())
    env['SDL_AUDIODRIVER'] = 'dummy'
    if not env.get('DISPLAY'):
        if shutil.which('xvfb-run') is None:
            parser.error('A display or xvfb-run is required')
        invocation = ['xvfb-run', '-a', '-s', '-screen 0 1280x720x24', *invocation]
    report = dict(passed=False, scope=__doc__, exact_shader=SHADER, exact_failure=expected_error,
                  command=command, invocation=invocation, runtime_sha256=args.runtime_sha256,
                  borrow_receipt=str(receipt), borrow_receipt_sha256=sha(receipt),
                  map_sha256=sha(armory), tool_sha256=sha(__file__),
                  owned_scratch=str(scratch), source_or_assets_changed=False,
                  native_only_qualified=False, new_game_or_handoff_qualified=False,
                  retained_large_artifacts=[])
    started = time.monotonic()
    try:
        with log.open('w') as stream:
            result = subprocess.run(invocation, cwd=scratch, env=env, stdout=stream,
                                    stderr=subprocess.STDOUT, timeout=args.timeout)
        report['returncode'] = result.returncode
    except subprocess.TimeoutExpired:
        report['timeout'] = True
    text = log.read_text(errors='replace')
    report.update(elapsed_seconds=round(time.monotonic() - started, 3),
                  exact_failure_reproduced=expected_error in text,
                  previous_machine_shader_guard_cleared=ERROR not in text,
                  armory_started='scenarios\\solo\\01a_tutorial\\01a_tutorial' in text,
                  rendered_one_frame='Rendered 1 frames' in text,
                  log_sha256=sha(log), log_bytes=log.stat().st_size, log_tail=text[-10000:],
                  runtime_unchanged=sha(runtime) == args.runtime_sha256,
                  borrow_receipt_unchanged=sha(receipt) == report['borrow_receipt_sha256'])
    opens = trace.read_text(errors='replace') if trace.is_file() else ''
    paths = re.findall(r'openat\([^,]+, "([^"]+)"', opens)
    report['reference_inputs_opened'] = sorted({path for path in paths if
        path.lower().endswith(('.xbe', '.exe')) or
        (path.lower().endswith('.map') and not Path(path).resolve().is_relative_to(cache))})
    report['cache_write_attempts'] = [line[:800] for line in opens.splitlines()
        if str(cache) in line and re.search(r'O_WRONLY|O_RDWR|O_CREAT|O_TRUNC', line)]
    clean = (report.get('returncode') == 0 and not report.get('timeout')
             and report['runtime_unchanged'] and report['borrow_receipt_unchanged']
             and trace.is_file() and not report['reference_inputs_opened']
             and not report['cache_write_attempts'])
    report['passed'] = bool(clean and report['armory_started'] and
                            report['rendered_one_frame'] and not report['exact_failure_reproduced'])
    args.report_output.parent.mkdir(parents=True, exist_ok=True)
    args.report_output.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({key: report[key] for key in ('passed', 'exact_failure_reproduced',
                      'armory_started', 'rendered_one_frame', 'elapsed_seconds')}))
    if report['passed']:
        return 0
    return 1 if report['exact_failure_reproduced'] else 2


if __name__ == '__main__':
    raise SystemExit(main())
