"""Bounded installer I/O; subprocess output never accumulates without a cap."""
import os
import selectors
import subprocess
import time


def stream_command(command, consume, *, output_limit, timeout=900):
    total = errors = 0
    deadline = time.monotonic() + timeout
    with subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE) as process:
        try:
            with selectors.DefaultSelector() as selector:
                selector.register(process.stdout, selectors.EVENT_READ, True)
                selector.register(process.stderr, selectors.EVENT_READ, False)
                while selector.get_map():
                    if time.monotonic() >= deadline:
                        raise ValueError("archive command timed out")
                    for key, _ in selector.select(.1):
                        chunk = os.read(key.fileobj.fileno(), 65536)
                        if not chunk:
                            selector.unregister(key.fileobj)
                        elif key.data:
                            total += len(chunk)
                            if total > output_limit:
                                raise ValueError("archive command output budget exceeded")
                            consume(chunk)
                        else:
                            errors += len(chunk)
                            if errors > 1024 * 1024:
                                raise ValueError("archive command diagnostic budget exceeded")
            process.wait(timeout=max(.1, deadline - time.monotonic()))
            if process.returncode:
                raise ValueError("archive command failed (exit %d)" % process.returncode)
        except BaseException:
            process.kill()
            process.wait()
            raise
    return total


def copy_exact(source, destination, size):
    remaining = size
    while remaining:
        chunk = source.read(min(1024 * 1024, remaining))
        if not chunk:
            raise ValueError("short archive member")
        destination.write(chunk)
        remaining -= len(chunk)
    if source.read(1):
        raise ValueError("archive member exceeds declared size")
