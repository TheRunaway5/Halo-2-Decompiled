#!/usr/bin/env python3
"""Configure, build and run tests in a disposable build tree.

Only bounded logs and report.json remain in --output. Use --target and --filter
for focused work; omit them to build halo2_tests and run the full CTest suite.
"""
import argparse
import json
import os
from pathlib import Path
import signal
import subprocess
import sys

from run_test import main as run_with_cleanup

ROOT = Path(__file__).resolve().parents[1]
LOG_LIMIT = 4 * 1024 * 1024


def run_step(command, log):
    print('+ ' + ' '.join(command), flush=True)
    with log.open('w+b') as stream:
        process = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        try:
            while data := process.stdout.read(65536):
                stream.write(data)
                if stream.tell() > 2 * LOG_LIMIT:
                    stream.seek(-LOG_LIMIT, os.SEEK_END)
                    tail = stream.read()
                    stream.seek(0)
                    stream.truncate()
                    stream.write(tail)
                stream.flush()
                sys.stdout.buffer.write(data)
                sys.stdout.buffer.flush()
            return process.wait()
        finally:
            if process.poll() is None:
                process.terminate()
                try:
                    process.wait(timeout=.5)
                except subprocess.TimeoutExpired:
                    process.kill()
                    process.wait()
            process.stdout.close()


def build_and_test(args):
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    build = scratch / 'build'
    for name in ('configure', 'build', 'tests'):
        (args.output / (name + '.log')).unlink(missing_ok=True)
    configure = ['cmake', '-S', str(args.source), '-B', str(build),
                 '-DBUILD_TESTING=ON', f'-DCMAKE_BUILD_TYPE={args.config}']
    if args.sanitizers:
        configure.append('-DHALO2_SANITIZERS=ON')
    configure.extend(args.cmake_arg)
    commands = [
        ('configure', configure),
        ('build', ['cmake', '--build', str(build), '--config', args.config,
                   '--parallel', str(args.jobs), '--target', *(args.target or ['halo2_tests'])]),
        ('tests', ['ctest', '--test-dir', str(build), '--build-config', args.config,
                   '--output-on-failure', '--no-tests=error', '--parallel', str(args.jobs)]),
    ]
    if args.filter:
        commands[-1][1].extend(['--tests-regex', args.filter])
    report = {'passed': False, 'status': 'running', 'build': str(build), 'steps': []}
    def save_report():
        (args.output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    save_report()
    try:
        for name, command in commands:
            log = args.output / (name + '.log')
            step = {'name': name, 'command': command, 'log': str(log), 'exit_code': None}
            report['steps'].append(step)
            save_report()
            result = run_step(command, log)
            step['exit_code'] = result
            save_report()
            if result:
                report['status'] = 'failed'
                return result if result > 0 else 128 - result
        report.update(passed=True, status='passed')
        return 0
    except Exception as error:
        report.update(status='failed', error=str(error))
        raise
    finally:
        if report['status'] == 'running':
            report['status'] = 'interrupted'
        save_report()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, default=ROOT)
    parser.add_argument('--output', type=Path, default=ROOT / 'build/test-results')
    parser.add_argument('--scratch-root', type=Path, default=ROOT / '.local/test-scratch',
                        help='Parent for disposable builds (on disk, rather than /tmp tmpfs)')
    parser.add_argument('--target', action='append', help='Build this target (repeatable)')
    parser.add_argument('--filter', help='CTest test-name regular expression')
    parser.add_argument('--jobs', type=int, default=2)
    parser.add_argument('--config', default='Release', help='Build configuration (default: Release)')
    parser.add_argument('--sanitizers', action='store_true')
    parser.add_argument('--cmake-arg', action='append', default=[])
    parser.add_argument('--keep-artifacts', action='store_true')
    parser.add_argument('--build-worker', action='store_true', help=argparse.SUPPRESS)
    args = parser.parse_args()
    if args.jobs < 1:
        parser.error('--jobs must be positive')
    args.source = args.source.resolve()
    args.output = args.output.resolve()
    args.scratch_root = args.scratch_root.resolve()
    args.output.mkdir(parents=True, exist_ok=True)
    if args.build_worker:
        def interrupted(number, _frame):
            raise SystemExit(128 + number)
        signal.signal(signal.SIGTERM, interrupted)
        signal.signal(signal.SIGINT, interrupted)
        return build_and_test(args)

    # One bounded report directory per invocation; concurrent runs must choose
    # different --output paths instead of mixing each other's logs.
    with (args.output / '.lock').open('w') as lock:
        if os.name == 'posix':
            import fcntl
            try:
                fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
            except BlockingIOError:
                parser.error('another run owns --output; choose a different directory')
        options = ['--scratch-root', str(args.scratch_root)]
        if args.keep_artifacts:
            options.append('--keep-artifacts')
        return run_with_cleanup([*options, '--', sys.executable, str(Path(__file__).resolve()),
                                 '--build-worker', *sys.argv[1:]])


if __name__ == '__main__':
    raise SystemExit(main())
