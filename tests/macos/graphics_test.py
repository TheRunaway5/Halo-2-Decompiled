#!/usr/bin/env python3
"""Run a relocated copy and prove both graphics libraries came from that app."""
import json
import os
from pathlib import Path
import plistlib
import shutil
import subprocess
import sys

executable, architecture, report_path = Path(sys.argv[1]), sys.argv[2], Path(sys.argv[3])
app = executable.parents[2]
relocated = Path(os.environ['HALO2_TEST_SCRATCH']) / 'Relocated Graphics Check.app'
shutil.copytree(app, relocated, symlinks=True)
name = plistlib.loads((relocated / 'Contents/Info.plist').read_bytes())['CFBundleExecutable']
subprocess.run(['/usr/bin/arch', '-' + architecture, str(relocated / 'Contents/MacOS' / name),
                '--frames', '3', '--report', str(report_path)], check=True)
report = json.loads(report_path.read_text())
assert report['architecture'] == architecture and report['presented_frames'] == 3, report
for key in ('loaded_moltenvk', 'loaded_sdl'):
    assert Path(report[key]).resolve().parent == (relocated / 'Contents/Frameworks').resolve(), report
