#!/usr/bin/env python3
"""Replay one retained original capture in disposable test storage.

Run with tools/run_test.py. Retain the small GPU inputs needed for comparison,
their hashes and the exact inspector sources; no duplicate capture is made.
"""
import argparse
import datetime
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys

ROOT = Path(__file__).absolute().parents[1]


def sha(path):
    with path.open('rb') as source:
        return hashlib.file_digest(source, 'sha256').hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--capture', type=Path, required=True)
    parser.add_argument('--parts', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--tool', type=Path, default=ROOT / 'tools/inspect_original_weapon_gpu.py')
    parser.add_argument('--geometry', type=Path)
    args = parser.parse_args()
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    output = args.output.absolute(); assert not output.exists()
    private = scratch / 'inspection'
    inspector = args.tool.absolute()
    capture, parts = args.capture.absolute(), args.parts.absolute()
    inputs = {str(p): sha(p) for p in (inspector, capture, parts)}
    environment = dict(WEAPON_GPU_OUT=str(private), WEAPON_GPU_RDC=str(capture),
        WEAPON_GPU_PARTS=str(parts), WEAPON_GPU_TOOL=str(inspector), LIBGL_ALWAYS_SOFTWARE='1',
        __GLX_VENDOR_LIBRARY_NAME='mesa', __EGL_VENDOR_LIBRARY_FILENAMES='/usr/share/glvnd/egl_vendor.d/50_mesa.json',
        QT_QPA_PLATFORM='xcb', XDG_CACHE_HOME=str(scratch / 'cache'))
    if args.geometry:
        geometry = args.geometry.absolute()
        environment['WEAPON_GPU_GEOMETRY'] = str(geometry)
        inputs[str(geometry)] = sha(geometry)
    env = dict(os.environ, **environment)
    command = ['xvfb-run', '-a', '-s', '-screen 0 640x480x24',
               str(ROOT / '.tools/renderdoc/usr/bin/qrenderdoc'), '--script', str(inspector)]
    log = scratch / 'replay.log'
    with log.open('w') as stream:
        run = subprocess.run(command, env=env, stdout=stream, stderr=subprocess.STDOUT, timeout=240)
    report_file = private / 'report.json'
    passed = run.returncode == 0 and report_file.is_file() and json.loads(report_file.read_text()).get('passed') is True
    if passed:
        report = json.loads(report_file.read_text())
        assert report['capture_sha256'] == inputs[str(capture)]
        assert report['parts_sha256'] == inputs[str(parts)] and report['tool_sha256'] == inputs[str(inspector)]
        shutil.copytree(private, output)
    else:
        output.mkdir(parents=True)
        error = private / 'error.txt'
        if error.is_file(): shutil.copyfile(error, output / 'error.txt')
    (output / 'replay.log').write_bytes(log.read_bytes()[-65536:])
    shutil.copyfile(inspector, output / 'inspection-tool-source.py')
    shutil.copyfile(Path(__file__), output / 'runner-source.py')
    for name, digest in inputs.items(): assert sha(Path(name)) == digest
    files = {str(p.relative_to(output)):dict(bytes=p.stat().st_size, sha256=sha(p)) for p in output.rglob('*') if p.is_file()}
    receipt = dict(passed=passed, timestamp_utc=datetime.datetime.now(datetime.timezone.utc).isoformat(),
        command=command, environment=environment, returncode=run.returncode, source_files_sha256=inputs,
        runner_scratch=str(scratch), capture_copied=False, retained_output=str(output),
        retained_bytes=sum(p['bytes'] for p in files.values()), files=files,
        retention_reason='Exact original GPU inputs and inspector provenance required for native comparisons',
        private_output_removed_by_test_runner=True, full_weapon_parity=False)
    (output / 'retention.json').write_text(json.dumps(receipt, indent=2) + '\n')
    if passed:
        subprocess.run([sys.executable, str(ROOT / 'tools/compact_original_weapon_gpu_inspection.py'), str(output)], check=True)
        receipt['retained_bytes_after_lossless_compaction'] = sum(p.stat().st_size for p in output.rglob('*') if p.is_file())
    print(json.dumps({k:v for k,v in receipt.items() if k not in ('files','environment','source_files_sha256')}), flush=True)
    assert passed, str(output)


if __name__ == '__main__':
    main()
