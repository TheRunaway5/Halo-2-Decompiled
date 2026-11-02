#!/usr/bin/env python3
"""Observe the real build_scene argument counts without supplying engine state.

Use tools/run_test.py. This read-only GDB entry breakpoint observes x86-64
SysV std::span arguments for the aggregate-returning build_scene: vertex
count RDX, material count R8, light count [entry RSP+16]. The emitted ELF
prologue must agree before using this observer on another ABI/build.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import signal
import subprocess
import time

from run_test import stop_process_tree


SYMBOL = ('_ZN5halo212path_tracing11build_sceneESt4spanIKNS_6VertexELm18446744073709551615EE'
          'S1_IKNS_8MaterialELm18446744073709551615EES1_IKNS0_5LightELm18446744073709551615EE')


def sha(path):
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--runtime', type=Path, required=True)
    parser.add_argument('--cache', type=Path, required=True)
    parser.add_argument('--map', default='coagulation.map')
    parser.add_argument('--players', type=int, default=4)
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    if 'HALO2_TEST_SCRATCH' not in os.environ:
        parser.error('Use tools/run_test.py')
    if not 1 <= args.players <= 4 or Path(args.map).name != args.map:
        parser.error('Use1..4 players and an exact map basename')
    root = Path(__file__).resolve().parents[1]
    owned = Path(os.environ['HALO2_TEST_SCRATCH'])
    runtime = args.runtime.resolve(strict=True)
    maps = args.cache.resolve(strict=True)/'halo2/h2_maps_win64_dx11'
    script = owned/'counts.gdb'
    script.write_text('set pagination off\nset confirm off\nset print thread-events off\n'
                     'set breakpoint pending off\nbreak *'+SYMBOL+'\ncommands\nsilent\n'
                     'printf "AUDIT_PT_COUNTS vertices=%llu materials=%llu lights=%llu\\n", '
                     '$rdx, $r8, *(unsigned long long*)($rsp+16)\ncontinue\nend\nrun\n')
    command = [str(runtime), '--multiplayer', '--map', str(maps/args.map), '--shared-maps', str(maps),
               '--local-players', str(args.players), '--graphics-mode', 'PathTracing',
               '--frames', '1', '--hidden', '--mute', '--viewport', '640', '360']
    invocation = ['xvfb-run', '-a', '-s', '-screen 0 1280x720x24 -nolisten tcp',
                  'gdb', '-q', '-batch', '-x', str(script), '--args', *command]
    env = {key: value for key, value in os.environ.items() if not key.startswith('HALO2_')}
    env.update(HALO2_TEST_SCRATCH=str(owned), XDG_CONFIG_HOME=str(owned/'config'),
               XDG_DATA_HOME=str(owned/'data'), XDG_CACHE_HOME=str(owned/'cache'),
               HALO2_PIPELINE_CACHE_DIRECTORY=str(owned/'pipelines'),
               MESA_SHADER_CACHE_DIR=str(owned/'mesa'), __GL_SHADER_DISK_CACHE_PATH=str(owned/'nvidia'),
               SDL_AUDIODRIVER='dummy')
    pin = sha(runtime)
    started = time.monotonic()
    log = owned/'gdb.log'
    def interrupted(number, _frame):
        raise SystemExit(128+number)
    previous = {number: signal.signal(number, interrupted)
                for number in (signal.SIGINT, signal.SIGTERM)}
    with log.open('wb') as stream:
        process = subprocess.Popen(invocation, cwd=root, env=env, stdout=stream,
                                   stderr=subprocess.STDOUT, start_new_session=True)
        try:
            process.wait(timeout=180)
        finally:
            try:
                stop_process_tree(process)
            finally:
                for number, handler in previous.items():
                    signal.signal(number, handler)
    text = log.read_text(errors='replace')
    counts = [dict(zip(('vertices', 'materials', 'lights'), map(int, values)))
              for values in re.findall(r'AUDIT_PT_COUNTS vertices=(\d+) materials=(\d+) lights=(\d+)', text)]
    for row in counts:
        row['triangle_remainder'] = row['vertices'] % 3
        row['complete_triangles'] = row['vertices'] // 3
        row['guard_failures'] = [name for name, failed in (
            ('incomplete_triangle_group', row['vertices'] % 3 != 0),
            ('triangle_capacity', row['vertices'] // 3 > 2_000_000),
            ('material_capacity', row['materials'] > 65536),
            ('light_capacity', row['lights'] > 65536)) if failed]
    report = dict(completed=True, scope=__doc__, runtime=str(runtime), runtime_sha256=pin,
                  runtime_unchanged=sha(runtime) == pin, helper_sha256=sha(__file__),
                  command=command, invocation=invocation, symbol=SYMBOL, players=args.players,
                  map_sha256=sha(maps/args.map), gdb_exit=process.returncode,
                  seconds=round(time.monotonic()-started, 3), counts=counts,
                  native_errors=re.findall(r'^error: (.*)$', text, re.M),
                  rendered_frames=list(map(int, re.findall(r'Multiplayer frames=(\d+)', text))),
                  inferior_exit_lines=[line for line in text.splitlines() if line.startswith('[Inferior')],
                  diagnostic_tail=text[-2200:], log_sha256=sha(log), retained_large_artifacts=[],
                  supplied_engine_outcomes=False)
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2)+'\n')
    print(json.dumps({key: report[key] for key in ('players','counts','native_errors','inferior_exit_lines')}), flush=True)
    return 0 if counts and report['runtime_unchanged'] and process.returncode == 0 else 1


if __name__ == '__main__':
    raise SystemExit(main())
