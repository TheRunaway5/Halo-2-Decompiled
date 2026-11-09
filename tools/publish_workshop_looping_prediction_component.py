"""Install exactly the qualified additive native component and reference.

This changes source and a3943-byte original-derived fixture, never the frozen
feedback build, script admission, selected runtime or launchers. Actual native
HS, MCC resource/streaming and mission acceptance remain required.
Run through run_test.py so regeneration outputs have an owned lifecycle.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--qualification', type=Path, required=True)
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    owner = Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    assert not args.report.resolve().is_relative_to(owner)
    q = json.loads(args.qualification.read_text())
    assert q['passed'] and q['sources_unchanged'] and not q['diagnostic_only']
    assert [(v['compiler'], v['sanitizers'], v['exit_code']) for v in q['results']] == [
        ('g++', False, 0), ('clang++', False, 0), ('clang++', True, 0)]
    assert len(q['negative_controls']) == 4
    assert not q['runtime_enabled'] and q['generation']['all_existing_bodies_unchanged']
    for name, pin in {**q['source_hashes'], **q['additional_current_native_dependencies']}.items():
        assert digest(root / name) == pin, name
    assert digest(Path(q['native_cache_kernel_source'])) == q['native_cache_kernel_sha256']
    destination = root / 'src/campaign/scripting/source_workshop_script_builtins_tu15_generated.cpp'
    before = digest(destination)
    assert before == q['generation']['preserved_module_sha256']
    profile = root / 'tools/mcc-script-layout.json'
    profile_pin = digest(profile)
    declarations = json.loads(profile.read_text())['builtins']
    assert len(declarations) == 412 and not any(v['mcc_opcode'] == 607 for v in declarations)
    publication = {name: digest(root / name) for name in ('Halo2-Launcher', 'Halo2-Title-Launcher',
        'gameplay-runtime-selection.json', 'frontend-runtime-selection.json')}
    audit = owner / 'audit.json'
    audit.write_text(json.dumps(dict(xbox_tu15_sha256=q['generation']['original_sha256'],
        declaration_candidates=q['generation']['candidates']), indent=2) + '\n')
    generated, directory = owner / 'component.cpp', owner / 'generation'
    python = root / '.tools/python/bin/python3'
    command = [str(python), '-B', str(root / 'tools/generate_workshop_script_builtins_tu15.py'),
        '--audit-report', str(audit), '--output', str(generated), '--directory', str(directory),
        '--preserve-module', str(destination), '--names', 'sound_looping_predict',
        '--owned-callees', '0x18d1d0', '0x128a20']
    subprocess.run(command, check=True)
    assert digest(generated) == q['generation']['generated_cpp_sha256']
    fixture = owner / 'workshop_looping_prediction.json.zlib'
    # The oracle report is small retained evidence; raw reference construction
    # and compression remain inside this run_test.py owner.
    subprocess.run([str(python), '-B', str(root / 'tools/audit_workshop_looping_prediction_tu15.py'),
        '--fixture', str(fixture), '--report', str(args.report.with_name(args.report.stem + '-original.json'))], check=True)
    descriptor = json.loads(fixture.with_suffix('').read_text())
    assert descriptor == q['fixture'] and fixture.stat().st_size == 3943
    targets = [(generated, destination),
               (fixture, root / 'tests/fixtures' / fixture.name),
               (fixture.with_suffix(''), root / 'tests/fixtures' / fixture.with_suffix('').name)]
    assert all(not target.exists() for _, target in targets[1:])
    assert digest(destination) == before and digest(profile) == profile_pin
    assert all(digest(root / name) == pin for name, pin in publication.items())
    for path, target in targets:
        os.replace(path, target)
    assert digest(destination) == q['generation']['generated_cpp_sha256']
    assert digest(profile) == profile_pin
    assert all(digest(root / name) == pin for name, pin in publication.items())
    report = dict(scope=__doc__, passed=True, qualification=str(args.qualification),
        qualification_sha256=digest(args.qualification), before_module_sha256=before,
        after_module_sha256=digest(destination), preserved_bodies=len(q['generation']['preserved_body_sha256']),
        new_entries=['0x2b3e10', '0x18d1d0', '0x128a20'],
        qualified_effect_entries=['0x18d1d0', '0x128a20'], HS_caller_qualified=False,
        MCC_projection_and_streaming_qualified=False, production_admission_changed=False,
        script_profile_sha256=profile_pin, publication_identities_unchanged=True,
        required_reference=dict(path=str(targets[1][1]), **descriptor),
        owned_scratch=str(owner), retained_large_artifacts=[],
        complete_mod_support=False)
    args.report.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(dict(passed=True, preserved_bodies=report['preserved_bodies'], reference_bytes=3943)))


if __name__ == '__main__':
    main()
