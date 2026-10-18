"""Contain an entire native match job, including every descendant, in 64 MiB.

Linux/systemd is required. Failure to establish or verify the cgroup stops the
test. There is deliberately no automatic fallback or limit increase.
"""
import fcntl
import json
import os
from pathlib import Path
import signal
import subprocess
import sys
import time

CEILING_BYTES = 64 * 1024 * 1024
# Linux can report transient batched charges above memory.max. Leave 4 MiB
# below the user ceiling and independently reject any measured overshoot.
LIMIT_BYTES = 60 * 1024 * 1024
HOST_RESERVE_BYTES = 4 * 1024**3
# Enforce the hard cap without memory.high reclaim delays on each temporary
# snapshot allocation. The RAM and swap limits remain unchanged.
HIGH_LIMIT = 'max'
UNIT = 'halo2-tag-match-validation.service'
MARKER = 'HALO2_TAG_MATCH_MEMORY_UNIT'
DISPLAY_ENV = ('DISPLAY', 'WAYLAND_DISPLAY', 'XAUTHORITY', 'XDG_RUNTIME_DIR',
               'DBUS_SESSION_BUS_ADDRESS', 'PULSE_SERVER', 'SDL_VIDEODRIVER',
               'SDL_AUDIODRIVER', 'SDL_GAMECONTROLLERCONFIG',
               'SDL_GAMECONTROLLER_IGNORE_DEVICES', 'VK_ICD_FILENAMES',
               'VK_DRIVER_FILES')
TEST_STORAGE_ENV = ('TMPDIR', 'TMP', 'TEMP', 'HALO2_TEST_SCRATCH',
                    'HALO2_KEEP_TEST_ARTIFACTS', 'PYTHONDONTWRITEBYTECODE')


def _systemctl(*args):
    return subprocess.run(['systemctl', '--user', *args], text=True,
                          stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=5)


def _properties():
    names = ('LoadState', 'ActiveState', 'SubState', 'Result', 'ExecMainStatus',
             'MemoryPeak', 'MemoryCurrent', 'ControlGroup')
    result = _systemctl('show', UNIT, *['--property=' + name for name in names])
    return dict(line.split('=', 1) for line in result.stdout.splitlines() if '=' in line)


def _limits(group):
    base = Path('/sys/fs/cgroup') / group.lstrip('/')
    return {name: (base / name).read_text().strip() for name in
            ('memory.max', 'memory.high', 'memory.swap.max', 'memory.oom.group')}


def _verify_limits(limits):
    expected = {'memory.max': str(LIMIT_BYTES), 'memory.high': HIGH_LIMIT,
                'memory.swap.max': '0', 'memory.oom.group': '1'}
    if limits != expected:
        raise RuntimeError('Required aggregate memory controls are not active: ' + str(limits))


def enter_memory_budget(output, timeout):
    """Re-enter this script inside the budget; return None only after verification."""
    marker = os.environ.get(MARKER)
    if marker:
        group = Path('/proc/self/cgroup').read_text().strip().split('0::', 1)[1]
        if marker != UNIT or Path(group).name != UNIT:
            raise RuntimeError('Match runner is outside its required memory cgroup')
        _verify_limits(_limits(group))
        return None
    return run_in_memory_budget(
        [sys.executable, str(Path(sys.argv[0]).resolve()), *sys.argv[1:]], output, timeout)


