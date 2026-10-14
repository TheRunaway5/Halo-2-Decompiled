#!/usr/bin/env python3
"""Run native loading checks against the same selected bank as the launcher."""
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
from audit_rendering import FormatError
from preserve_vista_loading import selected, validate


def main():
    mode, executable, *arguments = sys.argv[1:]
    if mode not in ("assets", "audio"):
        raise ValueError("Unknown selected-loading test mode")
    try:
        directory = selected(ROOT)
    except FormatError:
        # An explicit invalid selection is a failure. A checkout with no owned
        # bank has the same unavailable-source boundary as the original test.
        if (ROOT / "assets/loading-source.tsv").exists():
            raise
        print("SKIP: no prepared user-owned Vista loading bank")
        return 77
    validate(directory)
    if mode == "assets":
        command = [executable, str(directory), *arguments]
    else:
        fixture, *remaining = arguments
        command = [executable, fixture, str(directory / "loading-tone.xbox-adpcm"), *remaining]
    return subprocess.call(command)


if __name__ == "__main__":
    raise SystemExit(main())
