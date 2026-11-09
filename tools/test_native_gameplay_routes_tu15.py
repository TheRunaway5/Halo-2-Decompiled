#!/usr/bin/env python3
"""Check real CLI gameplay admission with read-only native entry breakpoints.

Run through tools/run_test.py. This observes route ownership and completed
native world phases; it does not establish all-map gameplay parity.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import signal
import shutil
import subprocess


def digest(path: Path) -> str:
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--runtime', type=Path, required=True)
    parser.add_argument('--source', type=Path, required=True)
    parser.add_argument('--inventory', type=Path)
    parser.add_argument('--multiplayer-only', action='store_true')
    parser.add_argument('--maps', type=Path, required=True)
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    if not shutil.which('gdb') or not (args.maps / 'lockout.map').is_file():
        print('SKIP native CLI routes: GDB and prepared MCC Lockout are required')
        return 77
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    environment = dict(os.environ)
    for key, child in [('XDG_CACHE_HOME', 'cache'), ('XDG_CONFIG_HOME', 'config'),
                       ('XDG_DATA_HOME', 'data'), ('MESA_SHADER_CACHE_DIR', 'mesa'),
                       ('__GL_SHADER_DISK_CACHE_PATH', 'nvidia')]:
        path = scratch / child
        path.mkdir(exist_ok=True)
        environment[key] = str(path)
    owner = 'src/app/main.cpp'
    main_digest = digest(args.source / owner)
    inventory = json.loads(args.inventory.read_text()) if args.inventory else None
    if inventory is not None and inventory.get(owner) != main_digest:
        raise ValueError('CLI source must match the exact runtime inventory')
    cases = [
        ('explicit_multiplayer_control', 'lockout', ['--multiplayer', '--simulate', '3'], False),
        ('multiplayer_play', 'lockout', ['--play', '--simulate', '3'], False),
        ('multiplayer_simulate', 'lockout', ['--simulate', '3'], False),
        ('multiplayer_replay', 'lockout', ['--multiplayer', '--mp-replay', '--simulate', '3'], False),
        ('campaign_simulate', '05b_deltatowers', ['--simulate', '1'], True),
    ]
    if args.multiplayer_only:
        cases = [case for case in cases if not case[3]]
    report = {'scope': 'Actual CLI route ownership and native world entry; no pose or gameplay output injection',
              'runtime': str(args.runtime.resolve()), 'runtime_sha256': digest(args.runtime),
              'source_inventory_sha256': digest(args.inventory) if args.inventory else None,
              'source_cohort_verified': inventory is not None,
              'main_source_sha256': main_digest, 'tool_sha256': digest(Path(__file__)),
              'cases': [], 'passed': False}
    for name, map_name, flags, campaign in cases:
        map_path = args.maps / (map_name + '.map')
        map_digest = digest(map_path)
        result_path = scratch / (name + '.json')
        script = scratch / (name + '.gdb')
        script.write_text('set pagination off\nset confirm off\nset breakpoint pending on\n'
                          'set disable-randomization off\npython\n' +
            'import gdb,json\n'
            'counts={"world_phases":0,"campaign_births":0,"multiplayer_births":0}\n'
            'class Count(gdb.Breakpoint):\n'
            ' def __init__(self,symbol,key):\n'
            '  super().__init__(symbol,internal=True);self.key=key\n'
            ' def stop(self):\n'
            '  counts[self.key]+=1;return False\n'
            'Count("halo2::PhysicsLiveWorldTu15::step_objects(halo2::SourceEngineFrameOwnerTu15)","world_phases")\n'
            'Count("halo2::CampaignSourceRuntime::CampaignSourceRuntime", "campaign_births")\n'
            'Count("halo2::MultiplayerPreparedWorld::MultiplayerPreparedWorld", "multiplayer_births")\n'
            'def exited(event):\n'
            ' counts["child_exit_code"]=getattr(event,"exit_code",None)\n'
            'gdb.events.exited.connect(exited)\n'
            'end\nrun\npython\n' +
            'open(' + repr(str(result_path)) + ',"w").write(json.dumps(counts))\nend\n')
        command = ['gdb', '--batch', '-x', str(script), '--args', str(args.runtime),
                   '--map', str(map_path), '--shared-maps', str(args.maps),
                   '--mp-source-object-world', str(scratch / (name + '-world')), '--mute', *flags]
        process = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                   text=True, env=environment, start_new_session=True)
        try:
            output, _ = process.communicate(timeout=180)
            observed = json.loads(result_path.read_text()) if result_path.is_file() else {}
            passed = (process.returncode == 0 and observed.get('child_exit_code') == 0
                      and observed.get('world_phases', 0) > 0
                      and (observed.get('campaign_births', 0) > 0 if campaign else
                           observed.get('multiplayer_births', 0) > 0 and
                           observed.get('campaign_births', 0) == 0))
            record = {'case': name, 'command': command, 'gdb_exit_code': process.returncode,
                      'observed': observed, 'passed': passed, 'log_tail': output[-5000:]}
        except subprocess.TimeoutExpired as error:
            os.killpg(process.pid, signal.SIGTERM)
            try:
                process.communicate(timeout=5)
            except subprocess.TimeoutExpired:
                os.killpg(process.pid, signal.SIGKILL)
                process.communicate()
            record = {'case': name, 'command': command, 'passed': False,
                      'timeout_seconds': error.timeout}
        finally:
            if process.poll() is None:
                os.killpg(process.pid, signal.SIGKILL)
                process.communicate()
        record['map_sha256'] = map_digest
        record['map_unchanged'] = digest(map_path) == map_digest
        record['passed'] = record['passed'] and record['map_unchanged']
        report['cases'].append(record)
    report['runtime_unchanged'] = digest(args.runtime) == report['runtime_sha256']
    report['passed'] = report['runtime_unchanged'] and all(c['passed'] for c in report['cases'])
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({'passed': report['passed'], 'cases': [
        {k: c[k] for k in ('case', 'passed', 'observed') if k in c} for c in report['cases']]}))
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