def run_in_memory_budget(command, output, timeout):
    """Serialize jobs and retain the kernel's measured peak and failure result."""
    output = Path(output).resolve()
    output.mkdir(parents=True, exist_ok=True)
    report_path = output / 'memory-budget.json'
    if report_path.exists() or (output / 'report.json').exists():
        print('Refusing to overwrite existing match evidence: ' + str(output), file=sys.stderr)
        return 2
    report = {'limit_bytes': LIMIT_BYTES, 'ceiling_bytes': CEILING_BYTES,
              'accounting_headroom_bytes': CEILING_BYTES - LIMIT_BYTES,
              'high_bytes': None, 'swap_limit_bytes': 0,
              'scope': 'Match runner and all native child processes together',
              'command': command, 'unit': UNIT, 'passed': False, 'errors': []}
    lock = None
    launched = False
    started = time.monotonic()
    try:
        if sys.platform != 'linux':
            raise RuntimeError('Kernel-enforced match budget currently requires Linux/systemd')
        runtime = Path('/run/user') / str(os.getuid())
        lock = (runtime / 'halo2-tag-match-validation.lock').open('a')
        try:
            fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
        except BlockingIOError:
            raise RuntimeError('Another memory-contained match test is already running')
        if _properties().get('LoadState') != 'not-found':
            raise RuntimeError('A prior match test unit is still loaded; refusing a second job')
        info = dict(line.split(':', 1) for line in Path('/proc/meminfo').read_text().splitlines())
        available = int(info['MemAvailable'].split()[0]) * 1024
        report['host_available_bytes_before'] = available
        if available < HOST_RESERVE_BYTES + LIMIT_BYTES:
            raise RuntimeError('Insufficient spare host RAM; reserving at least 4 GiB for other work')
        log = output / 'memory-budget-runtime.log'
        args = ['systemd-run', '--user', '--quiet', '--remain-after-exit', '--unit=' + UNIT,
                '--working-directory=' + str(Path.cwd()),
                '--property=MemoryAccounting=yes', '--property=MemoryMax=' + str(LIMIT_BYTES),
                '--property=MemoryHigh=infinity', '--property=MemorySwapMax=0',
                '--property=OOMPolicy=kill', '--property=KillMode=control-group',
                '--property=TimeoutStopSec=3', '--property=RuntimeMaxSec=' + str(timeout),
                '--property=TasksMax=512', '--property=LimitCORE=0',
                '--property=StandardOutput=append:' + str(log), '--property=StandardError=inherit',
                '--setenv=' + MARKER + '=' + UNIT]
        args += ['--setenv=' + name for name in (*DISPLAY_ENV, *TEST_STORAGE_ENV) if name in os.environ]
        result = subprocess.run(args + ['--', *command], text=True, stdout=subprocess.PIPE,
                                stderr=subprocess.PIPE, timeout=10)
        if result.returncode:
            raise RuntimeError('Cannot establish match memory cgroup: ' + result.stderr.strip())
        launched = True
        print('Match test: aggregate 60 MiB RAM, 64 MiB measured ceiling, zero swap; ' + str(log), flush=True)
        while True:
            props = _properties()
            report['kernel'] = props
            peak = props.get('MemoryPeak', '')
            if peak.isdecimal() and int(peak) > CEILING_BYTES:
                raise RuntimeError('Kernel measured peak exceeds the 64 MiB ceiling: ' + peak)
            if props.get('ControlGroup'):
                try:
                    report['verified_limits'] = _limits(props['ControlGroup'])
                    _verify_limits(report['verified_limits'])
                except FileNotFoundError:
                    # A terminating unit may release its cgroup between show
                    # and this read. Record its actual terminal result next.
                    latest = _properties()
                    report['kernel'] = latest
                    if latest.get('SubState') == 'exited' or latest.get('ActiveState') == 'failed':
                        props = latest
                        break
                    if latest.get('ActiveState') == 'active':
                        raise
                    if time.monotonic() - started > timeout + 10:
                        raise TimeoutError('Contained match job exceeded its deadline')
                    time.sleep(.02)
                    continue
            if props.get('SubState') == 'exited' or props.get('ActiveState') == 'failed':
                break
            if props.get('LoadState') == 'not-found':
                raise RuntimeError('Match cgroup disappeared before its result was recorded')
            if time.monotonic() - started > timeout + 10:
                raise TimeoutError('Contained match job exceeded its deadline')
            time.sleep(.2)
        if 'verified_limits' not in report:
            raise RuntimeError('Contained job ended before required kernel controls were verified')
        peak = props.get('MemoryPeak', '')
        if not peak.isdecimal() or int(peak) <= 0:
            raise RuntimeError('Kernel did not provide a measured memory peak')
        if int(peak) > CEILING_BYTES:
            raise RuntimeError('Kernel measured peak exceeds the 64 MiB ceiling: ' + peak)
        report['passed'] = props.get('Result') == 'success' and props.get('ExecMainStatus') == '0'
        if not report['passed']:
            report['errors'].append('Contained job failed: ' + props.get('Result', 'unknown') +
                                    ', status ' + props.get('ExecMainStatus', 'unknown'))
    except (OSError, RuntimeError, TimeoutError, subprocess.SubprocessError) as error:
        report['errors'].append(str(error))
    except KeyboardInterrupt:
        report['errors'].append('Interrupted; stopping only this match test cgroup')
    finally:
        if launched:
            try:
                stopped = _systemctl('stop', UNIT)
                if stopped.returncode:
                    report['errors'].append('Could not stop owned test unit: ' + stopped.stderr.strip())
                _systemctl('reset-failed', UNIT)
            except (OSError, subprocess.SubprocessError) as error:
                report['errors'].append('Test unit cleanup failed: ' + str(error))
        if lock:
            lock.close()
    report['passed'] = report['passed'] and not report['errors']
    report['elapsed_seconds'] = round(time.monotonic() - started, 3)
    report_path.write_text(json.dumps(report, indent=2) + '\n')
    if not report['passed'] and not (output / 'report.json').exists():
        (output / 'report.json').write_text(json.dumps({
            'passed': False, 'scope': 'Contained job did not complete match validation',
            'errors': report['errors'], 'memory_budget': str(report_path)}, indent=2) + '\n')
    if not report['passed']:
        # CTest scratch is disposable. Forward a bounded native failure log so
        # its useful diagnostics survive in CTest's output after cleanup.
        try:
            with (output / 'memory-budget-runtime.log').open('rb') as stream:
                stream.seek(0, os.SEEK_END)
                stream.seek(max(0, stream.tell() - 65536))
                print(stream.read().decode('utf-8', errors='replace'), flush=True)
        except FileNotFoundError:
            pass
    print(json.dumps({'passed': report['passed'], 'memory_budget': str(report_path),
                      'kernel': report.get('kernel'), 'verified_limits': report.get('verified_limits'),
                      'errors': report['errors']}), flush=True)
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    import argparse
    import uuid
    def interrupted(_number, _frame):
        raise KeyboardInterrupt
    signal.signal(signal.SIGTERM, interrupted)
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output-root', type=Path, required=True)
    parser.add_argument('--timeout', type=float, default=60)
    parser.add_argument('command', nargs=argparse.REMAINDER)
    args = parser.parse_args()
    command = args.command
    if command and command[0] == '--':
        command = command[1:]
    if not command or not 0 < args.timeout <= 7200:
        parser.error('Supply a command and a timeout in (0, 7200] seconds')
    output = args.output_root / str(uuid.uuid4())
    raise SystemExit(run_in_memory_budget(command, output, args.timeout))
