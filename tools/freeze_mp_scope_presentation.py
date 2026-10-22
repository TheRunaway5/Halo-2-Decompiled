#!/usr/bin/env python3
"""Freeze this independent HUD source/native checkpoint without rewriting prior evidence."""
import argparse
import hashlib
import json
import subprocess
from pathlib import Path

from oracle_smg_firing_1_1 import ROOT, PIN
from research_mp_sandbox_update_functions import NEW_PIN

BASE = ROOT / 'analysis/multiplayer/integration/scope-presentation'


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--native-test', type=Path, required=True)
    parser.add_argument('--sanitized-test', type=Path, required=True)
    parser.add_argument('--zoom-regression-test', type=Path, required=True)
    args = parser.parse_args()
    assert not args.output.exists(), 'Write a fresh manifest; never overwrite this checkpoint'

    files = {}
    pairs = [('measured', 'repeat'), ('controls-measured', 'controls-repeat'),
             ('update-measured', 'update-repeat')]
    for left, right in pairs:
        assert (BASE / left / 'report.json').read_bytes() == (BASE / right / 'report.json').read_bytes()
        for path in sorted((BASE / left).rglob('*')):
            if not path.is_file():
                continue
            if path.name != 'fixtures.tsv':
                paired = BASE / right / path.relative_to(BASE / left)
                assert path.read_bytes() == paired.read_bytes(), path
            files[str(path.relative_to(ROOT))] = sha(path)
        for path in sorted((BASE / right).rglob('*')):
            if path.is_file():
                files[str(path.relative_to(ROOT))] = sha(path)

    audit_path = BASE / 'audit-frozen/audit.json'
    audit = json.loads(audit_path.read_text())
    assert audit['original_xbe_sha256'] == PIN and audit['later_xbe_sha256'] == NEW_PIN
    assert audit['source_instruction_rows'] == 7291
    assert audit['explicitly_modified_branch_cases'] == 309
    for name, digest in audit['source_files'].items():
        assert sha(ROOT / name) == digest, ('audited_file_changed', name)
    for folder in ['audit-frozen', 'prior-native-unit-zoom']:
        for path in sorted((BASE / folder).rglob('*')):
            if path.is_file():
                files[str(path.relative_to(ROOT))] = sha(path)

    prior_path = ROOT / 'analysis/multiplayer/integration/scope-camera/manifest.json'
    assert sha(prior_path) == audit['previous_scope_manifest_sha256']
    prior = json.loads(prior_path.read_text())
    prior_pins = {**prior['source_files'], **prior['verified_prior_pins']}
    for name, digest in prior_pins.items():
        assert sha(ROOT / name) == digest, ('prior_file_changed', name)

    tools = ['oracle_mp_scope_presentation_1_1.py', 'oracle_mp_scope_hud_controls_1_1.py',
             'research_mp_scope_presentation_updates.py', 'export_mp_scope_hud_fixtures.py',
             'audit_mp_scope_presentation.py', 'freeze_mp_scope_presentation.py']
    for name in tools:
        files['tools/' + name] = sha(ROOT / 'tools' / name)
    for name in ['docs/MP_UNIT_ZOOM_HUD_SOURCE_RESEARCH.md', 'include/halo2/unit_zoom.hpp',
                 'src/gameplay/units/unit_zoom.cpp', 'tests/unit_zoom_hud_tests.cpp']:
        files[name] = sha(ROOT / name)

    hud_fixtures = BASE / 'controls-measured/fixtures.tsv'
    zoom_fixtures = ROOT / 'analysis/multiplayer/integration/unit-zoom/measured-frozen/fixtures.tsv'
    tests = []
    for binary, fixture in [(args.native_test, hud_fixtures),
                            (args.sanitized_test, hud_fixtures),
                            (args.zoom_regression_test, zoom_fixtures)]:
        result = subprocess.run([str(binary.resolve()), str(fixture)], cwd=ROOT,
                                capture_output=True, text=True, check=True)
        tests.append(dict(binary=str(binary.resolve()), binary_sha256=sha(binary),
                          fixture=str(fixture.relative_to(ROOT)), fixture_sha256=sha(fixture),
                          returncode=result.returncode, stdout=result.stdout, stderr=result.stderr))

    report = dict(
        schema=1, original_xbe_sha256=PIN, later_xbe_sha256=NEW_PIN,
        independently_identical_report_pairs=pairs, source_files=files,
        previous_scope_manifest_sha256=sha(prior_path), verified_prior_pins=prior_pins,
        native_tests=tests,
        scope='Original CURRENT/binocular raw status and 0x05C0 HUD mask subset; '
              'six original CPU model-request frames and 309 explicit branch controls. '
              'No complete other-HUD state, local-session creation, GPU overlay coverage, '
              'hardware matrix parity or invented weapon-hiding rule is claimed.',
        module_runtime_integration_by_this_agent=False)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(dict(manifest_sha256=sha(args.output), source_files=len(files),
                          prior_pins=len(prior_pins), native_tests=len(tests),
                          doc_sha256=files['docs/MP_UNIT_ZOOM_HUD_SOURCE_RESEARCH.md'])))


if __name__ == '__main__':
    main()
