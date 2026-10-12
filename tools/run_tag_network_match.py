#!/usr/bin/env python3
"""Run a visible full Team Slayer match in independent native host/client processes.

Original maps, an active display and tag_network_match_tests are required.
This verifies the port's native UDP transport; it is not Xbox System Link proof.
"""
import argparse
import csv
import hashlib
import json
from pathlib import Path
import re
import subprocess
import time

ROOT = Path(__file__).resolve().parents[1]


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--native', type=Path, required=True)
    parser.add_argument('--map', type=Path, default=ROOT / 'assets/disc/maps/coagulation.map')
    parser.add_argument('--shared', type=Path, default=ROOT / 'assets/disc/maps')
    parser.add_argument('--xbe', type=Path,
                        default=ROOT / 'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03')
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--timeout', type=float, default=190)
    args = parser.parse_args()
    if not 30 <= args.timeout <= 3600:
        parser.error('--timeout must be between 30 and 3600 seconds')
    binary = args.native.resolve()
    if not binary.is_file():
        parser.error('build tag_network_match_tests first')
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    binary_hash = digest(binary)
    common = [str(binary), str(args.map.resolve()), str(args.shared.resolve()), str(args.xbe.resolve())]
    children = []
    errors = []
    started = time.monotonic()
    deadline = started + args.timeout
    try:
        for role in ('host', 'client'):
            log = output / (role + '.log')
            stream = log.open('w')
            command = common + [role, str(output / role)]
            try:
                process = subprocess.Popen(command, cwd=ROOT, stdout=stream, stderr=subprocess.STDOUT)
            except Exception:
                stream.close()
                raise
            children.append((role, process, stream, command, log))
            print(f'{role}: PID {process.pid}', flush=True)
            if role == 'host':
                ready_deadline = min(deadline, time.monotonic() + 25)
                while 'LAN host UDP port=29179' not in log.read_text():
                    if process.poll() is not None:
                        raise RuntimeError('Host exited before binding its UDP socket')
                    if time.monotonic() >= ready_deadline:
                        raise TimeoutError('Host did not bind its UDP socket within the startup deadline')
                    time.sleep(.1)
        for role, process, stream, command, log in children:
            status = process.wait(timeout=max(.01, deadline - time.monotonic()))
            stream.close()
            if status != 0:
                raise RuntimeError(f'{role} failed with exit status {status}; see {log}')
    except (OSError, RuntimeError, TimeoutError, subprocess.TimeoutExpired) as error:
        errors.append(str(error))
    finally:
        for role, process, stream, command, log in children:
            if process.poll() is None:
                errors.append(f'{role} was still running at failure/deadline; stopped this test child')
                process.terminate()
                try:
                    process.wait(timeout=10)
                except subprocess.TimeoutExpired:
                    process.kill()
                    process.wait(timeout=5)
            stream.close()
    records = []
    for role, process, stream, command, log in children:
        metrics = {}
        match = re.search(r'PASS complete two-process .* match: (.*)', log.read_text())
        if match:
            for key, value in re.findall(r'(\w+)=(-?\d+(?:,-?\d+)?)', match.group(1)):
                metrics[key] = [int(x) for x in value.split(',')] if ',' in value else int(value)
        records.append(dict(role=role, pid=process.pid, command=command,
                            exit_status=process.returncode, log=str(log), measurements=metrics))
    finals = []
    if len(records) == 2 and all(r['exit_status'] == 0 for r in records):
        try:
            for role in ('host', 'client'):
                with (output / role / 'final.tsv').open() as source:
                    finals.append(list(csv.DictReader(source, delimiter='\t')))
        except OSError as error:
            errors.append(str(error))
    equal = len(finals) == 2 and len(finals[0]) == 2 and finals[0] == finals[1]
    if not equal:
        errors.append('Complete host/client final identities and combat records did not match')
    if digest(binary) != binary_hash:
        errors.append('Test executable changed during execution')
    passed = not errors and len(records) == 2 and all(r['measurements'] for r in records)
    report = dict(scope='Two independent visible native tag worlds and render/audio loops; '
                  'actual UDP and SDL input; authored collision/spawns; ordinary variant edits; no injected state',
                  binary_sha256=binary_hash, elapsed_seconds=round(time.monotonic() - started, 3),
                  results=records, final_records_equal=equal, errors=errors, passed=passed)
    (output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(dict(passed=passed, final_records_equal=equal, errors=errors,
                          report=str(output / 'report.json'))), flush=True)
    return 0 if passed else 1


if __name__ == '__main__':
    raise SystemExit(main())
