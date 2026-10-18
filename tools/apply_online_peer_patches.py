"""Apply the repository's reviewed fixes to the pinned native peer stack.

Refuse conflicts; never reset or overwrite unrelated dependency edits.
"""
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
source = Path(sys.argv[1]).resolve() / "deps/usrsctp"
patches = ("usrsctp-freed-window-atomic.patch", "usrsctp-free-chunks-atomic.patch",
           "usrsctp-callout-flags-lock.patch")


def check(patch, *args):
    return subprocess.run(["git", "-C", str(source), "apply", *args, str(patch)],
                          capture_output=True, text=True)


for name in patches:
    patch = ROOT / "cmake/patches" / name
    if check(patch, "--reverse", "--check").returncode == 0:
        print("Pinned SCTP patch already applied:", name)
        continue
    result = check(patch, "--check")
    if result.returncode:
        raise SystemExit("Pinned SCTP patch conflicts; dependency changes were preserved.\n" + result.stderr)
    result = check(patch)
    if result.returncode:
        raise SystemExit("Could not apply pinned SCTP patch.\n" + result.stderr)
    print("Applied pinned SCTP patch:", name)
