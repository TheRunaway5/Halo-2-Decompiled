#!/usr/bin/env python3
"""Owned native Coagulation noise, motion-history and texture checks.

Run through tools/run_test.py. Every native process, private preference copy,
savegame, cache, log and PPM belongs to HALO2_TEST_SCRATCH. Only the explicit
bounded JSON report survives; --retain-captures DIR explicitly retains images.
Each capture starts a fresh match at an exact source tick. A still pair measures
stationary temporal variation. A pan-stop case compares moving, early and
settled results against stationary controls at the SAME ticks and presented
look direction, including a foreground weapon ROI. Both RGB RMS and
the 95th-percentile absolute channel delta gate localized history trails.
Every capture begins with the same neutral raw look samples while the native
unit initializes its delta-input baseline, before applying the intended turn.

Examples (the enclosing owner removes raw captures):
  python3 tools/run_test.py -- python3 -B tools/validate_path_tracing_visual.py \
    --runtime /absolute/build/halo2_native --report-output /absolute/new-report.json \
    --case PathTracing:Off:still --width 1280
  # Omit --case for Original/Off still, PT/Off still and PT/DLSS pan-stop.
  # Add --case PathTracing:FSR:pan-stop to exercise the other vendor backend.

These are native regression metrics, not Xbox framebuffer/aesthetic parity.
Adjacent-frame variation cannot reject spatially stable grain. Inspect retained
Original/PT frames separately for persistent grain and crushed shading. A
matched final yaw also does not remove legitimate pose response to prior turns.
"""
import argparse
import csv
import hashlib
from itertools import chain
import json
import math
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile
import time

import numpy as np
from run_tests import validation_environment
from test_player_shadows_native import ppm

ROOT = Path(__file__).resolve().parents[1]
MAP = ROOT / 'assets/imported/mcc-sky-water-tu15-20261025-v7/halo2/h2_maps_win64_dx11/coagulation.map'
DEFAULT_CASES = ('Original:Off:still', 'PathTracing:Off:still', 'PathTracing:DLSS:pan-stop')
METHOD = {'Off': 0, 'FSR': 1, 'DLSS': 2}
MODE = {'Original': 0, 'PathTracing': 2}
# The measured native capture cohort starts at source tick1. Script row0
# produces tick2; fraction0 presents the previous snapshot. Enforce that
# complete clock contract from every renderer receipt before comparing images.
NATIVE_INITIAL_SOURCE_TICK = 1
DEFAULT_ROIS = ('cliff-left:.28:.26:.46:.51', 'cliff-right:.62:.25:.84:.48',
                'ground-left:.16:.66:.40:.80', 'weapon:.60:.68:.78:.87')
SHADERS = ('path_tracing/world.vert.spv', 'path_tracing/world.frag.spv', 'path_tracing/lighting.frag.spv',
           'path_tracing/depth.frag.spv', 'upscale_prepare.comp.spv', 'upscale_motion.vert.spv', 'upscale_motion.frag.spv')
SOURCES = ('shaders/path_tracing/world.frag', 'shaders/path_tracing/lighting_filter.glsl',
           'shaders/path_tracing/surface.glsl', 'shaders/upscale_prepare.comp',
           'src/rendering/path_tracing/render.cpp', 'include/halo2/path_tracing/render.hpp',
           'src/rendering/renderer/temporal_upscaler.cpp', 'src/rendering/renderer/temporal_geometry.hpp',
           'src/rendering/renderer/renderer.cpp')
TIMEOUT_DRIVER = ('import subprocess,sys\n'
                  'try: result=subprocess.run(sys.argv[2:],timeout=float(sys.argv[1]))\n'
                  'except subprocess.TimeoutExpired:\n'
                  ' print("VISUAL_CAPTURE_TIMEOUT",flush=True);raise SystemExit(124)\n'
                  'raise SystemExit(result.returncode)\n')


def sha(path):
    path = Path(path)
    if not path.is_file():
        return None
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def compiled_source_evidence(runtime, observed):
    # Qualified runtime manifests survive disposable compiler-tree cleanup.
    # Keep their compiled hashes separate from the current checkout observed
    # by this helper; unrelated source work may have continued since the build.
    path = runtime.parent/'source-hashes.json'
    if not path.exists():
        return None
    if path.is_symlink() or not path.is_file() or not 0 < path.stat().st_size <= 8*1024*1024:
        raise RuntimeError('Invalid compiled source manifest')
    pins = json.loads(path.read_text())
    if not isinstance(pins, dict) or any(not isinstance(pins.get(name), str) or
            not re.fullmatch('[0-9a-f]{64}', pins[name]) for name in SOURCES):
        raise RuntimeError('Compiled source manifest lacks the declared RT inputs')
    selected = {name:pins[name] for name in SOURCES}
    return {'path':str(path),'sha256':sha(path),'source_sha256':selected,
            'observed_source_differences':[name for name in SOURCES if observed[name] != selected[name]]}


