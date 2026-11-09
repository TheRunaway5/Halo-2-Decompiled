#!/usr/bin/env python3
"""Compare native/original renderer paths at an actual rocket-lit PRT frame.

Run through run_test.py. Raw captures, command histories and graphics caches
belong to HALO2_TEST_SCRATCH. The capture executable uses real firing inputs;
it requests its frame only after the selected PRT object receives packets from
an occupied source rocket-light record with spatial radius >= 3.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import time

from perf_4k60 import parity
from qualify_native_tu15_large_sphere_renderer import runtime_evidence, sha
from validate_weapon_runtime_tu15 import private_display

ROOT = Path(__file__).resolve().parents[1]


def capture(args, role, scratch):
    work = scratch / role
    work.mkdir()
    output, log, bank = work / 'capture', work / 'runtime.log', work / 'commands.banks'
    reference = role.startswith('reference')
    environment = {k: v for k, v in os.environ.items()
                   if not k.startswith(('HALO2_REFERENCE_', 'HALO2_CAPTURE_')) and k != 'WAYLAND_DISPLAY'}
    environment.update(
        HALO2_CAPTURE_MODEL_ROCKET='1', SDL_VIDEODRIVER='x11', SDL_AUDIODRIVER='dummy',
        HALO2_CAPTURE_MODEL_LIMIT=str(args.max_tick),
        HALO2_REFERENCE_LIGHT_FRUSTUM_CULL='reference' if reference else '',
        HALO2_REFERENCE_LIGHT_COMMANDS='1' if reference else '',
        HALO2_REFERENCE_PART_PREPARE='1' if reference else '', HALO2_PART_PREPARE_ORACLE='',
        HALO2_NATIVE_PART_PREPARE_STATS='1',
        HALO2_NATIVE_LARGE_SPHERE_STATS='1', HALO2_NATIVE_LIGHT_COMMANDS_STATS='1',
        HALO2_NATIVE_CULL_WALK_STATS='1', HALO2_MAIN_CULL_ORACLE='1' if reference else '',
        HALO2_COMMAND_BANKS_PATH=str(bank), HALO2_MENU_FOG_SEED='1',
        HALO2_PIPELINE_CACHE_DIRECTORY=str(work / 'pipeline-cache'),
        MESA_SHADER_CACHE_DIR=str(work / 'mesa-cache'),
        __GL_SHADER_DISK_CACHE_PATH=str(work / 'nvidia-cache'),
        XDG_CACHE_HOME=str(work / 'cache'), XDG_DATA_HOME=str(work / 'data'),
        XDG_CONFIG_HOME=str(work / 'config'))
    layers = ROOT / '.tools/validation/usr/share/vulkan/explicit_layer.d'
    libraries = [ROOT / '.tools/validation/usr/lib', ROOT / '.tools/validation/usr/lib/x86_64-linux-gnu']
    if layers.is_dir():
        environment['VK_LAYER_PATH'] = str(layers)
    environment['LD_LIBRARY_PATH'] = ':'.join([*(str(p) for p in libraries if p.is_dir()), environment.get('LD_LIBRARY_PATH', '')])
    command = [str(args.runtime), str(args.maps / (args.map + '.map')), str(args.maps),
               str(args.xbe), str(args.width), str(args.height), '1', str(output), args.target]
    run = dict(role=role, valid=False, command=command)
    started = time.monotonic()
    with private_display(work) as display, log.open('w') as stream:
        environment['DISPLAY'] = display
        try:
            run['exit_code'] = subprocess.run(command, cwd=args.runtime.parent, env=environment,
                stdout=stream, stderr=subprocess.STDOUT, timeout=args.timeout).returncode
        except subprocess.TimeoutExpired:
            run.update(exit_code=124, error='Actual rocket capture timed out')
    run['seconds'] = round(time.monotonic() - started, 1)
    report = output / 'report.json'
    detail = json.loads(report.read_text()) if report.is_file() else {}
    # Draw lists are useful locally but only the bounded active-frame inputs
    # and coverage need to survive the scratch owner.
    run['capture'] = {k: v for k, v in detail.items() if k != 'draws'}
    run['draw_records'] = len(detail.get('draws', []))
    inputs = output / 'inputs.txt'
    run['input_script_sha256'] = sha(inputs) if inputs.is_file() else None
    sources = detail.get('model_light_sources', [])
    run['actual_lit_frame'] = bool(
        detail.get('rocket_workload') and detail.get('original_engine_rules')
        and detail.get('declared_position_input', {}).get('owner') == 0xb95d0
        and detail.get('firing_input_frames', 0) > 0
        and detail.get('actual_capture_tick', 0) == detail.get('model_light_tick', 0) > 20
        and detail.get('model_light_packets', 0) > 0 and sources
        and all(s.get('spatial_radius', 0) >= 3 and 'rocket' in s.get('light_tag', '')
                and s.get('model_packets', 0) > 0
                and any(v > 0 for v in s.get('diffuse_vertex_c37', [])[:3]) for s in sources))
    image = output / 'prop.ppm'
    if image.is_file() and image.stat().st_size <= args.width * args.height * 3 + 4096:
        payload = image.read_bytes()
        error = parity.validate_image(payload, (args.width, args.height))
        if error:
            run['error'] = error
        else:
            run.update(valid=run['exit_code'] == 0, _payload=payload,
                       sha256=hashlib.sha256(payload).hexdigest(), scratch_preview=str(image))
            rgb = payload.split(b'\n', 3)[-1]
            run['nonuniform_image'] = any(rgb[i:i+3] != rgb[:3] for i in range(3, len(rgb), 3))
    if not run['valid']:
        run['log_tail'] = parity.log_tail(log)
    evidence, data = runtime_evidence(log, bank)
    run.update(evidence)
    return run, data


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--runtime', type=Path, required=True)
    parser.add_argument('--maps', type=Path, required=True)
    parser.add_argument('--xbe', type=Path, required=True)
    parser.add_argument('--map', default='zanzibar')
    parser.add_argument('--target', required=True)
    parser.add_argument('--report', type=Path, required=True)
    parser.add_argument('--probe-only', action='store_true')
    parser.add_argument('--width', type=int, default=1920)
    parser.add_argument('--height', type=int, default=1080)
    parser.add_argument('--timeout', type=int, default=600)
    parser.add_argument('--max-tick', type=int, default=200)
    args = parser.parse_args()
    if not 40 <= args.max_tick <= 200:
        parser.error('--max-tick requires40..200')
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    for name in ('runtime', 'maps', 'xbe'):
        setattr(args, name, getattr(args, name).resolve(strict=True))
    pins = {str(p): sha(p) for p in (args.runtime, args.xbe, args.maps / (args.map + '.map'),
        Path(__file__), Path(parity.__file__), ROOT / 'tools/qualify_native_tu15_large_sphere_renderer.py')}
    entry = dict(case='actual-rocket-lit-prt', runs=[])
    banks = []
    roles = ('reference',) if args.probe_only else ('reference', 'reference-repeat', 'candidate', 'candidate-repeat')
    for role in roles:
        run, data = capture(args, role, scratch)
        entry['runs'].append(run)
        banks.append(data)
        print(role, 'valid', run['valid'], 'active frame', run['actual_lit_frame'],
              'mode0', run['receivers'], 'model entries', run['model_entries'], flush=True)
    runs = entry['runs']
    checks = dict(
        actual_light_visible=all(r['valid'] and r['actual_lit_frame'] and r.get('nonuniform_image') for r in runs),
        positive_mode0=all(r['receivers'] > 0 and r['entries'] > 0 and r['model_entries'] > 0 for r in runs),
        main_models_native=all(r['main_native_model_entries'] > 0 and r['main_residual_model_entries'] == 0 for r in runs),
        no_mismatches=all(r['mismatches'] == r['main_unit_mismatches'] == r['main_mask_mismatches'] == r['main_fp_mismatches'] == 0
                          and not r['diagnostic_errors'] and not r['vulkan_validation_errors'] for r in runs),
        path_selection=all((r['reference_calls'] == r['receivers'] > 0 and r['original_command_calls'] > 0 and r['native_command_calls'] == 0)
            if r['role'].startswith('reference') else
            (r['reference_calls'] == 0 and r['native_command_calls'] > 0 and r['original_command_calls'] == 0) for r in runs),
        all_commands_traced=all(r['every_light_command_traced'] for r in runs))
    checks['part_prepare_selection'] = all(r['part_residual'] == r['part_mismatches'] == 0 and
        (r['part_reference'] > 0 and r['part_native'] == 0 if r['role'].startswith('reference') else
         r['part_native'] > 0 and r['part_models'] > 0 and r['part_reference'] == 0) for r in runs)
    # Compare actual capture timing, source owner/pose and light identity as
    # well as the scripted firing bytes. Paths differ between owned roles.
    keys = ('actual_capture_tick', 'model_light_tick', 'firing_input_frames', 'source_inputs', 'model_light_sources', 'declared_position_input')
    checks['active_inputs_equal'] = all(
        r['input_script_sha256'] == runs[0]['input_script_sha256']
        and all(r['capture'].get(k) == runs[0]['capture'].get(k) for k in keys) for r in runs)
    checks['full_command_banks_and_fp_equal'] = bool(banks[0]) and all(data == banks[0] for data in banks)
    if args.probe_only:
        for run in runs:
            run.pop('_payload', None)
        passed = False
    else:
        parity.compare_runs(entry)
        passed = entry['passed'] and all(checks.values())
    unchanged = all(sha(p) == digest for p, digest in pins.items())
    report = dict(passed=passed and unchanged, validation_not_run=args.probe_only,
        probe_completed=all(checks.values()) if args.probe_only else None, checks=checks,
        case=entry, inputs_unchanged=unchanged, source_runtime_map_sha256=pins,
        retained_large_artifacts=[], whole_renderer_xbox_visual_parity_verified=False,
        scope='Actual firing and source-light model packets; same-engine native/original cull and command-frame comparison. '
              'The capture entry point differs from the production launcher. No physical Xbox framebuffer comparison.')
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2) + '\n')
    return int(not (unchanged and all(checks.values()) if args.probe_only else report['passed']))


if __name__ == '__main__':
    raise SystemExit(main())
