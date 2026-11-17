#!/usr/bin/env python3
"""Exercise the selected campaign runtime at the retained council-camera frame.

Run through tools/run_test.py. Captures and trace banks stay in its scratch;
only commands, hashes, compact frame metrics and bounded logs are retained.
This is a local visual regression, not a whole-game Xbox parity claim.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import time

import numpy as np
from PIL import Image, ImageDraw

from gameplay_runtime_selection import SELECTION, selected_gameplay_runtime


def digest(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def admitted_capture_runtime(root, runtime_override=None):
    """Use the launcher's complete package admission for selected or prepared runtimes."""
    root = root.resolve(strict=True)
    selection_name = SELECTION
    requested = None
    if runtime_override is not None:
        requested = runtime_override if runtime_override.is_absolute() else root / runtime_override
        try:
            selection_name = (requested.parent / 'selection.json').relative_to(root).as_posix()
        except ValueError as error:
            raise ValueError('Candidate runtime must belong directly to this project') from error
    record = selected_gameplay_runtime(root, selection_name=selection_name)
    if record is None:
        raise ValueError('Capture runtime has no qualified selection')
    runtime = root / record['runtime']
    if requested is not None and requested != runtime:
        raise ValueError('Candidate selection names a different runtime')
    return root / selection_name, record


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--report-output', required=True, type=Path)
    parser.add_argument('--hold-for-inspection', type=int, default=0)
    parser.add_argument('--runtime', type=Path,
                        help='Prepared candidate executable with an authenticated adjacent selection.json; defaults to the selected launcher route')
    args = parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):
        parser.error('Use tools/run_test.py')
    if not 0 <= args.hold_for_inspection <= 600:
        parser.error('Inspection hold must be 0..600 seconds')
    root = Path(__file__).resolve().parents[1]
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    try:
        selection_path, selected = admitted_capture_runtime(root, args.runtime)
    except (OSError, ValueError, KeyError, TypeError, RuntimeError) as error:
        parser.error(str(error))
    selection_pin = digest(selection_path)
    launcher_selection_path = root / SELECTION
    launcher_selection_pin = digest(launcher_selection_path)
    runtime = root / selected['runtime']
    assert digest(runtime) == selected['runtime_sha256'], 'Selected runtime hash changed'
    maps = root / selected['cache'] / 'halo2/h2_maps_win64_dx11'
    route = ([str(runtime), '--campaign', '--map', str(maps / '00a_introduction.map'),
              '--shared-maps', str(maps)] if args.runtime else [str(root / 'Halo2-pStencil-Campaign')])
    report = dict(runtime=str(runtime), runtime_sha256=selected['runtime_sha256'],
                  route='prepared-candidate' if args.runtime else 'launcher-selected',
                  selection_path=str(selection_path), launcher_selection_sha256=launcher_selection_pin,
                  selection_sha256=selection_pin, scratch=str(scratch), cases=[],
                  scope='Natural MCC introduction, neutral input, declared .05s interval, Original mode; local visual controls only')
    images = {}
    output = args.report_output.resolve()
    output.parent.mkdir(parents=True, exist_ok=True)
    for name, width, height, old_lighting in (
            ('fixed-480', 640, 480, False),
            ('world-lighting-control', 640, 480, True),
            ('fixed-4k', 3840, 2160, False)):
        owned = scratch / name
        owned.mkdir()
        capture, trace = owned / 'frame.ppm', owned / 'trace.jsonl'
        env = os.environ.copy()
        env.pop('WAYLAND_DISPLAY', None)
        env.pop('HALO2_REFERENCE_CAMPAIGN_WORLD_MODEL_LIGHTING', None)
        env.update(SDL_VIDEODRIVER='x11', SDL_AUDIODRIVER='dummy',
                   XDG_DATA_HOME=str(owned / 'data'), XDG_CONFIG_HOME=str(owned / 'config'),
                   XDG_CACHE_HOME=str(owned / 'cache'), HALO2_PIPELINE_CACHE_DIRECTORY=str(owned / 'vk'),
                   MESA_SHADER_CACHE_DIR=str(owned / 'mesa'),
                   __GL_SHADER_DISK_CACHE_PATH=str(owned / 'nvidia'),
                   HALO2_REFERENCE_CAMPAIGN_CAPTURE_FRAME_LIMIT='4000',
                   VK_LAYER_PATH=str(root / '.tools/validation/usr/share/vulkan/explicit_layer.d'),
                   LD_LIBRARY_PATH=str(root / '.tools/validation/usr/lib'), VK_LAYER_VALIDATE_SYNC='1')
        if old_lighting:
            env['HALO2_REFERENCE_CAMPAIGN_WORLD_MODEL_LIGHTING'] = '1'
        command = ['xvfb-run', '-a', '-s', f'-screen 0 {width}x{height}x24',
                   *route, '--graphics-mode', 'Original',
                   '--viewport', str(width), str(height), '--frames', '827', '--mute',
                   '--screenshot', str(capture), '--campaign-reference-capture',
                   '.05', '0', '305419896', str(trace), '--validation']
        start = time.monotonic()
        with (owned / 'run.log').open('w') as log:
            result = subprocess.run(command, cwd=root, env=env, stdout=log,
                                    stderr=subprocess.STDOUT, timeout=900)
        log = (owned / 'run.log').read_text(errors='replace')
        row = dict(name=name, command=command, exit_code=result.returncode,
                   seconds=time.monotonic()-start, old_world_lighting_control=old_lighting,
                   dimensions=[width, height], map_sha256=digest(maps / '00a_introduction.map'),
                   checks=dict(exit_ok=result.returncode == 0,
                               frames_complete='Rendered 827 frames' in log,
                               council_tick=bool(re.search(r'^tick=1240\b', log, re.MULTILINE)),
                               no_script_faults=not bool(re.search(r'script_faults=[1-9]', log)),
                               no_validation_errors='Vulkan validation:' not in log and 'VUID-' not in log))
        (output.parent / f'{output.stem}-{name}.log').write_text(log[-131072:])
        if capture.exists():
            image = Image.open(capture).convert('RGB')
            row['capture_sha256'] = digest(capture)
            row['checks']['dimensions_match'] = image.size == (width, height)
            row['checks']['nonempty_scene'] = bool(np.asarray(image).std() > 5)
            images[name] = image
            image.thumbnail((960, 540))
            image.save(owned / 'preview.png')
        else:
            row['checks']['capture_exists'] = False
        row['passed'] = all(row['checks'].values())
        report['cases'].append(row)
        output.write_text(json.dumps(report, indent=2) + '\n')
        print(json.dumps(dict(case=name, passed=row['passed'], scratch=str(owned))), flush=True)
    fixed = np.asarray(images['fixed-480'])
    control = np.asarray(images['world-lighting-control'])
    def stats(pixels, box):
        x0, y0, x1, y1 = box
        region = pixels[y0:y1, x0:x1]
        return dict(maximum_channel=int(region.max()), mean_rgb=region.mean(axis=(0, 1)).tolist(),
                    pixels_over_128=int((region.max(axis=2) > 128).sum()))
    bright = [475, 98, 512, 139]
    anchor = [311, 248, 338, 267]
    report['councillor'] = dict(rectangle=bright, fixed=stats(fixed, bright),
                                 control=stats(control, bright))
    report['anchor'] = dict(rectangle=anchor, fixed=stats(fixed, anchor))
    # Retained failure is a near-white prop; genuine correction must retain
    # its visible surface and change this actual rendered positive control.
    report['visual_checks'] = dict(
        control_reproduces_glare=report['councillor']['control']['pixels_over_128'] > 20,
        fixed_removes_glare=report['councillor']['fixed']['pixels_over_128'] == 0,
        prop_still_visible=report['councillor']['fixed']['maximum_channel'] > 50,
        anchor_speck_absent=report['anchor']['fixed']['pixels_over_128'] == 0,
        selection_unchanged=digest(selection_path) == selection_pin,
        launcher_selection_unchanged=digest(launcher_selection_path) == launcher_selection_pin,
        runtime_unchanged=digest(runtime) == selected['runtime_sha256'])
    try:
        final_selection_path, final_selected = admitted_capture_runtime(root, args.runtime)
        report['visual_checks']['qualified_package_unchanged'] = final_selection_path == selection_path and final_selected == selected
    except (OSError, ValueError, KeyError, TypeError, RuntimeError) as error:
        report['visual_checks']['qualified_package_unchanged'] = False
        report['final_admission_error'] = str(error)
    sheet = Image.new('RGB', (1280, 480))
    sheet.paste(images['world-lighting-control'], (0, 0))
    sheet.paste(images['fixed-480'], (640, 0))
    draw = ImageDraw.Draw(sheet)
    draw.text((8, 8), 'World-lighting control', fill='yellow')
    draw.text((648, 8), 'Prepared candidate: fixed' if args.runtime else 'Selected runtime: fixed', fill='yellow')
    sheet.save(scratch / 'council-comparison.png')
    report['passed'] = all(row['passed'] for row in report['cases']) and all(report['visual_checks'].values())
    output.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(dict(passed=report['passed'], inspection=str(scratch / 'council-comparison.png'))), flush=True)
    deadline = time.monotonic() + args.hold_for_inspection
    while time.monotonic() < deadline and not (scratch / 'inspection-complete').exists():
        time.sleep(.2)
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