def parse_case(value):
    fields = value.split(':')
    if len(fields) != 3 or fields[0] not in MODE or fields[1] not in METHOD or fields[2] not in ('still', 'pan-stop'):
        raise argparse.ArgumentTypeError('case must be Original|PathTracing:Off|DLSS|FSR:still|pan-stop')
    return tuple(fields)


def parse_roi(value):
    try:
        name, *edges = value.split(':')
        x0, y0, x1, y1 = map(float, edges)
        if not name or not (0 <= x0 < x1 <= 1 and 0 <= y0 < y1 <= 1):
            raise ValueError()
        return name, (x0, y0, x1, y1)
    except ValueError:
        raise argparse.ArgumentTypeError('ROI requires NAME:X0:Y0:X1:Y1 in normalized image coordinates')


def scripted_yaw(args, sequence, source_tick, reference_yaw=None):
    if sequence == 'still':
        return args.yaw if reference_yaw is None else reference_yaw
    fraction = min(1, max(0, (source_tick-args.pan_start+1)/(args.pan_end-args.pan_start)))
    return args.yaw-args.pan_angle*(1-fraction)


def scripted_look(args, sequence, source_tick, reference_yaw=None):
    # TU1.5 consumes changes between raw PC look samples. Starting captures at
    # different raw angles would establish different baselines, rather than
    # turn their native cameras to the same final direction. Unit creation is
    # later than tick0, so retain the common baseline through its warmup.
    if source_tick < args.look_warmup_ticks:
        return 0.0, 0.0
    return scripted_yaw(args, sequence, source_tick, reference_yaw), args.pitch


def presented_input_tick(capture_tick):
    return capture_tick-NATIVE_INITIAL_SOURCE_TICK-2


def presented_pan_yaw(args, capture_tick):
    return scripted_look(args,'pan',presented_input_tick(capture_tick))[0]


def validate_camera_clock(rows, receipt, capture_tick):
    fields=dict(part.split('=',1) for part in receipt.split() if '=' in part)
    try:
        if int(fields['tick'])!=capture_tick or fields['capture_tick_cadence']!='1' or \
                float(fields['animation_fraction'])!=0 or float(fields['pose_tick'])!=capture_tick-1:
            raise ValueError('capture does not present the previous source snapshot')
        if len(rows)!=capture_tick-NATIVE_INITIAL_SOURCE_TICK:
            raise ValueError('unexpected native initial tick or rendered frame count')
        clock=[]
        for frame,row in enumerate(rows):
            observed={key:int(row[key]) for key in ('host_frame','host_tick_begin','host_tick_end',
                                                   'host_match_step_calls','host_state_projection_calls')}
            expected={'host_frame':frame,'host_tick_begin':frame+NATIVE_INITIAL_SOURCE_TICK,
                      'host_tick_end':frame+NATIVE_INITIAL_SOURCE_TICK+1,
                      'host_match_step_calls':1,'host_state_projection_calls':1}
            if observed!=expected:
                raise ValueError(f'nonconsecutive native clock at rendered frame {frame}: {observed}')
            clock.append(observed)
        if not clock or clock[-1]['host_tick_end']!=capture_tick:
            raise ValueError('native frame sequence does not end at the captured source tick')
    except (KeyError,ValueError,TypeError) as error:
        raise RuntimeError(f'Native camera/input clock contract failed: {error}') from error
    return {'initial_source_tick':NATIVE_INITIAL_SOURCE_TICK,'frames':len(clock),
            'first':clock[0],'last':clock[-1],'rendered_pose_tick':capture_tick-1,
            'effective_input_tick':presented_input_tick(capture_tick),
            'all_frames_one_consecutive_step_and_projection':True}


