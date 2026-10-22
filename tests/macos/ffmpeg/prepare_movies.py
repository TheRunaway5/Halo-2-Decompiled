#!/usr/bin/env python3
"""Generate tiny explicit synthetic playback inputs in the private build tree."""
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys


def main():
    executable, output = sys.argv[1], Path(sys.argv[2]).resolve()
    # The enclosing run_tests.py owns the build destination; encoder temporary
    # files and subprocess caches use run_test.py's HALO2_TEST_SCRATCH.
    if 'HALO2_TEST_SCRATCH' not in os.environ:
        raise RuntimeError('Use tools/run_test.py for fixture generation')
    output.mkdir(parents=True, exist_ok=True)
    record = dict(schema='H2_MACOS_SYNTHETIC_MOVIES_V1', commands=[], files={},
        generator_version=subprocess.check_output([executable, '-version'], text=True).splitlines()[:3])
    for name, codec, extra in [('h264', 'libx264', ['-preset', 'ultrafast']),
        ('hevc', 'libx265', ['-preset', 'ultrafast', '-x265-params', 'pools=1:frame-threads=1']),
        ('av1', 'libsvtav1', ['-preset', '12', '-svtav1-params', 'lp=1'])]:
        destination = output / (name + '.mp4')
        command = [executable, '-nostdin', '-hide_banner', '-loglevel', 'error', '-y',
            '-f', 'lavfi', '-i', 'testsrc2=size=64x64:rate=30:duration=0.2',
            '-f', 'lavfi', '-i', 'sine=frequency=440:sample_rate=48000:duration=0.2',
            '-map', '0:v', '-map', '1:a', '-c:v', codec, '-pix_fmt', 'yuv420p',
            *extra, '-threads', '1', '-c:a', 'aac', '-ac', '2', '-movflags', '+faststart', str(destination)]
        subprocess.run(command, check=True, timeout=60)
        record['commands'].append(command)
        record['files'][destination.name] = dict(bytes=destination.stat().st_size,
            sha256=hashlib.sha256(destination.read_bytes()).hexdigest())
    (output / 'manifest.json').write_text(json.dumps(record, indent=2) + '\n')


if __name__ == '__main__':
    main()
