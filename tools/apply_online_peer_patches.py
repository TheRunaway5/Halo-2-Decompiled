"""Apply the repository's reviewed fixes to the pinned native peer stack.

Refuse conflicts; never reset or overwrite unrelated dependency edits.
"""
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
source = Path(sys.argv[1]).resolve() / "deps/usrsctp"
patch = ROOT / "cmake/patches/usrsctp-freed-window-atomic.patch"


def check(*args):
    return subprocess.run(["git", "-C", str(source), "apply", *args, str(patch)],
                          capture_output=True, text=True)


if check("--reverse", "--check").returncode == 0:
    print("Pinned SCTP receive-window atomic patch already applied")
else:
    result = check("--check")
    if result.returncode:
        raise SystemExit("Pinned SCTP patch conflicts; dependency changes were preserved.\n" + result.stderr)
    result = check()
    if result.returncode:
        raise SystemExit("Could not apply pinned SCTP patch.\n" + result.stderr)
    print("Applied pinned SCTP receive-window atomic patch")