def classify_validation_diagnostics(output, mode):
    # The RT depth pass does not consume the color-guide varyings. Match the
    # native callback's exact unused-output warning and its next line; a shared
    # location number alone must never hide another interface/validation error.
    unused_output = re.compile(
        r'Vulkan validation: vkCreateGraphicsPipelines\(\): pCreateInfos\[0\] '
        r'\(SPIR-V Interface\) \[VK_SHADER_STAGE_VERTEX_BIT\] has an Output value '
        r'declared at Location (9|10|11) Component 0, but there is no corresponding '
        r'Input declared in \[VK_SHADER_STAGE_FRAGMENT_BIT\]\.')
    benign_continuation = 'This is not invalid, but the write to the unused Output is discarded.'
    lines = output.splitlines() if isinstance(output, str) else (line.rstrip('\r\n') for line in output)
    benign, unexpected = [], []
    previous = None
    for line in chain(lines, (None,)):
        if previous is not None and previous.startswith('Vulkan validation: '):
            match = unused_output.fullmatch(previous) if mode == 'PathTracing' else None
            if match and line == benign_continuation:
                benign.append({'location': int(match[1]), 'diagnostic': previous,
                               'continuation': line})
            else:
                unexpected.append(previous)
        previous = line
    return {'validation_diagnostics': len(benign)+len(unexpected),
            'benign_unused_output_warnings': benign, 'validation_errors': len(unexpected),
            'unexpected_validation_diagnostics': unexpected}


def private_preferences(original, width, height, mode, method, quality):
    # Keep the private copy intact except the explicitly selected video fields.
    # Never include account/profile configuration contents in the report.
    replacements = {'pc_graphics_mode': f'pc_graphics_mode {MODE[mode]}',
                    'pc_upscaling': f'pc_upscaling {METHOD[method]} {quality}',
                    'pc_path_tracing': 'pc_path_tracing 1 1',
                    'pc_video': f'pc_video {width} {height} 0'}
    lines = []
    for line in original.decode('utf-8').splitlines():
        fields = line.split()
        if not fields or fields[0] in replacements:
            continue
        if fields[0] == 'pc_display' and len(fields) == 10:
            fields[1:3] = ['0', '0']  # No inherited internal-resolution override.
            line = ' '.join(fields)
        lines.append(line)
    lines.extend(replacements.values())
    return ('\n'.join(lines) + '\n').encode('utf-8')


def roi_pixels(image, edges, shift=(0, 0), margin=4):
    h, w, _ = image.shape
    x0, y0, x1, y1 = (round(edges[0]*w)+margin, round(edges[1]*h)+margin,
                       round(edges[2]*w)-margin, round(edges[3]*h)-margin)
    dx, dy = shift
    return image[y0+dy:y1+dy, x0+dx:x1+dx].astype(np.float32) / 255


def luma(rgb):
    return rgb[..., 0]*.2126 + rgb[..., 1]*.7152 + rgb[..., 2]*.0722


def texture(rgb):
    light = luma(rgb)
    gradient = math.sqrt(float(np.mean(np.diff(light, axis=0)**2) + np.mean(np.diff(light, axis=1)**2)))
    contrast = float(light.std())
    return {'mean_rgb': [float(value) for value in rgb.mean(axis=(0,1))],
            'mean_luma': float(light.mean()), 'luma_std': contrast, 'gradient_rms': gradient,
            'normalized_detail': gradient/max(contrast, 1e-8), 'dark_fraction_below_8': float((light < 8/255).mean())}


