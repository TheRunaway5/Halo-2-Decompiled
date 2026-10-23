#!/usr/bin/env python3
"""Run the combat diagnostic on an owned display; keep bounded text evidence.

Usage through run_test.py: EXE MODE MCC_MAP REPORT [WEAPON PRESSES].
Mode zoom-visual repeats a scoped frame and isolates only the arms draw.
Exit 1 means visible scoped arm pixels were reproduced, not fixed.
The executable is built in the source snapshot recorded by the audit session.
"""
import json
import hashlib
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys

from validate_weapon_runtime_tu15 import private_display

TIMEOUT = 360

def graphics_cache(scratch):
    path = Path(os.environ.get('HALO2_COMBAT_AUDIT_CACHE', str(scratch / 'cache'))).resolve()
    owner = Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    owners = [owner] + [p for p in owner.parents if p.name.startswith('halo2-test-')]
    if not any(path.is_relative_to(p) for p in owners):
        raise RuntimeError('Graphics cache must remain in an enclosing owned test directory')
    return str(path)

def main():
    if not os.environ.get('HALO2_TEST_SCRATCH'):
        raise RuntimeError('Use tools/run_test.py')
    executable, mode, map_, report, *arguments = sys.argv[1:]
    report = Path(report).resolve()
    report.parent.mkdir(parents=True, exist_ok=True)
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']) / ('combat-' + report.stem)
    scratch.mkdir(exist_ok=True)
    output = scratch / 'runtime.log'
    if mode in ('zoom-visual', 'zoom-full-visual', 'vehicle-visual', 'fire-visual'):
        return visual(executable, map_, report, arguments, scratch, mode)
    with private_display(scratch) as display, output.open('w') as stream:
        environment = dict(os.environ, DISPLAY=display, SDL_VIDEODRIVER='x11',
                           SDL_AUDIODRIVER='dummy', XDG_CACHE_HOME=graphics_cache(scratch))
        if mode == 'vehicle':
            case = arguments[0]
            environment.update(HALO2_TEST_VEHICLE_WEAPON_HUD_CASE=case,
                               HALO2_VEHICLE_HUD_GPU_NAME=case.split('-')[0],
                               HALO2_TEST_GHOST_PROJECTILE_RENDER='1',
                               HALO2_TEST_VEHICLE_WEAPON_HUD_REPORT=str(report))
            command = [executable, map_, str(Path(map_).parent), 'native-mcc',
                       str(scratch / 'firing.ppm')]
        else:
            command = [executable, mode, map_, str(report), *arguments]
        try:
            code = subprocess.run(command,
                                  env=environment, stdout=stream, stderr=subprocess.STDOUT,
                                  timeout=TIMEOUT, check=False).returncode
        except subprocess.TimeoutExpired:
            code = 124
    keep_log(output, report.with_suffix('.log'))
    print(json.dumps({'mode': mode, 'arguments': arguments, 'exit_code': code,
                      'report': str(report), 'report_produced': report.is_file()}))
    return code


def keep_log(source, target):
    with source.open('rb') as stream:
        if source.stat().st_size <= 65536:
            data = stream.read()
        else:
            data = stream.read(16384) + b'\n[Middle of diagnostic log omitted]\n'
            stream.seek(-49152, 2);data += stream.read()
    target.write_bytes(data)


