#!/usr/bin/env python3
"""Restore the canonical production CMake file after same-source qualification."""
import argparse
import hashlib
import json
import os
from pathlib import Path
from build_launcher_engine import source_hashes
from weapon_test_cache import sha

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('prepared', 'validation', 'regression', 'output'):
        parser.add_argument('--' + name, type=Path, required=True)
    args = parser.parse_args()
    owner = Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    source = owner / 'source'
    prepared = json.loads(args.prepared.read_text())
    validation = json.loads(args.validation.read_text())
    regression = json.loads(args.regression.read_text())
    binding = validation.get('vehicle_choice_source_binding', {})
    cohort = regression.get('candidate_vehicle_regression', {})
    if (Path(prepared['owner']) != owner or not validation['passed'] or not regression['passed'] or
            binding.get('preparation_sha256') != sha(args.prepared) or
            not binding.get('source_unchanged_after_run') or
            cohort.get('prepared_sha256') != sha(args.prepared) or cohort.get('expected_cases') != 374 or
            not cohort.get('source_unchanged_after_run') or not cohort.get('incoming_CMake_restored') or
            not cohort.get('all_declared_cases_passed_without_skips')):
        raise ValueError('Delivery needs both complete native cohorts from the same unchanged source epoch')
    if any(sha(source / name) != pin for name, pin in prepared['source_hashes'].items()):
        raise ValueError('Qualified source changed before production restoration')
    if prepared.get('selected_delivery_sha256') and sha(ROOT / 'gameplay-runtime-selection.json') != prepared['selected_delivery_sha256']:
        raise ValueError('A newer selected delivery requires source reconciliation first')
    cmake = source / 'CMakeLists.txt'
    raw = cmake.read_bytes()
    canonical = None
    digest = hashlib.sha256()
    end = 0
    for line in raw.splitlines(keepends=True):
        end += len(line)
        digest.update(line)
        if digest.hexdigest() == prepared['canonical_cmake_sha256']:
            canonical = raw[:end]
            break
    if canonical is None or not raw[len(canonical):].startswith(b'\n# Actual authored Random vehicle births'):
        raise ValueError('Canonical source prefix or declared private registration boundary changed')
    replacements={}
    for row in prepared.get('private_generated_owner_instrumentation',[]):
        path=source/row['path'];text=path.read_text()
        include='#include "halo2/source_physics_entry_observation.hpp"\n'
        if sha(path)!=row['observed_sha256'] or not text.startswith(include):
            raise ValueError('Private constructor diagnostic preimage changed')
        text=text[len(include):]
        for entry in row['entries']:
            needle=f'\nSourcePhysicsEntryObservation diagnostic(s.memory,0x{int(entry,16):x}u,[&]{{return s.save();}});'
            if text.count(needle)!=1:raise ValueError('Private constructor diagnostic scope changed')
            text=text.replace(needle,'',1)
        if hashlib.sha256(text.encode()).hexdigest()!=row['original_sha256']:
            raise ValueError('Diagnostic removal does not restore the complete production preimage')
        replacements[row['path']]=text
    args.output.mkdir(parents=True, exist_ok=False)
    changes=[]
    for name,text in replacements.items():
        path=source/name;before=sha(path);path.write_text(text)
        changes.append(dict(path=name,before=before,after=sha(path),complete_production_preimage_restored=True))
    before = sha(cmake)
    cmake.write_bytes(canonical)
    changes.append(dict(path='CMakeLists.txt',before=before,after=sha(cmake)))
    pins = source_hashes(source)
    (args.output / 'source-hashes.json').write_text(json.dumps(pins, indent=2) + '\n')
    report = dict(passed=True, owner=str(owner), changes=changes,
                  native_match_validation_sha256=sha(args.validation), complete_regression_sha256=sha(args.regression),
                  prepared_sha256=sha(args.prepared), production_bodies_changed=False,
                  launcher_runtime_rebuilt=False, full_vehicle_parity=False, tool_sha256=sha(Path(__file__)))
    (args.output / 'prepared.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(dict(status='canonical', output=str(args.output))), flush=True)


if __name__ == '__main__':
    main()
