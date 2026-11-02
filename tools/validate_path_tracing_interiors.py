#!/usr/bin/env python3
"""Compare actual native Original/PT at a fixed authored-map interior view.

Run through tools/run_test.py. Raw captures, private preferences, logs and caches
stay in HALO2_TEST_SCRATCH. --retain-captures explicitly keeps the paired images
as required diagnostic references. No launcher preference/runtime is modified.
"""
import argparse
import csv
import hashlib
import json
import math
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import time

import numpy as np
from PIL import Image
from run_tests import validation_environment
from test_player_shadows_native import ppm
from validate_path_tracing_visual import private_preferences, classify_validation_diagnostics, TIMEOUT_DRIVER

ROOT = Path(__file__).resolve().parents[1]
BLUE_LIFT_EYE = (-1.4967865943908691, 12.242735862731934, -9.355700492858887)
RT_PREFIXES = ('include/halo2/path_tracing/', 'src/rendering/path_tracing/', 'shaders/path_tracing/')


def sha(path):
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def energy(pixels):
    rgb = pixels.astype(np.float64)/255.
    luma = rgb @ np.array([.299, .587, .114])
    return {'mean_rgb': rgb.mean(axis=(0, 1)).tolist(), 'mean_luma': float(luma.mean()),
            'median_luma': float(np.median(luma)), 'black_fraction': float(np.mean(luma < 3/255)),
            'dark_fraction': float(np.mean(luma < .03)), 'p90_luma': float(np.percentile(luma, 90))}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--runtime', type=Path, required=True)
    parser.add_argument('--map', type=Path, default=ROOT/'assets/imported/mcc-sky-water-tu15-20261025-v7/halo2/h2_maps_win64_dx11/lockout.map')
    parser.add_argument('--preferences', type=Path, default=Path(os.environ.get('XDG_DATA_HOME') or Path.home()/'.local/share')/'Halo2Native/Halo2/menu.cfg')
    parser.add_argument('--eye', type=float, nargs=3, default=BLUE_LIFT_EYE)
    parser.add_argument('--forward', type=float, nargs=3, default=(0, -1, 0))
    parser.add_argument('--up', type=float, nargs=3, default=(0, 0, 1))
    parser.add_argument('--label', default='blue-lift-interior')
    parser.add_argument('--tick', type=int, default=106)
    parser.add_argument('--width', type=int, default=640)
    parser.add_argument('--height', type=int, default=480)
    parser.add_argument('--timeout-seconds', type=float, default=150,
                        help='Finite native capture limit, at most300 seconds for cold/full-resolution runs')
    parser.add_argument('--report-output', type=Path, required=True)
    parser.add_argument('--retain-captures', type=Path)
    parser.add_argument('--mode', choices=('Original', 'PathTracing'), action='append')
    parser.add_argument('--original-frame', type=Path,
                        help='Hashed paired reference when only PathTracing is recaptured')
    parser.add_argument('--bounces', type=int, choices=(1,2), default=1)
    args = parser.parse_args()
    if not math.isfinite(args.timeout_seconds) or not 0 < args.timeout_seconds <= 300:
        parser.error('Native capture timeout must be finite and greater than0, at most300 seconds')
    if 'HALO2_TEST_SCRATCH' not in os.environ:
        parser.error('Run this helper through tools/run_test.py')
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    runtime, source_map = args.runtime.resolve(), args.map.resolve()
    if not runtime.is_file() or not source_map.is_file():
        parser.error('Native runtime and prepared map must exist')
    preferences = args.preferences.read_bytes() if args.preferences.is_file() else b''
    before = {'runtime': sha(runtime), 'map': sha(source_map), 'preferences': sha(args.preferences) if args.preferences.is_file() else None}
    compiled = json.loads((runtime.parent/'source-hashes.json').read_text())
    previous = json.loads((ROOT/'bin/engine-builds/883a7671d0d84be78ef5e61570036532/source-hashes.json').read_text())
    paths = sorted(name for name in compiled if name.startswith(RT_PREFIXES))
    hashes = {name: {'selected': compiled[name], 'qualified883': previous.get(name),
                     'current': sha(ROOT/name) if (ROOT/name).is_file() else None} for name in paths}
    report = {'label': args.label, 'scope': 'Native Original/PT same fixed camera/source tick; no original-Xbox framebuffer parity claim',
              'camera': {'eye': args.eye, 'forward': args.forward, 'up': args.up}, 'tick': args.tick,
              'runtime': str(runtime), 'input_hashes_before': before, 'rt_source_hashes': hashes, 'captures': []}
    frames = {}
    if args.original_frame:
        with Image.open(args.original_frame) as image:
            frames['Original'] = np.asarray(image.convert('RGB'))
        report['original_reference'] = {'path': str(args.original_frame), 'sha256': sha(args.original_frame)}
    for mode in (args.mode or ('Original', 'PathTracing')):
        directory = Path(tempfile.mkdtemp(prefix=args.label+'-'+mode+'-', dir=scratch))
        config = directory/'data/Halo2Native/Halo2/menu.cfg';config.parent.mkdir(parents=True)
        config.write_bytes(private_preferences(preferences, args.width, args.height, mode, 'Off', 1).replace(
            b'pc_path_tracing 1 1', f'pc_path_tracing 1 {args.bounces}'.encode()));config.chmod(0o600)
        inputs = directory/'input.tsv'
        inputs.write_text('# tick player fire reload grenade raw_yaw raw_pitch\n'+''.join(f'{n} 0 0 0 0 0 0\n' for n in range(args.tick)))
        image, log_path, timing_path = directory/'frame.ppm', directory/'runtime.log', directory/'renderer.tsv'
        command = ['xvfb-run', '-a', '-s', f'-screen 0 {max(800,args.width)}x{max(600,args.height)}x24', str(runtime),
                   '--multiplayer', '--map', str(source_map), '--shared-maps', str(source_map.parent), '--local-players', '1',
                   '--mp-starting-weapon', '5', '--mp-input-script', str(inputs), '--mp-capture-tick', str(args.tick),
                   '--mp-reference-tick-cadence', '--mp-reference-render-fraction', '0', '--mp-reference-auxiliary-seed', '1',
                   '--mp-reference-camera', *map(str, (*args.eye, *args.forward, *args.up)), '--mute', '--hidden', '--validation',
                   '--graphics-mode', mode, '--viewport', str(args.width), str(args.height), '--screenshot', str(image)]
        environment = {key:value for key,value in os.environ.items() if not key.startswith('HALO2_') and key not in ('WAYLAND_DISPLAY', '__NGX_CONF_FILE')}
        environment.update({'HALO2_TEST_SCRATCH': str(directory), 'HALO2_MENU_FOG_SEED': '1', 'XDG_DATA_HOME': str(directory/'data'),
                            'XDG_CONFIG_HOME': str(directory/'config'), 'XDG_CACHE_HOME': str(directory/'cache'),
                            'HALO2_PIPELINE_CACHE_DIRECTORY': str(scratch/'vulkan-cache'), 'MESA_SHADER_CACHE_DIR': str(scratch/'mesa-cache'),
                            '__GL_SHADER_DISK_CACHE_PATH': str(scratch/'nvidia-cache'), 'SDL_VIDEODRIVER': 'x11',
                            'HALO2_RENDER_TIMINGS': str(timing_path)})
        environment, validation = validation_environment(ROOT, environment)
        owned = [sys.executable, '-B', str(ROOT/'tools/run_test.py'), '--scratch-root', str(directory), '--',
                 sys.executable, '-B', '-c', TIMEOUT_DRIVER, str(args.timeout_seconds), *command]
        started = time.monotonic()
        with log_path.open('w') as log:
            result = subprocess.run(owned, cwd=runtime.parent, env=environment, stdout=log, stderr=subprocess.STDOUT)
        with log_path.open(errors='replace') as log:
            diagnostics = classify_validation_diagnostics(log, mode)
        text = log_path.read_text(errors='replace')
        capture = {'mode': mode, 'command': command, 'exit_code': result.returncode, 'validation_discovery': validation,
                   **diagnostics, 'log_sha256': sha(log_path), 'seconds': round(time.monotonic()-started, 3)}
        if timing_path.is_file():
            with timing_path.open() as stream:
                timing_rows = list(csv.DictReader(stream, delimiter='\t'))
            if timing_rows:
                capture['timings'] = {'frames': len(timing_rows),
                                     'first_renderer_ms': float(timing_rows[0]['renderer_ms']),
                                     'last_renderer_ms': float(timing_rows[-1]['renderer_ms']),
                                     'scope': 'Diagnostic CPU/driver wall timing under concurrent system load; no performance acceptance claim'}
        report['captures'].append(capture)
        if result.returncode or diagnostics['validation_errors'] or not image.is_file():
            capture['diagnostic_tail'] = text[-6000:]
            report['passed'] = False;args.report_output.parent.mkdir(parents=True, exist_ok=True)
            args.report_output.write_text(json.dumps(report, indent=2)+'\n')
            raise RuntimeError(f'{mode} native capture failed')
        size, pixels = ppm(image);assert size == (args.width,args.height)
        rgb = np.frombuffer(pixels, dtype=np.uint8).reshape(args.height,args.width,3);frames[mode] = rgb
        capture['image_sha256'] = sha(image)
        capture['frame_energy'] = energy(rgb[args.height//8:args.height*7//8,args.width//8:args.width*7//8])
        capture['path_tracing_receipts'] = [json.loads(line[len('Path Tracing frame: '):]) for line in text.splitlines() if line.startswith('Path Tracing frame: ')][-1:]
        capture['input_receipt'] = next((line[:5000] for line in text.splitlines() if line.startswith('Input capture tick=')), None)
        if args.retain_captures:
            args.retain_captures.mkdir(parents=True, exist_ok=True);target = args.retain_captures/(args.label+'-'+mode+'.png')
            Image.fromarray(rgb).save(target);capture['retained_capture'] = {'path': str(target), 'bytes': target.stat().st_size, 'sha256': sha(target)}
        print(json.dumps({'label': args.label, 'mode': mode, 'energy': capture['frame_energy']}), flush=True)
    original, traced = frames['Original'], frames['PathTracing']
    y0,y1,x0,x1 = args.height//8,args.height*7//8,args.width//8,args.width*7//8
    a,b = original[y0:y1,x0:x1].astype(np.float64)/255.,traced[y0:y1,x0:x1].astype(np.float64)/255.
    la,lb = a@np.array([.299,.587,.114]),b@np.array([.299,.587,.114])
    lit = la >= .08
    report['matched_interior'] = {'roi_pixels': int(la.size), 'original_lit_pixels': int(lit.sum()),
                                  'traced_black_fraction_on_original_lit': float(np.mean(lb[lit] < 3/255)) if lit.any() else None,
                                  'traced_dark_fraction_on_original_lit': float(np.mean(lb[lit] < .05)) if lit.any() else None,
                                  'traced_mean_luma_on_original_lit': float(lb[lit].mean()) if lit.any() else None,
                                  'original_mean_luma_on_original_lit': float(la[lit].mean()) if lit.any() else None,
                                  'mean_luma_retention_on_original_lit': float(lb[lit].mean()/la[lit].mean()) if lit.any() else None}
    # This is a black-interior regression gate, not a demand to preserve baked
    # shadow intensity or match every original pixel. More than half of a
    # substantial source-lit interior cannot disappear into near-black. This
    # deliberately coarse floor also catches blue pixels above the3/255 black
    # threshold whose total intensity has nevertheless collapsed by over90%.
    report['control_valid'] = bool(lit.sum() >= 256)
    report['passed'] = bool(report['control_valid'] and report['matched_interior']['traced_black_fraction_on_original_lit'] < .5 and
                            report['matched_interior']['mean_luma_retention_on_original_lit'] >= .1)
    report['input_hashes_after'] = {'runtime': sha(runtime), 'map': sha(source_map), 'preferences': sha(args.preferences) if args.preferences.is_file() else None}
    report['changed_inputs'] = [name for name,value in before.items() if report['input_hashes_after'][name] != value]
    if report['changed_inputs']:
        report['control_valid'] = report['passed'] = False
        report['invalid_control_reason'] = 'Capture input changed'
    args.report_output.parent.mkdir(parents=True, exist_ok=True);args.report_output.write_text(json.dumps(report, indent=2)+'\n')
    if report['changed_inputs']:
        raise RuntimeError('Capture input changed')
    print(json.dumps({'passed': report['passed'], 'matched_interior': report['matched_interior'], 'report': str(args.report_output)}), flush=True)
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
