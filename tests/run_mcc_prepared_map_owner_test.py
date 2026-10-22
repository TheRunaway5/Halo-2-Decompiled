"""Run only the standalone synthetic C++ admission test with shipped scalars."""
from pathlib import Path
import os
import subprocess
import sys
import tempfile

from mcc_native_scalar_fixture import shipped_native_scalar_tables


def main():
    if len(sys.argv) != 2:
        raise ValueError("Supply the standalone mcc_prepared_map_owner_tests executable")
    scratch = Path(os.environ["HALO2_TEST_SCRATCH"])
    if not scratch.is_absolute() or not scratch.is_dir():
        raise ValueError("Run through tools/run_tests.py/tools/run_test.py owned scratch")
    executable = Path(sys.argv[1]).resolve(strict=True)
    with tempfile.TemporaryDirectory(prefix="prepared-owner-scalars-", dir=scratch) as temporary:
        directory = Path(temporary)
        path = directory / "function-tables.bin"
        path.write_bytes(shipped_native_scalar_tables())
        environment = os.environ.copy()
        environment.update(PYTHONDONTWRITEBYTECODE="1", XDG_CACHE_HOME=str(directory / "cache"))
        return subprocess.run([str(executable), str(path)], env=environment, check=False, timeout=60).returncode


if __name__ == "__main__":
    raise SystemExit(main())
