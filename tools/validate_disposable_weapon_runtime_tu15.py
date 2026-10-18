#!/usr/bin/env python3
"""Run the freshly linked application without retaining its disposable build.

This finite headless integration check does not supply weapon inputs or prove
rendering, sound playback, Xbox scheduling, or full weapon parity.
"""
import argparse
import datetime
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import time

ROOT = Path(__file__).resolve().parents[1]


def sha(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def has_writer(path):
    """Do not copy a partially linked ELF while ld still has it open."""
    identity = (path.stat().st_dev, path.stat().st_ino)
    for process in Path('/proc').iterdir():
        if not process.name.isdigit():
            continue
        try:
            for descriptor in (process / 'fd').iterdir():
                try:
                    stat = descriptor.stat()
                    if (stat.st_dev, stat.st_ino) != identity:
                        continue
                    info = (process / 'fdinfo' / descriptor.name).read_text()
                    flags = int(re.search(r'^flags:\s+(\d+)', info, re.M)[1], 8)
                    if flags & os.O_ACCMODE:
                        return True
                except (FileNotFoundError, PermissionError, ProcessLookupError):
                    pass
        except (FileNotFoundError, PermissionError, ProcessLookupError):
            pass
    return False


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build', type=Path, required=True)
    parser.add_argument('--build-report', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--wait-seconds', type=int, default=900)
    args = parser.parse_args()
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    build = args.build.absolute()
    output = args.output.absolute()
    output.mkdir(exist_ok=False)
    runtime = scratch / 'runtime'
    runtime.mkdir()
    result = dict(passed=False, status='waiting for current application link',
                  runner_scratch=str(scratch), build=str(build),
                  timestamp_utc=datetime.datetime.now(datetime.timezone.utc).isoformat(),
                  tool_sha256=sha(Path(__file__)), commands=[],
                  temporary_runtime_files=[], large_artifacts_retained=[],
                  weapon_inputs_supplied=False, renderer_or_PCM_verified=False,
                  full_weapon_parity=False)

    def save():
        (output / 'report.json').write_text(json.dumps(result, indent=2) + '\n')

    save()
    shutil.copyfile(Path(__file__), output / Path(__file__).name)
    try:
        source = build / 'halo2_native'
        deadline = time.monotonic() + args.wait_seconds
        previous = None
        while time.monotonic() < deadline:
            if source.is_file() and os.access(source, os.X_OK):
                stat = source.stat()
                sample = (stat.st_size, stat.st_mtime_ns)
                if sample == previous and stat.st_size and not has_writer(source):
                    break
                previous = sample
            if args.build_report.exists():
                parent = json.loads(args.build_report.read_text())
                if parent['status'] in ('failed', 'interrupted'):
                    raise RuntimeError('The parent build failed before application validation')
            time.sleep(2)
        else:
            raise TimeoutError('Current application did not finish linking within the declared wait')

        def copy(path):
            before = sha(path)
            target = runtime / path.name
            shutil.copy2(path, target)
            assert sha(path) == before == sha(target), path
            result['temporary_runtime_files'].append(dict(
                source=str(path), temporary_path=str(target), bytes=target.stat().st_size,
                sha256=before))

        copy(source)
        dependencies = subprocess.run(['ldd', str(source)], capture_output=True, text=True, check=True)
        (output / 'runtime-dependencies.txt').write_text(dependencies.stdout)
        for value in re.findall(r'=>\s+(/\S+)', dependencies.stdout):
            dependency = Path(value).resolve()
            if dependency.is_relative_to(build):
                copy(dependency)
        environment = os.environ.copy()
        environment['LD_LIBRARY_PATH'] = str(runtime) + ':' + environment.get('LD_LIBRARY_PATH', '')
        binary = runtime / source.name
        xbe = Path('/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe')
        assert sha(xbe) == '21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'
        result['original_xbe_sha256'] = sha(xbe)
        runs = [('help', [str(binary), '--help'], None)]
        for players in (1, 4):
            runs.append((f'headless-{players}-players', [str(binary), '--multiplayer',
                '--map', str(ROOT / 'assets/disc/maps/lockout.map'),
                '--shared-maps', str(ROOT / 'assets/disc/maps'), '--gameplay-xbe', str(xbe),
                '--mp-starting-weapon', '5', '--local-players', str(players),
                '--simulate', '120', '--mute'], f'Multiplayer ticks=120 players={players}'))
        result['status'] = 'running'
        save()
        for name, command, expected in runs:
            print('+ ' + ' '.join(command), flush=True)
            started = time.monotonic()
            with (output / (name + '.log')).open('w+b') as log:
                completed = subprocess.run(command, cwd=ROOT, env=environment,
                                           stdout=log, stderr=subprocess.STDOUT, timeout=180)
                log.seek(0)
                text = log.read().decode(errors='replace')
            record = dict(name=name, command=command, exit_code=completed.returncode,
                          elapsed_seconds=round(time.monotonic() - started, 3),
                          expected=expected, expected_found=expected is None or expected in text)
            result['commands'].append(record)
            save()
            print(json.dumps(record), flush=True)
            if completed.returncode or not record['expected_found']:
                raise RuntimeError(f'{name} did not complete its declared application check')
        result.update(passed=True, status='passed')
        return 0
    except Exception as error:
        result.update(status='failed', error=str(error))
        raise
    finally:
        save()


if __name__ == '__main__':
    raise SystemExit(main())
