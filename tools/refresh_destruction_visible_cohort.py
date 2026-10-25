#!/usr/bin/env python3
"""Refresh only the declared SDL observer fixture in its existing owned build.

Use the active tools/run_test.py finite session. Preserve the authenticated
frozen engine; compile and validate with the official CMake worker. Pixels
still require inspection after the ordinary input run.
"""
import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys

from refresh_destruction_current_cohort import sha

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cohort', required=True, type=Path)
    parser.add_argument('--previous-refresh', required=True, type=Path)
    parser.add_argument('--production-refresh', required=True, type=Path)
    parser.add_argument('--patch-file', type=Path,
                        help='Authenticated exact fixture substitutions and new fixture headers')
    parser.add_argument('--output', required=True, type=Path)
    parser.add_argument('--jobs', type=int, default=8)
    args = parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):
        parser.error('Use the still-owned finite test session')
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    source = scratch/'source'
    cohort = json.loads(args.cohort.read_text());previous = json.loads(args.previous_refresh.read_text())
    production = json.loads(args.production_refresh.read_text())
    if any(Path(row['scratch']).resolve()!=scratch for row in (cohort,previous,production)):
        raise ValueError('Visible fixture refresh has a different source/build owner')
    if previous['prior_cohort']!=str(args.cohort.resolve()):
        raise ValueError('Previous fixture refresh belongs to another source cohort')
    args.output = args.output.resolve();args.output.mkdir(parents=True, exist_ok=False)
    receipt = args.output/'admission.json'
    report = dict(passed=False,scratch=str(scratch),prior_cohort=str(args.cohort.resolve()),
                  previous_refresh=str(args.previous_refresh.resolve()),
                  inherited_production_refresh=str(args.production_refresh.resolve()),
                  patches=dict(production['patches']),retained_large_artifacts=[])
    def save():
        receipt.write_text(json.dumps(report,indent=2)+'\n')
    manifest = json.loads((args.cohort.parent/'source/source-manifest.json').read_text())
    expected = dict(manifest['source_files_sha256']);expected.update(cohort['additional_frozen_inputs'])
    expected.update({name:change['after'] for name,change in production['patches'].items()})
    headless = 'tests/destruction_runtime_tu15_tests.cpp'
    expected[headless] = previous['fixture_refresh']['after']
    expected['CMakeLists.txt'] = previous['registered_cmake_sha256']
    if previous.get('production_refresh')!=str(args.production_refresh.resolve()):
        expected[headless] = production['patches'].get(headless,{}).get('after',expected[headless])
        expected['CMakeLists.txt'] = production['cmake_sha256']
    if cohort.get('private_build_partition'):
        partition = cohort['private_build_partition'];expected[partition['path']] = partition['after']
    report['frozen_source_drift_before_refresh'] = [name for name,digest in expected.items() if sha(source/name)!=digest]
    save()
    if report['frozen_source_drift_before_refresh']:
        raise ValueError('Frozen source changed before the declared SDL fixture patch')
    if headless in report['patches']:
        report['patches'][headless] = dict(report['patches'][headless],after=sha(source/headless))
    relative = 'tests/physics_multiplayer_acceptance.cpp';path = source/relative
    before = sha(path);text = path.read_text()
    replacements = [
        ('            {"yaw",input.yaw},{"pitch",input.pitch}};\n        // Render/input samples',
         '            {"yaw",input.yaw},{"pitch",input.pitch},\n            {"source_yaw",watcher.yaw},{"source_pitch",watcher.pitch}};\n        // Render/input samples'),
        ('          observer->axis(SDL_CONTROLLER_AXIS_RIGHTX,-std::remainder(yaw-input.yaw,6.283185307f)*8.f);\n          observer->axis(SDL_CONTROLLER_AXIS_RIGHTY,-(pitch-input.pitch)*8.f);',
         '''          // As for the actor's SDL mouse, close this ordinary gamepad loop
          // against the processed source view. Raw PC sample angles can
          // retain a different baseline after original birth/vehicle turns.
          const auto observed_yaw=vehicle_blast_test?watcher.yaw:input.yaw;
          const auto observed_pitch=vehicle_blast_test?watcher.pitch:input.pitch;
          observer->axis(SDL_CONTROLLER_AXIS_RIGHTX,-std::remainder(yaw-observed_yaw,6.283185307f)*8.f);
          observer->axis(SDL_CONTROLLER_AXIS_RIGHTY,-(pitch-observed_pitch)*8.f);'''),
        ('        report["vehicle_original_debris_visible_parts_by_view"]=visible_parts;',
         '''        report["vehicle_original_debris_visible_parts_by_view"]=visible_parts;
        report["vehicle_original_debris_submitted_parts_by_view"]=visible_parts;
        report["vehicle_original_debris_pixel_visibility_verified"]=false;
        report["vehicle_blast_acceptance_scope"]="Original death/impulse/debris motion, LAN equality and draw submissions; pixel visibility and audio require separate verification";''')]
    new_files = {}
    if args.patch_file:
        recipe = json.loads(args.patch_file.read_text())
        if recipe['path']!=relative or recipe['before_sha256']!=before:
            raise ValueError('Declared visible fixture delta has a different baseline')
        replacements = [(entry['old'],entry['new']) for entry in recipe['replacements']]
        new_files = recipe.get('new_files',{})
        for name,digest in new_files.items():
            if not name.startswith('tests/helpers/') or '..' in Path(name).parts or (source/name).exists():
                raise ValueError('Declared new fixture header is outside its new-file seam')
            if sha(ROOT/name)!=digest:
                raise ValueError('Declared fixture header changed before admission')
        report['patch_recipe'] = dict(path=str(args.patch_file.resolve()),sha256=sha(args.patch_file))
    for old,new in replacements:
        if text.count(old)!=1:
            raise ValueError('The qualified SDL observer patch seam changed')
        text = text.replace(old,new)
    if args.patch_file:
        import hashlib
        if hashlib.sha256(text.encode()).hexdigest()!=recipe['after_sha256']:
            raise ValueError('Declared fixture patch output does not match its reviewed digest')
    path.write_text(text)
    report['patches'][relative] = dict(before=before,after=sha(path))
    for name,digest in new_files.items():
        shutil.copyfile(ROOT/name,source/name)
        report['patches'][name] = dict(before=None,after=digest)
    report['workspace_fixture_matches_qualified_patch'] = sha(ROOT/relative)==sha(path)
    report['cmake_sha256'] = sha(source/'CMakeLists.txt')
    command = [sys.executable,str(ROOT/'tools/run_tests.py'),'--build-worker','--source',str(source),
               '--target','physics_multiplayer_acceptance','--filter','^destruction_lan_identity_tu15$',
               '--jobs',str(args.jobs),'--test-jobs','1','--output',str(args.output/'validation'),
               '--retain-report','destruction-lan-identity.json']
    report['command'] = command;save()
    print(json.dumps(dict(status='validating',report=str(receipt))),flush=True)
    code = subprocess.run(command,stdout=subprocess.DEVNULL,stderr=subprocess.STDOUT).returncode
    report.update(passed=code==0,returncode=code);save()
    print(json.dumps(dict(passed=code==0,returncode=code,report=str(receipt))),flush=True)
    return code


if __name__ == '__main__':
    raise SystemExit(main())
