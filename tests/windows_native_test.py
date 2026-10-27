#!/usr/bin/env python3
"""Run a native Windows test, with an owned Wine prefix when cross-compiling."""
import os
from pathlib import Path
import shutil
import subprocess
import sys


def main():
    command = sys.argv[1:]
    if not command:
        raise ValueError("A Windows test executable is required")
    if sys.platform == "win32":
        return subprocess.run(command, check=False).returncode
    wine = shutil.which("wine")
    if wine is None:
        print("Wine is unavailable for this cross-compiled Windows test")
        return 77
    scratch = os.environ.get("HALO2_TEST_SCRATCH")
    if not scratch:
        raise RuntimeError("Run Windows tests through tools/run_test.py")
    # These console resource checks need no desktop. Avoid starting Wine's
    # shell against the user's display while bootstrapping the private prefix.
    env = os.environ | {"WINEPREFIX": str(Path(scratch) / "wine"),
                        "WINEDEBUG": "-all", "DISPLAY": "",
                        "WAYLAND_DISPLAY": "",
                        "WINEDLLOVERRIDES": "mscoree,mshtml="}
    server = shutil.which("wineserver")
    try:
        # A new disk-backed prefix may take over two minutes to initialize
        # during a parallel engine build. Keep that setup finite and owned.
        return subprocess.run([wine, *command], env=env, check=False, timeout=240).returncode
    finally:
        if server:
            subprocess.run([server, "-k"], env=env, check=False, timeout=15)
            subprocess.run([server, "-w"], env=env, check=False, timeout=15)


if __name__ == "__main__":
    raise SystemExit(main())