def difference(reference, candidate, rois, max_shift):
    metrics = {}
    for name, edges in rois:
        a = roi_pixels(reference, edges, margin=max_shift+1)
        raw = roi_pixels(candidate, edges, margin=max_shift+1)
        # Match coarse structure, not high-frequency path-traced samples. Keep
        # the raw metric too; a boundary optimum is not accepted as alignment.
        def coarse(rgb):
            gray = luma(rgb)
            h, w = gray.shape
            h -= h % 4; w -= w % 4
            return gray[:h, :w].reshape(h//4, 4, w//4, 4).mean(axis=(1, 3))
        coarse_a = coarse(a)
        best = (float('inf'), 0, 0)
        for dy in range(-max_shift, max_shift+1):
            for dx in range(-max_shift, max_shift+1):
                b = roi_pixels(candidate, edges, (dx, dy), margin=max_shift+1)
                score = float(np.mean((coarse_a-coarse(b))**2))
                trial = (score, abs(dx)+abs(dy), dx, dy)
                current = (best[0], abs(best[1])+abs(best[2]), best[1], best[2])
                if trial < current:
                    best = (score, dx, dy)
        _, dx, dy = best
        b = roi_pixels(candidate, edges, (dx, dy), margin=max_shift+1)
        delta = a-b
        ta, tb = texture(a), texture(b)
        metrics[name] = {'raw_rgb_rms': float(np.sqrt(np.mean((a-raw)**2))),
                         'aligned_rgb_rms': float(np.sqrt(np.mean(delta**2))),
                         'p95_absolute_channel_delta': float(np.percentile(np.abs(delta), 95)),
                         'alignment_pixels': [dx, dy],
                         'alignment_verified': max(abs(dx), abs(dy)) < max_shift,
                         'reference': ta, 'candidate': tb,
                         'detail_retention': tb['normalized_detail']/max(ta['normalized_detail'], 1e-8)}
    return metrics


def receipts(output, prefix):
    return [json.loads(line[len(prefix):]) for line in output.splitlines() if line.startswith(prefix)]


def save_report(path, report):
    payload = json.dumps(report, indent=2, allow_nan=False) + '\n'
    if len(payload.encode()) > 1024*1024:
        raise RuntimeError('Bounded visual report exceeds one MiB')
    path.write_text(payload)


def capture(args, scratch, prefs, mode, method, sequence, tick, label, reference_yaw=None):
    target_yaw=args.yaw if reference_yaw is None else reference_yaw
    if sequence=='still' and presented_input_tick(tick)<args.look_warmup_ticks and (target_yaw!=0 or args.pitch!=0):
        raise ValueError('Nonzero stationary look requires a capture after the neutral look warmup')
    directory = Path(tempfile.mkdtemp(prefix=label+'-', dir=scratch))
    config = directory/'data/Halo2Native/Halo2/menu.cfg'
    config.parent.mkdir(parents=True)
    config.write_bytes(private_preferences(prefs, args.width, args.height, mode, method, args.quality))
    config.chmod(0o600)
    inputs = directory/'input.tsv'
    rows = ['# tick player fire reload grenade raw_yaw raw_pitch']
    for at in range(tick):
        yaw,pitch = scripted_look(args, sequence, at, reference_yaw)
        rows.append(f'{at} 0 0 0 0 {yaw:.9g} {pitch:.9g}')
    inputs.write_text('\n'.join(rows)+'\n')
    image, log_path, timings = directory/'frame.ppm', directory/'runtime.log', directory/'renderer.tsv'
    command = [str(args.runtime), '--multiplayer', '--map', str(args.map), '--shared-maps', str(args.map.parent),
               '--local-players', '1', '--mp-starting-weapon', '5',
               '--mp-input-script', str(inputs), '--mp-capture-tick', str(tick), '--mp-reference-tick-cadence',
               '--mp-reference-render-fraction', '0', '--mp-reference-auxiliary-seed', str(args.seed),
               '--mute', '--graphics-mode', mode, '--viewport', str(args.width), str(args.height), '--screenshot', str(image)]
    if not args.visible:
        command = ['xvfb-run', '-a', '-s', f'-screen 0 {max(800,args.width)}x{max(600,args.height)}x24', *command, '--hidden']
    if not args.no_validation:
        command.append('--validation')
    environment = {k: v for k, v in os.environ.items() if not k.startswith('HALO2_') and k not in ('WAYLAND_DISPLAY', '__NGX_CONF_FILE')}
    environment.update({'HALO2_TEST_SCRATCH': str(directory), 'HALO2_MENU_FOG_SEED': str(args.seed),
                        'HALO2_RENDER_TIMINGS': str(timings), 'XDG_DATA_HOME': str(directory/'data'),
                        'XDG_CONFIG_HOME': str(directory/'config'), 'XDG_CACHE_HOME': str(directory/'cache'),
                        'HALO2_PIPELINE_CACHE_DIRECTORY': str(scratch/'vulkan-cache'),
                        'MESA_SHADER_CACHE_DIR': str(scratch/'mesa-cache'), '__GL_SHADER_DISK_CACHE_PATH': str(scratch/'nvidia-cache'),
                        'SDL_VIDEODRIVER': 'x11'})
    environment, validation_info = validation_environment(ROOT, environment)
    # This nested owner stops the complete native/Xvfb group on timeout before
    # the next capture starts. It cannot leave a timed-out game using the GPU.
    owned = [sys.executable, '-B', str(ROOT/'tools/run_test.py'), '--scratch-root', str(directory), '--',
             sys.executable, '-B', '-c', TIMEOUT_DRIVER, str(args.timeout_seconds), *command]
    result = {'label': label, 'sequence': sequence, 'tick': tick, 'command': command,
              'config_sha256': sha(config), 'inputs_sha256': sha(inputs), 'validation_discovery': validation_info,
              'size': [args.width, args.height], 'valid': False,
              'look_warmup_ticks': args.look_warmup_ticks,
              'first_scripted_look': [float(f'{v:.9g}') for v in scripted_look(args,sequence,0,reference_yaw)],
              'last_scripted_look': [float(f'{v:.9g}') for v in scripted_look(args,sequence,tick-1,reference_yaw)],
              'effective_input_tick':presented_input_tick(tick),
              'effective_presented_look':[float(f'{v:.9g}') for v in
                  scripted_look(args,sequence,presented_input_tick(tick),reference_yaw)]}
    result['last_scripted_yaw']=result['last_scripted_look'][0]
    started = time.monotonic()
    try:
        with log_path.open('w') as log:
            process = subprocess.run(owned, cwd=args.runtime.parent, env=environment, stdout=log, stderr=subprocess.STDOUT)
        result['exit_code'] = process.returncode
        with log_path.open('rb') as log:
            log.seek(max(0, log_path.stat().st_size-512*1024)); output = log.read().decode(errors='replace')
        result['log_sha256'] = sha(log_path)
        # Scan all diagnostics even when receipts use only the bounded log tail.
        with log_path.open(errors='replace') as log:
            result.update(classify_validation_diagnostics(log, mode))
        if process.returncode:
            raise RuntimeError(f'Native capture exited {process.returncode}')
        if result['validation_errors']:
            raise RuntimeError('Unexpected native Vulkan validation diagnostics: '+
                               '\n'.join(result['unexpected_validation_diagnostics'][:4])[:2000])
        capture_lines = [line for line in output.splitlines() if line.startswith('Input capture tick=')]
        if len(capture_lines) != 1 or not capture_lines[0].startswith(f'Input capture tick={tick} ') or 'capture_tick_cadence=1' not in capture_lines[0]:
            raise RuntimeError('Missing exact source-tick/cadence capture receipt')
        result['input_receipt'] = capture_lines[0][:5000]
        expected = [args.width, args.height] if method == 'Off' else [max(1, args.width*100//130), max(1, args.height*100//130)]
        if method != 'Off' and args.quality != 1:
            n, d = ((100,100),(100,130),(2,3),(10,17),(1,2),(1,3))[args.quality]
            expected = [max(1,args.width*n//d), max(1,args.height*n//d)]
        with timings.open() as stream:
            timing_rows = list(csv.DictReader(stream, delimiter='\t'))
        result['camera_input_clock']=validate_camera_clock(timing_rows,capture_lines[0],tick)
        require = timing_rows[-1] if timing_rows else {}
        actual = [int(require.get('render_width', 0)), int(require.get('render_height', 0))]
        presented = [int(require.get('present_width', 0)), int(require.get('present_height', 0))]
        result['rendered_frames'] = len(timing_rows);result['actual_input_size'] = actual;result['actual_output_size'] = presented
        if actual != expected or presented != [args.width,args.height]:
            raise RuntimeError(f'Actual native internal/output extent mismatch: {actual}/{presented}, expected {expected}')
        rt = receipts(output, 'Path Tracing frame: ')
        if mode == 'PathTracing':
            if not rt or not rt[-1].get('completed') or not rt[-1].get('opaque_draws') or not rt[-1].get('gpu_bytes'):
                raise RuntimeError('Path Tracing lacks completed real GPU geometry receipt')
            result['path_tracing'] = rt[-1]
            if (rt[-1].get('samples_per_pixel'), rt[-1].get('max_bounces')) != (1,1):
                raise RuntimeError('Native RT quality did not use the declared one sample/one bounce')
        elif rt:
            raise RuntimeError('Original control unexpectedly submitted a Path Tracing frame')
        upscaling = receipts(output, 'Temporal Upscaling frame: ')
        if not upscaling:
            raise RuntimeError('Missing completed native upscaler/Off dispatch receipt')
        result['upscaling'] = upscaling[-1]
        receipt = upscaling[-1]
        if not receipt.get('completed') or receipt.get('enabled') != (method != 'Off') or \
                receipt.get('method') != METHOD[method] or receipt.get('quality') != args.quality or \
                [receipt.get('input_width'),receipt.get('input_height')] != expected or \
                [receipt.get('output_width'),receipt.get('output_height')] != [args.width,args.height] or \
                (method != 'Off' and receipt.get('frames',0) < 1) or (method == 'Off' and receipt.get('frames') != 0):
            raise RuntimeError('Completed native receipt does not match the requested backend/quality/extents/dispatch count')
        shader_paths = sorted(set(re.findall(r'^Pinned raw world shaders: (.+)$',output,re.MULTILINE)))
        result['observed_shader_directories'] = shader_paths
        if any(Path(path).resolve() != args.shader_dir for path in shader_paths):
            raise RuntimeError('Native source draw used another shader directory than the hashed inputs')
        if image.stat().st_size > args.width*args.height*3+4096:
            raise RuntimeError('Native PPM exceeds declared RGB image extent')
        size, pixels = ppm(image)
        if size != (args.width,args.height):
            raise RuntimeError('Native PPM dimensions differ from declared presentation extent')
        result['image'] = {'sha256': sha(image), 'bytes': image.stat().st_size}
        result['valid'] = True
        result['_pixels'] = np.frombuffer(pixels, dtype=np.uint8).reshape(args.height,args.width,3)
        if args.retain_captures:
            target = args.retain_captures/(label+'.ppm');shutil.copyfile(image,target)
            result['retained_capture'] = {'path': str(target), **result['image']}
    except Exception as error:
        result['error'] = str(error)
        if log_path.exists():
            with log_path.open('rb') as log:
                log.seek(max(0,log_path.stat().st_size-4000));result['diagnostic_tail'] = log.read().decode(errors='replace')
    result['seconds'] = round(time.monotonic()-started,3)
    print(json.dumps({key: result.get(key) for key in ('label','tick','valid','seconds','error')}),flush=True)
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--runtime', type=Path, required=True)
    parser.add_argument('--map', type=Path, help='Explicit prepared MCC map; otherwise --map-name selects the installed imported cache')
    parser.add_argument('--map-name', choices=('coagulation','lockout'), default='coagulation')
    parser.add_argument('--shader-dir', type=Path)
    parser.add_argument('--source-root', type=Path, default=ROOT)
    parser.add_argument('--preferences', type=Path, default=Path(os.environ.get('XDG_DATA_HOME') or Path.home()/'.local/share')/'Halo2Native/Halo2/menu.cfg')
    parser.add_argument('--report-output', type=Path, required=True)
    parser.add_argument('--retain-captures', type=Path, help='Explicit NEW directory to retain only PPM images (never private settings/savegames)')
    parser.add_argument('--case', type=parse_case, action='append')
    parser.add_argument('--roi', type=parse_roi, action='append')
    parser.add_argument('--width', type=int, default=2560)
    parser.add_argument('--height', type=int, help='Default maintains the user 2560x1378 aspect, including --width 1280 ->689')
    parser.add_argument('--quality', type=int, choices=range(6), default=1, help='0 nativeAA,1 UltraQuality,2 Quality,3 Balanced,4 Performance,5 UltraPerformance')
    parser.add_argument('--look-warmup-ticks', type=int, default=30,
                        help='Common neutral raw look samples while the native unit initializes delta input')
    parser.add_argument('--pan-start', type=int, default=60)
    parser.add_argument('--pan-end', type=int, default=90)
    parser.add_argument('--settle-ticks', type=int, default=16)
    parser.add_argument('--pan-angle', type=float, default=.35)
    parser.add_argument('--yaw', type=float, default=0)
    parser.add_argument('--pitch', type=float, default=-.06)
    parser.add_argument('--seed', type=int, default=1)
    parser.add_argument('--timeout-seconds', type=float, default=300)
    parser.add_argument('--max-temporal-rms', type=float, default=.04)
    parser.add_argument('--max-ghost-rms', type=float, default=.05)
    parser.add_argument('--max-temporal-p95', type=float, help='Default twice the temporal RMS bound (absolute-channel95th percentile)')
    parser.add_argument('--max-ghost-p95', type=float, help='Default twice the ghost RMS bound (absolute-channel95th percentile)')
    parser.add_argument('--min-detail-retention', type=float, default=.45)
    parser.add_argument('--max-align-shift', type=int, default=3)
    parser.add_argument('--visible', action='store_true', help='Use existing desktop display instead of owned Xvfb')
    parser.add_argument('--no-validation', action='store_true')
    args = parser.parse_args()
    args.max_temporal_p95 = args.max_temporal_p95 if args.max_temporal_p95 is not None else min(1, args.max_temporal_rms*2)
    args.max_ghost_p95 = args.max_ghost_p95 if args.max_ghost_p95 is not None else min(1, args.max_ghost_rms*2)
    cases = args.case or list(map(parse_case,DEFAULT_CASES))
    if not os.environ.get('HALO2_TEST_SCRATCH'):
        parser.error('Run through tools/run_test.py; capture storage requires its owned scratch')
    args.height = args.height or round(args.width*1378/2560)
    if not (320 <= args.width <= 7680 and 240 <= args.height <= 4320 and 0 <= args.pan_start < args.pan_end < 1000000 and
            1 <= args.look_warmup_ticks < 1000000 and
            2 <= args.settle_ticks <= 120 and 1 <= args.max_align_shift <= 8 and -1.5 <= args.pitch <= 1.5 and
            args.pan_end+args.settle_ticks+1<=1000000 and 0<=args.seed<=0xffffffff and
            all(math.isfinite(x) for x in (args.yaw,args.pitch,args.pan_angle,args.timeout_seconds,
                args.max_temporal_rms,args.max_ghost_rms,args.max_temporal_p95,args.max_ghost_p95,args.min_detail_retention)) and args.timeout_seconds>0 and
            0<=args.max_temporal_rms<=1 and 0<=args.max_ghost_rms<=1 and
            0<=args.max_temporal_p95<=1 and 0<=args.max_ghost_p95<=1 and 0<=args.min_detail_retention<=2):
        parser.error('Invalid dimensions, source tick interval, camera or finite timeout')
    if args.pan_start<args.look_warmup_ticks+2:
        parser.error('Pan start must follow the neutral look warmup by at least two source ticks')
    if any(scene=='pan-stop' for _,_,scene in cases) and (args.pan_end-args.pan_start<4 or args.pan_angle==0):
        parser.error('Pan-stop cases need a nonzero turn over at least four source ticks for an interior moving capture')
    selected_rois=args.roi or list(map(parse_roi,DEFAULT_ROIS))
    if any((edges[2]-edges[0])*args.width<2*args.max_align_shift+12 or
           (edges[3]-edges[1])*args.height<2*args.max_align_shift+12 for _,edges in selected_rois):
        parser.error('Each ROI must retain at least twelve pixels per axis after alignment margins')
    args.runtime=args.runtime.resolve();args.map=(args.map or MAP.with_name(args.map_name+'.map')).resolve();args.source_root=args.source_root.resolve()
    args.shader_dir=(args.shader_dir or args.runtime.parent/'shaders').resolve();args.preferences=args.preferences.resolve()
    report_path=args.report_output.resolve();report_path.parent.mkdir(parents=True,exist_ok=True)
    # Fail before any native work rather than overwriting another run's evidence.
    with report_path.open('x') as stream:stream.write('{}\n')
    if args.retain_captures:
        args.retain_captures=args.retain_captures.resolve();args.retain_captures.mkdir(parents=True,exist_ok=False)
    scratch=Path(tempfile.mkdtemp(prefix='path-tracing-visual-',dir=os.environ['HALO2_TEST_SCRATCH']))
    prefs=args.preferences.read_bytes() if args.preferences.exists() else b''
    runtime_hash,preference_hash=sha(args.runtime),sha(args.preferences)
    shader_hashes={name:sha(args.shader_dir/name) for name in SHADERS}
    report={'schema':1,'passed':False,'scope':'native deterministic world ROI noise/motion regression; no Xbox framebuffer parity claim',
            'runtime':str(args.runtime),'runtime_sha256':runtime_hash,'map':str(args.map),'map_sha256':sha(args.map),
            'shader_directory':str(args.shader_dir),'shader_sha256':shader_hashes,
            'source_root':str(args.source_root),'source_sha256':{name:sha(args.source_root/name) for name in SOURCES},
            'preferences_source_sha256':preference_hash,'helper_sha256':sha(__file__),
            'upscaler_runtime_sha256':{path.name:sha(path) for path in (args.runtime.parent/'upscalers/linux/x64').glob('*') if path.is_file()},
            'presentation_size':[args.width,args.height],'quality':args.quality,'samples_per_pixel':1,'max_bounces':1,
            'look_warmup_ticks':args.look_warmup_ticks,
            'thresholds':{'temporal_rms':args.max_temporal_rms,'ghost_rms':args.max_ghost_rms,
                          'temporal_p95':args.max_temporal_p95,'ghost_p95':args.max_ghost_p95,
                          'detail_retention':args.min_detail_retention},
            'rois':dict(selected_rois),'cases':[],'retained_large_artifacts':[]}
    rois=list(report['rois'].items())
    try:
        report['compiled_source_manifest']=compiled_source_evidence(args.runtime,report['source_sha256'])
        if not runtime_hash or not args.map.is_file() or any(value is None for value in shader_hashes.values()):
            raise RuntimeError('Native runtime, MCC map or declared compiled shader inputs are missing')
        original_appearance=None
        for index,(mode,method,scene) in enumerate(cases):
            entry={'case':f'{mode}:{method}:{scene}','runs':[],'passed':False};report['cases'].append(entry)
            motion_tick=args.pan_start+(args.pan_end-args.pan_start)//2
            motion_yaw=presented_pan_yaw(args,motion_tick)
            early_yaw=presented_pan_yaw(args,args.pan_end+1)
            settled_yaw=presented_pan_yaw(args,args.pan_end+args.settle_ticks)
            plans=(('still-a','still',args.pan_end+args.settle_ticks,None),
                   ('still-b','still',args.pan_end+args.settle_ticks+1,None)) if scene=='still' else (
                ('motion-reference','still',motion_tick,motion_yaw),('motion-pan','pan',motion_tick,None),
                ('early-reference','still',args.pan_end+1,early_yaw),('early-pan','pan',args.pan_end+1,None),
                ('settled-reference','still',args.pan_end+args.settle_ticks,settled_yaw),
                ('settled-pan','pan',args.pan_end+args.settle_ticks,None))
            images={}
            for role,sequence,tick,reference_yaw in plans:
                run=capture(args,scratch,prefs,mode,method,sequence,tick,f'c{index}-{role}',reference_yaw)
                pixels=run.pop('_pixels',None);entry['runs'].append(run)
                if pixels is not None:images[role]=pixels
                if run.get('retained_capture'):report['retained_large_artifacts'].append(run['retained_capture'])
                save_report(report_path,report)
            if not all(run['valid'] for run in entry['runs']):
                entry['error']='One or more native captures failed';save_report(report_path,report);continue
            pairs=(('stationary','still-a','still-b',args.max_temporal_rms,args.max_temporal_p95),) if scene=='still' else (
                ('moving-history','motion-reference','motion-pan',args.max_ghost_rms,args.max_ghost_p95),
                ('early-history','early-reference','early-pan',args.max_ghost_rms,args.max_ghost_p95),
                ('settled-history','settled-reference','settled-pan',args.max_ghost_rms,args.max_ghost_p95))
            entry['metrics']={};entry['passed']=True
            runs_by_role={role:run for (role,_,_,_),run in zip(plans,entry['runs'])}
            for label,reference,candidate,limit,p95_limit in pairs:
                if scene!='still' and any(runs_by_role[reference][field]!=runs_by_role[candidate][field]
                    for field in ('first_scripted_look','effective_input_tick','effective_presented_look')):
                    raise RuntimeError('Motion/reference pair used different initial or presented raw look samples')
                metrics=difference(images[reference],images[candidate],rois,args.max_align_shift);entry['metrics'][label]=metrics
                for values in metrics.values():
                    values['passed']=values['alignment_verified'] and values['aligned_rgb_rms']<=limit and \
                        values['p95_absolute_channel_delta']<=p95_limit and values['detail_retention']>=args.min_detail_retention
                    entry['passed'] &= values['passed']
            if scene=='still':
                entry['appearance']={name:texture(roi_pixels(images['still-a'],edges)) for name,edges in rois}
                if mode=='Original' and method=='Off':original_appearance=entry['appearance']
            save_report(report_path,report)
        if original_appearance:
            for entry in report['cases']:
                if not entry['case'].startswith('PathTracing:') or 'appearance' not in entry:continue
                entry['relative_to_original']={}
                for name,current in entry['appearance'].items():
                    baseline=original_appearance[name]
                    entry['relative_to_original'][name]={
                        'normalized_detail_ratio':current['normalized_detail']/max(baseline['normalized_detail'],1e-8),
                        'mean_luma_ratio':current['mean_luma']/max(baseline['mean_luma'],1e-8),
                        'dark_fraction_increase':current['dark_fraction_below_8']-baseline['dark_fraction_below_8'],
                        'mean_rgb_delta':[a-b for a,b in zip(current['mean_rgb'],baseline['mean_rgb'])],
                        'scope':'native mode appearance observations; different GI does not establish original aesthetic parity'}
        if sha(args.runtime)!=runtime_hash or {name:sha(args.shader_dir/name) for name in SHADERS}!=shader_hashes:
            raise RuntimeError('Runtime/shader inputs changed during the declared capture cohort')
        if {path.name:sha(path) for path in (args.runtime.parent/'upscalers/linux/x64').glob('*') if path.is_file()} != report['upscaler_runtime_sha256']:
            raise RuntimeError('Bundled native vendor runtime changed during the capture cohort')
        report['original_preferences_unchanged']=sha(args.preferences)==preference_hash
        if not report['original_preferences_unchanged']:raise RuntimeError('Original saved settings changed during validation')
        report['passed']=bool(report['cases']) and all(case['passed'] for case in report['cases'])
    except Exception as error:report['error']=str(error)
    finally:
        runs=[(case['case'],run) for case in report['cases'] for run in case['runs']]
        report['validation_diagnostics']=sum(run.get('validation_diagnostics',0) for _,run in runs)
        report['benign_unused_output_warnings']=[{'case':case,'label':run['label'],**warning}
            for case,run in runs for warning in run.get('benign_unused_output_warnings',[])]
        report['validation_errors']=sum(run.get('validation_errors',0) for _,run in runs)
        report['retained_large_bytes']=sum(item['bytes'] for item in report['retained_large_artifacts'])
        report['raw_storage']='All non-retained inputs/logs/savegames/cache/PPMs are inside the enclosing HALO2_TEST_SCRATCH owner'
        save_report(report_path,report)
    print(json.dumps({'passed':report['passed'],'cases':[(case['case'],case['passed']) for case in report['cases']],
                      'report':str(report_path),'error':report.get('error')}),flush=True)
    return 0 if report['passed'] else 1


if __name__=='__main__':raise SystemExit(main())
