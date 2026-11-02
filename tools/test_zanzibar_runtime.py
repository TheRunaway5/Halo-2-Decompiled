#!/usr/bin/env python3
"""Replay finite MCC Zanzibar views and retain bounded CPU/GPU timing receipts.

Run through tools/run_test.py. Native captures, input tapes, caches and timing
tables stay in HALO2_TEST_SCRATCH; only the explicit report and bounded log leave.
"""
import argparse
import csv
import hashlib
import json
import os
from pathlib import Path
import statistics
import subprocess


def sha(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def timings(path, warmup):
    if not path.is_file():
        return {'frames': 0, 'sampled_frames': 0, 'phases': {}, 'dimensions': {}, 'work': {}}
    with path.open() as stream:
        rows = [row for row in csv.DictReader(stream, delimiter='\t')
                if None not in row and all(value is not None for value in row.values())]
    sample = rows[warmup:]
    phases = {}
    if sample:
        for key in sample[0]:
            if key.endswith('_ms'):
                values = sorted(float(row[key]) for row in sample)
                phases[key] = dict(mean=statistics.mean(values), median=statistics.median(values),
                                   p95=values[int(.95 * (len(values) - 1))])
    dimensions = {key: rows[-1][key] for key in
                  ('render_width', 'render_height', 'present_width', 'present_height')
                  if rows and key in rows[-1]}
    work={key:sum(int(row[key]) for row in sample) for key in (sample[0] if sample else {})
          if key.startswith('model_skin_') and not key.endswith(('_ms','_ns'))}
    return dict(frames=len(rows), sampled_frames=len(sample), phases=phases, dimensions=dimensions,work=work)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--runtime', type=Path, required=True)
    parser.add_argument('--mcc-root', type=Path, required=True)
    parser.add_argument('--report-output', type=Path, required=True)
    parser.add_argument('--view', choices=('spawn', 'ocean', 'beach'), default='spawn')
    parser.add_argument('--ticks', type=int, default=180)
    parser.add_argument('--frames', type=int, default=0,
                        help='Ordinary frame loop instead of fixed tick cadence')
    parser.add_argument('--reference-costs', action='store_true')
    parser.add_argument('--reference-optimization',action='append',default=[],
                        choices=('material','collision','machine'),
                        help='Restore one pre-optimization owner for equal-work A/B')
    parser.add_argument('--verify-gpu-skinning',action='store_true')
    parser.add_argument('--timeout', type=float, default=240)
    args = parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):
        parser.error('Use tools/run_test.py')
    if not 60 <= (args.frames or args.ticks) <= 900:
        parser.error('Require a finite 60..900 frame/tick sample')
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    case = scratch / args.report_output.stem
    case.mkdir(exist_ok=False)
    runtime = args.runtime.resolve(strict=True)
    runtime_pin = sha(runtime)
    maps = args.mcc_root.resolve(strict=True) / 'halo2/h2_maps_win64_dx11'
    map_path = maps / 'zanzibar.map'
    command = [str(runtime), '--multiplayer', '--map', str(map_path), '--shared-maps', str(maps),
               '--local-players', '1', '--viewport', '1280', '720', '--render-size', '1280', '720',
               '--graphics-mode', 'Original', '--mute', '--screenshot', str(case / 'frame.ppm')]
    if args.frames:
        command += ['--frames', str(args.frames)]
    else:
        (case / 'inputs.tsv').write_text('')
        command += ['--mp-input-script', str(case / 'inputs.tsv'), '--mp-capture-tick', str(args.ticks),
                    '--mp-reference-auxiliary-seed', '1', '--mp-reference-tick-cadence',
                    '--mp-reference-render-fraction', '1']
    if args.view != 'spawn':
        command += ['--mp-reference-camera', '-10', '23' if args.view == 'beach' else '13', '1',
                    '0', '1', '.03', '0', '-.03', '1']
    env = dict(os.environ, SDL_VIDEODRIVER='x11',
               SDL_GAMECONTROLLER_IGNORE_DEVICES_EXCEPT='0x0000/0x0000',
               HALO2_RENDER_TIMINGS=str(case / 'cpu.tsv'),
               HALO2_RENDER_GPU_TIMINGS=str(case / 'gpu.tsv'),
               HALO2_PIPELINE_CACHE_DIRECTORY=str(scratch / 'pipeline-cache'),
               MESA_SHADER_CACHE_DIR=str(scratch / 'mesa-cache'),
               __GL_SHADER_DISK_CACHE_PATH=str(scratch / 'nvidia-cache'),
               XDG_CONFIG_HOME=str(case / 'preferences/config'),
               XDG_DATA_HOME=str(case / 'preferences/data'),
               XDG_CACHE_HOME=str(case / 'preferences/cache'))
    for key in ('HALO2_REFERENCE_FRESH_SCENERY_POSES', 'HALO2_REFERENCE_UNCACHED_DEPTH_TEXTURE_HASHES',
                'HALO2_REFERENCE_UNCACHED_COLLISION_BOUNDS'):
        env.pop(key, None)
        if args.reference_costs:
            env[key] = '1'
    controls={'material':'HALO2_REFERENCE_FULL_DEPTH_MATERIAL_LOADS',
              'collision':'HALO2_REFERENCE_SERIAL_COLLISION_BUILDS',
              'machine':'HALO2_REFERENCE_FRESH_MACHINE_POSES'}
    for name,key in controls.items():
        env.pop(key,None)
        if name in args.reference_optimization:env[key]='1'
    env.pop('HALO2_VERIFY_GPU_SKINNING',None)
    if args.verify_gpu_skinning:env['HALO2_VERIFY_GPU_SKINNING']='1'
    with (case / 'native.log').open('w') as stream:
        try:
            code = subprocess.run(command, env=env, stdout=stream, stderr=subprocess.STDOUT,
                                  timeout=args.timeout).returncode
        except subprocess.TimeoutExpired:
            code = 124
    cpu, gpu = timings(case / 'cpu.tsv', 30), timings(case / 'gpu.tsv', 30)
    capture = case / 'frame.ppm'
    runtime_unchanged = sha(runtime) == runtime_pin
    receipt = dict(passed=code == 0 and capture.is_file() and cpu['sampled_frames'] > 0 and runtime_unchanged,
                   command=command, exit_code=code, runtime=str(runtime), runtime_sha256=sha(runtime),
                   runtime_unchanged=runtime_unchanged,
                   map=str(map_path), map_sha256=sha(map_path), view=args.view,
                   reference_costs=args.reference_costs, cpu=cpu, gpu=gpu,
                   reference_optimizations=args.reference_optimization,verify_gpu_skinning=args.verify_gpu_skinning,
                   helper_sha256=sha(Path(__file__).resolve()),
                   large_artifacts_retained=[], scratch_owner=str(scratch))
    if capture.is_file():
        receipt['capture'] = dict(path=str(capture), bytes=capture.stat().st_size, sha256=sha(capture))
    log = (case / 'native.log').read_bytes()
    args.report_output = args.report_output.resolve()
    args.report_output.parent.mkdir(parents=True, exist_ok=True)
    args.report_output.with_suffix('.log').write_bytes(log[-256 * 1024:])
    args.report_output.write_text(json.dumps(receipt, indent=2) + '\n')
    interval = cpu['phases'].get('interval_ms', {}).get('mean')
    print(json.dumps(dict(passed=receipt['passed'], exit_code=code, frames=cpu['frames'],
                          mean_frame_ms=interval, fps=1000 / interval if interval else None,
                          capture=str(capture), report=str(args.report_output))))
    return 0 if receipt['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
