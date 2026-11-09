#!/usr/bin/env python3
"""Compare native 44C80 with its original on a live campaign World."""
import argparse
import json
import os
from pathlib import Path
import re
import subprocess

from physics_test_cache import runtime_cache, runtime_map
from qualify_native_tu15_large_sphere_renderer import sha
from validate_weapon_runtime_tu15 import private_display

ROOT = Path(__file__).resolve().parents[1]
COUNTERS = ('native', 'reference', 'residual', 'parts', 'part_native', 'part_residual',
            'emits', 'emit_native', 'emit_residual', 'bank_appends', 'comparisons', 'mismatches')


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--runtime', type=Path, required=True)
    p.add_argument('--mcc-root', type=Path, required=True)
    p.add_argument('--xbe', type=Path, required=True)
    p.add_argument('--report', type=Path, required=True)
    p.add_argument('--frames', type=int, default=1600)
    p.add_argument('--timeout', type=int, default=600)
    p.add_argument('--require-section-queue', action='store_true')
    p.add_argument('--require-command-selection', action='store_true')
    p.add_argument('--require-material-selection', action='store_true')
    a = p.parse_args()
    if not (1 <= a.frames <= 5000 and 1 <= a.timeout <= 1800):
        p.error('The campaign proof requires finite frame and timeout bounds')
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    maps, receipt = runtime_cache(a.mcc_root)
    intro = runtime_map(maps, maps/'00a_introduction.map')
    runtime, xbe = a.runtime.resolve(strict=True), a.xbe.resolve(strict=True)
    pins = {str(path): sha(path) for path in (runtime, xbe, intro, Path(__file__),
                                             maps/'shared.map', maps/'single_player_shared.map')}
    command = [str(runtime), '--campaign', '--map', str(intro), '--shared-maps', str(maps),
               '--gameplay-xbe', str(xbe), '--graphics-mode', 'Original', '--frames', str(a.frames),
               '--viewport', '960', '540', '--render-size', '960', '540', '--hidden', '--mute']
    log = scratch/'runtime.log'
    trace = scratch/'hologram.jsonl'
    environment = {key: value for key, value in os.environ.items()
                   if not key.startswith('HALO2_') and key != 'WAYLAND_DISPLAY'}
    environment.update(HALO2_TEST_SCRATCH=str(scratch), SDL_VIDEODRIVER='x11', SDL_AUDIODRIVER='dummy',
                       HALO2_TRANSPARENT_COLLECTION_ORACLE='1', HALO2_NATIVE_TRANSPARENT_COLLECTION_STATS='1',
                       HALO2_CAMPAIGN_HOLOGRAM_TRACE=str(trace), HALO2_NATIVE_CULL_WALK_STATS='1',
                       HALO2_PIPELINE_CACHE_DIRECTORY=str(scratch/'pipelines'),
                       MESA_SHADER_CACHE_DIR=str(scratch/'mesa'),
                       __GL_SHADER_DISK_CACHE_PATH=str(scratch/'nvidia'))
    if a.require_section_queue:
        environment.update(HALO2_SECTION_QUEUE_ORACLE='1', HALO2_NATIVE_SECTION_QUEUE_STATS='1')
    if a.require_command_selection or a.require_material_selection:
        environment.update(HALO2_COMMAND_SELECTION_ORACLE='1', HALO2_NATIVE_COMMAND_SELECTION_STATS='1')
    if a.require_material_selection:
        environment['HALO2_NATIVE_MATERIAL_SELECTION_STATS']='1'
    for key in ('XDG_CONFIG_HOME', 'XDG_DATA_HOME', 'XDG_CACHE_HOME'):
        environment[key] = str(scratch/key.lower())
    with private_display(scratch) as display, log.open('w') as stream:
        environment['DISPLAY'] = display
        try:
            exit_code = subprocess.run(command, cwd=runtime.parent, env=environment, stdout=stream,
                                       stderr=subprocess.STDOUT, timeout=a.timeout).returncode
        except subprocess.TimeoutExpired:
            exit_code = 124
    counters = dict.fromkeys(COUNTERS, 0)
    sections = dict.fromkeys(('native', 'reference', 'residual', 'appends', 'comparisons', 'mismatches'), 0)
    selection = sections.copy()
    materials = dict.fromkeys(('native', 'reference', 'residual', 'accepted', 'class_rejected', 'post_rejected'), 0)
    groups = submissions = 0
    first_group_frame = None
    if trace.is_file():
        with trace.open() as stream:
            for line in stream:
                row = json.loads(line)
                if row.get('event') == 'hologram_expected':
                    groups += 1
                    if first_group_frame is None:
                        first_group_frame = row.get('frame')
                elif row.get('event') == 'hologram_submission':
                    submissions += 1
    errors = []
    with log.open(errors='replace') as stream:
        for line in stream:
            if line.startswith('Collect44C80 stats:'):
                values = {key: int(value) for key, value in re.findall(r'(\w+)=(\d+)', line)}
                for key in COUNTERS:
                    counters[key] += values.get(key, 0)
            elif line.startswith('Queue42750 stats:'):
                values = {key: int(value) for key, value in re.findall(r'(\w+)=(\d+)', line)}
                for key in sections:
                    sections[key] += values.get(key, 0)
            elif line.startswith('Select3BAA0 stats:'):
                values = {key: int(value) for key, value in re.findall(r'(\w+)=(\d+)', line)}
                for key in selection:
                    selection[key] += values.get(key, 0)
            elif line.startswith('Select4CBE0 stats:'):
                values = {key: int(value) for key, value in re.findall(r'(\w+)=(\d+)', line)}
                for key in materials:
                    materials[key] += values.get(key, 0)
            if len(errors) < 24 and (line.startswith('error:') or 'Script fault:' in line):
                errors.append(line.strip()[:1600])
    positive = (counters['native'] > 0 and counters['comparisons'] == counters['native'] == counters['reference']
                and counters['part_native'] > 0 and counters['emit_native'] > 0 and counters['bank_appends'] > 0
                and not any(counters[key] for key in ('mismatches', 'residual', 'part_residual', 'emit_residual')))
    unchanged = all(sha(path) == digest for path, digest in pins.items())
    section_positive = (sections['native'] > 0 and sections['comparisons'] == sections['reference'] > 0
                        and sections['appends'] > 0 and not sections['mismatches'] and not sections['residual'])
    selection_positive = (selection['native'] == selection['comparisons'] > 0 and selection['appends'] > 0
                          and not selection['mismatches'] and not selection['residual'])
    material_positive = selection_positive and materials['native'] > 0 and materials['accepted'] > 0
    result = dict(passed=positive and (not a.require_section_queue or section_positive)
                         and (not a.require_command_selection or selection_positive)
                         and (not a.require_material_selection or material_positive)
                         and exit_code == 0 and not errors and unchanged,
                  same_world_oracle_positive=positive, exit_code=exit_code, counters=counters,
                  section_queue_required=a.require_section_queue, section_queue_oracle_positive=section_positive,
                  section_queue_counters=sections,
                  command_selection_required=a.require_command_selection, command_selection_oracle_positive=selection_positive,
                  command_selection_counters=selection,
                  material_selection_required=a.require_material_selection, material_selection_oracle_positive=material_positive,
                  material_selection_counters=materials,
                  hologram_group_frames=groups, hologram_submissions=submissions, first_group_frame=first_group_frame,
                  errors=errors, command=command, mcc_inputs=receipt, source_runtime_map_sha256=pins,
                  inputs_unchanged=unchanged, retained_large_artifacts=[],
                  scope='Same-World CPU parity for native campaign group collection, ordered source banks and FP. The run uses the natural opening path.')
    a.report.parent.mkdir(parents=True, exist_ok=True)
    a.report.write_text(json.dumps(result, indent=2)+'\n')
    print(json.dumps(dict(passed=result['passed'], exit_code=exit_code, counters=counters, sections=sections,
                          selection=selection, materials=materials, errors=errors)), flush=True)
    return int(not result['passed'])


if __name__ == '__main__':
    raise SystemExit(main())