def visual(executable, map_, report, arguments, scratch, mode):
    from PIL import Image, ImageChops
    runs = []
    images = []
    vehicle = mode == 'vehicle-visual'
    firing = mode == 'fire-visual'
    glow = vehicle or firing
    full = mode == 'zoom-full-visual'
    control = 'without-glow' if glow else 'without-foreground' if full else 'without-arms'
    for label in ('baseline', 'repeat', control):
        capture = scratch / (label + '.ppm')
        detail = report.with_name(report.stem + '-' + label + '.json')
        log = scratch / (label + '.log')
        with private_display(scratch) as display, log.open('w') as stream:
            environment = dict(os.environ, DISPLAY=display, SDL_VIDEODRIVER='x11',
                               SDL_AUDIODRIVER='dummy', XDG_CACHE_HOME=graphics_cache(scratch),
                               HALO2_COMBAT_AUDIT='1', HALO2_COMBAT_AUDIT_CAPTURE=str(capture))
            environment.pop('HALO2_COMBAT_AUDIT_HIDE_ARMS', None)
            environment.pop('HALO2_COMBAT_AUDIT_HIDE_GLOW', None)
            environment.pop('HALO2_COMBAT_AUDIT_HIDE_FOREGROUND', None)
            if label == control:
                environment['HALO2_COMBAT_AUDIT_HIDE_GLOW' if glow else 'HALO2_COMBAT_AUDIT_HIDE_FOREGROUND' if full else 'HALO2_COMBAT_AUDIT_HIDE_ARMS'] = '1'
            if vehicle:
                environment.update(HALO2_TEST_VEHICLE_WEAPON_HUD_CASE=arguments[0],
                                   HALO2_VEHICLE_HUD_GPU_NAME=arguments[0],
                                   HALO2_TEST_GHOST_PROJECTILE_RENDER='1',
                                   HALO2_TEST_VEHICLE_WEAPON_HUD_REPORT=str(detail),
                                   HALO2_COMBAT_AUDIT_PIXEL_CAPTURE='1')
                command = [executable, map_, str(Path(map_).parent), 'native-mcc', str(capture)]
            else:
                command = [executable, 'fire' if firing else 'zoom', map_, str(detail), *arguments]
            try:
                code = subprocess.run(command,
                                      env=environment, stdout=stream, stderr=subprocess.STDOUT,
                                      timeout=TIMEOUT, check=False).returncode
            except subprocess.TimeoutExpired:
                code = 124
        keep_log(log, detail.with_suffix('.log'))
        if code or not capture.is_file():
            report.write_text(json.dumps({'audit_completed': False, 'failed_case': label,
                                          'exit_code': code, 'process_timeout_seconds': TIMEOUT,
                                          'reason': 'Runtime timed out before a capture' if code == 124 else 'Runtime did not produce a capture'}, indent=2) + '\n')
            return code or 2
        data = json.loads(detail.read_text())
        expected_selector = None if glow else int(arguments[1]) - 1
        if not glow and (data['final_zoom'] != expected_selector or data['views'][-1]['zoom'] != expected_selector):
            report.write_text(json.dumps({'audit_completed': False, 'failed_case': label,
                                          'reason': 'Actual source/render zoom did not reach the requested selector',
                                          'detail': str(detail)}, indent=2) + '\n')
            return 2
        with Image.open(capture) as image:
            full_size = image.size
            pixels = image.convert('RGB')
        row = {'case': label, 'report': str(detail), 'exit_code': code}
        if not glow:
            row.update(actual_source_zoom=data['final_zoom'],
                       actual_source_camera_fov=data['views'][-1]['source_horizontal_fov'],
                       actual_render_projection_degrees=data['views'][-1]['render_projection_degrees'])
        else:
            row['projectile_render'] = data['cases'][0]['projectile_render'] if vehicle else {
                k:data[k] for k in ('live_projectiles','attached_projectiles','raw_source_attached_projectiles',
                    'attached_model_projectiles','glow_projectiles','glow_draws','glow_vertices')}
            rectangles = re.findall(r'\[COMBAT-AUDIT\] view=0 .* viewport=([0-9.,-]+)', log.read_text())
            if not rectangles:
                raise RuntimeError('Vehicle pixel audit has no actual pilot viewport observation')
            left, top, width, height = map(float, rectangles[-1].split(','))
            bounds = tuple(round(x) for x in (left, top, left + width, top + height))
            if not (0 <= bounds[0] < bounds[2] <= full_size[0] and 0 <= bounds[1] < bounds[3] <= full_size[1]):
                raise RuntimeError('Actual pilot viewport lies outside the capture')
            row['pilot_viewport'] = bounds
            if runs and row['pilot_viewport'] != runs[0]['pilot_viewport']:
                raise RuntimeError('Pilot viewport changed between pixel controls')
            pixels = pixels.crop(bounds)
        images.append(pixels)
        runs.append(row)
    baseline, repeat, without_arms = images
    repeat_pixels = sum(a != b for a, b in zip(baseline.getdata(), repeat.getdata()))
    arm_pixels = sum(a != b for a, b in zip(baseline.getdata(), without_arms.getdata()))
    diff = ImageChops.difference(baseline, without_arms)
    baseline.save(scratch / 'baseline.png')
    without_arms.save(scratch / (control + '.png'))
    diff.save(scratch / ('glow-difference.png' if glow else 'foreground-difference.png' if full else 'arms-difference.png'))
    scoped = not glow and int(arguments[1]) > 0
    subject = 'glow' if glow else 'foreground' if full else 'arms'
    result = {'audit_completed': True, 'scope': 'Actual SDL/Vulkan frames; only ' + subject + ' submission is suppressed in the diagnostic control',
              'subject': arguments[0], 'runs': runs,
              'repeat_changed_pixels': repeat_pixels, subject + '_changed_pixels': arm_pixels,
              subject + '_difference_bounds': diff.getbbox(), 'image_size': baseline.size,
              'symptom_reproduced': scoped and repeat_pixels == 0 and arm_pixels > 0,
              'original_Xbox_pixels_compared': False, 'capture_scratch': str(scratch)}
    if glow:
        result['pixel_scope'] = 'Actual view 0 pilot viewport only'
        result['full_image_size'] = full_size
        result['actual_projectile_glow_visible'] = repeat_pixels == 0 and arm_pixels > 0
        result['missing_glow_verdict'] = 'Not established by a single final frame' if not arm_pixels else 'Not reproduced in this final frame'
        if firing:
            actual=runs[0]['projectile_render']
            attached=set(actual['raw_source_attached_projectiles'])
            result['actual_attached_models_presented']=bool(attached) and attached<=set(actual['attached_projectiles']) and attached<=set(actual['attached_model_projectiles'])
            result['actual_attached_glows_presented']=bool(attached) and attached<=set(actual['glow_projectiles'])
    else:
        result['presses'] = int(arguments[1])
    reference = os.environ.get('HALO2_COMBAT_AUDIT_REFERENCE_DIR')
    if reference and report.stem in ('combat-audit-sniper-1', 'combat-audit-vehicle-pixels-banshee',
            'repair-scope-normal-full', 'repair-scope-projection-full',
            'repair-banshee-grounded', 'repair-banshee-airborne', 'repair-sniper-fixed', 'repair-needler-fixed') and repeat_pixels == 0:
        destination = Path(reference);destination.mkdir(parents=True, exist_ok=True)
        names = ('baseline', control, 'glow-difference' if glow else 'foreground-difference' if full else 'arms-difference')
        sources = [scratch / (name + '.png') for name in names]
        if all(p.stat().st_size <= 1024*1024 for p in sources) and sum(p.stat().st_size for p in sources) <= 2*1024*1024:
            result['retained_reference_images'] = []
            for source in sources:
                target = destination / (report.stem + '-' + source.name)
                shutil.copyfile(source, target)
                result['retained_reference_images'].append({'path': str(target), 'bytes': target.stat().st_size,
                    'sha256': hashlib.sha256(target.read_bytes()).hexdigest()})
            result['reference_reason'] = 'Small actual frame controls required to inspect the reported regression; raw frames remain disposable'
    report.write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(result))
    if firing and (not result['actual_attached_models_presented'] or
                   not result['actual_attached_glows_presented'] or not arm_pixels):
        return 2
    return 2 if repeat_pixels else int(result['symptom_reproduced'])


if __name__ == '__main__':
    raise SystemExit(main())
