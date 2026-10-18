#!/usr/bin/env python3
"""Run a command with owned temporary storage, removed even on CTest timeout.

Use @HALO2_TEST_SCRATCH@ in command arguments or environment values for scratch
output paths. stdout/stderr and the command's exit status pass through unchanged.
"""
import argparse
import contextlib
import os
from pathlib import Path
import select
import shutil
import signal
import subprocess
import sys
import tempfile
import traceback

TOKEN = '@HALO2_TEST_SCRATCH@'


@contextlib.contextmanager
def workspace(parent=None, keep=False):
    if parent is not None:
        Path(parent).mkdir(parents=True, exist_ok=True)
    directory = Path(tempfile.mkdtemp(prefix='halo2-test-', dir=parent)).resolve()
    try:
        yield directory
    finally:
        if keep:
            print(f'Retained test artifacts: {directory}', file=sys.stderr, flush=True)
        else:
            # Only this invocation's newly created directory is ever removed.
            shutil.rmtree(directory)


def stop_process_tree(process):
    if os.name != 'posix':
        if process.poll() is None:
            process.terminate()
        try:
            process.wait(timeout=1)
        except subprocess.TimeoutExpired:
            process.kill()
            process.wait()
        return
    # The child owns a new session. Also retire children left by a test that
    # already returned; none may continue writing into a deleted workspace.
    with contextlib.suppress(ProcessLookupError):
        os.killpg(process.pid, signal.SIGTERM)
    try:
        process.wait(timeout=5)
    except subprocess.TimeoutExpired:
        pass
    with contextlib.suppress(ProcessLookupError):
        os.killpg(process.pid, signal.SIGKILL)
    process.wait()


def worker(args):
    if os.name == 'posix':
        import resource
        # A crashing test must not leave a multi-GB core outside its workspace.
        resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
    with workspace(args.scratch_root, args.keep_artifacts) as directory:
        env = {key: value.replace(TOKEN, str(directory)) for key, value in os.environ.items()}
        env.update(TMPDIR=str(directory), TMP=str(directory), TEMP=str(directory),
                   HALO2_TEST_SCRATCH=str(directory), PYTHONDONTWRITEBYTECODE='1')
        if args.keep_artifacts:
            env['HALO2_KEEP_TEST_ARTIFACTS'] = '1'
        command = [value.replace(TOKEN, str(directory)) for value in args.command]
        if args.worker_fd is not None and select.select([args.worker_fd], [], [], 0)[0]:
            return 143  # The owner disappeared before execution started.
        process = subprocess.Popen(command, env=env,
                                   cwd=directory if args.scratch_cwd else None,
                                   start_new_session=os.name == 'posix')
        try:
            while process.poll() is None:
                if args.worker_fd is None:
                    return process.wait()
                if select.select([args.worker_fd], [], [], .05)[0]:
                    if not os.read(args.worker_fd, 1):
                        return 143
            return process.returncode if process.returncode >= 0 else 128 - process.returncode
        finally:
            stop_process_tree(process)


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--scratch-root', type=Path)
    parser.add_argument('--scratch-cwd', action='store_true', help='Run inside the scratch directory')
    parser.add_argument('--keep-artifacts', action='store_true',
                        default=os.environ.get('HALO2_KEEP_TEST_ARTIFACTS', '').lower() in ('1', 'true', 'yes'))
    parser.add_argument('--worker-fd', type=int, help=argparse.SUPPRESS)
    parser.add_argument('command', nargs=argparse.REMAINDER)
    args = parser.parse_args(argv)
    if args.command[:1] == ['--']:
        args.command.pop(0)
    if not args.command:
        parser.error('a command is required after --')
    if args.worker_fd is not None or os.name != 'posix':
        return worker(args)

    # CTest kills the descendant tree on timeout, even across new sessions.
    # Reparent the supervisor before launching the command so it survives that
    # kill. It owns scratch and watches a pipe held only by this parent. A
    # second pipe returns the real exit code after cleanup has finished.
    read_fd, write_fd = os.pipe()
    result_fd, status_fd = os.pipe()
    previous = {}
    def interrupted(number, _frame):
        raise SystemExit(128 + number)
    try:
        for number in (signal.SIGINT, signal.SIGTERM):
            previous[number] = signal.signal(number, interrupted)
        pid = os.fork()
        if pid == 0:
            os.close(write_fd)
            os.close(result_fd)
            try:
                os.setsid()
                if os.fork():
                    os._exit(0)
                args.worker_fd = read_fd
                try:
                    result = worker(args)
                except BaseException:
                    traceback.print_exc()
                    result = 1
                sys.stdout.flush()
                sys.stderr.flush()
                os.write(status_fd, str(result).encode('ascii'))
            finally:
                os._exit(0)
        os.close(status_fd)
        status_fd = None
        os.close(read_fd)
        read_fd = None
        os.waitpid(pid, 0)
        status = os.read(result_fd, 32)
        return int(status) if status else 1
    finally:
        os.close(write_fd)
        if read_fd is not None:
            os.close(read_fd)
        if status_fd is not None:
            os.close(status_fd)
        # On a handled interruption, wait until the owner-EOF cleanup finishes.
        with contextlib.suppress(OSError):
            os.read(result_fd, 32)
        os.close(result_fd)
        for number, handler in previous.items():
            signal.signal(number, handler)


if __name__ == '__main__':
    raise SystemExit(main())
