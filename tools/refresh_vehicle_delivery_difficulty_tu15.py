#!/usr/bin/env python3
"""Apply only qualified difficulty routes to the current owned delivery source."""
import argparse
import difflib
import hashlib
import json
import os
from pathlib import Path

from build_launcher_engine import source_hashes
from native_difficulty_parameter_kernel_tu15 import install_native_difficulty_routes, strip_native_difficulty_routes
from weapon_test_cache import sha

ROOT = Path(__file__).resolve().parents[1]
EVIDENCE = ROOT / 'analysis/vehicles-tu15-goal-20261023'


def add_fixture_diagnostics(text):
    replacements = (
        ('#include "halo2/source_damage_model_native_tu15.hpp"\n',
         '#include "halo2/source_damage_model_native_tu15.hpp"\n#include "halo2/source_difficulty_parameter_native_tu15.hpp"\n'),
        ('  SourceDamageModelNativeActivity native_model_before=source_damage_model_native_activity();\n',
         '  SourceDamageModelNativeActivity native_model_before=source_damage_model_native_activity();\n'
         '  SourceDifficultyParameterNativeActivity native_difficulty_before=source_difficulty_parameter_native_activity();\n'),
        ('    std::ofstream output(path);output<<recorded.dump(2)<<\'\\n\';\n',
         '    const auto difficulty=source_difficulty_parameter_native_activity();\n'
         '    recorded["native_difficulty_parameter_activity"]={\n'
         '        {"parameter_calls",difficulty.parameter_calls-native_difficulty_before.parameter_calls},\n'
         '        {"current_difficulty_calls",difficulty.current_difficulty_calls-native_difficulty_before.current_difficulty_calls}};\n'
         '    std::ofstream output(path);output<<recorded.dump(2)<<\'\\n\';\n'),
        ('  report.data["passed"]=true;\n',
         '  if(std::getenv("HALO2_DESTRUCTION_REQUIRE_NATIVE_DIFFICULTY") && name!="campaign_guntower_no_damage")\n'
         '    check(source_difficulty_parameter_native_activity().parameter_calls>native_difficulty_firing_before.parameter_calls,\n'
         '        "Actual source composition executed no native difficulty parameter kernel");\n'
         '  report.data["passed"]=true;\n'),
        ('  bool pending_qualified=false;\n  for(unsigned tick=0;tick<900;++tick){\n',
         '  bool pending_qualified=false;\n'
         '  const auto native_difficulty_firing_before=source_difficulty_parameter_native_activity();\n'
         '  for(unsigned tick=0;tick<900;++tick){\n'),
        ('  report.data.update({{"shots",shots},{"minimum_body",minimum},{"source_dead",dead},\n',
         '  const auto native_difficulty_firing_after=source_difficulty_parameter_native_activity();\n'
         '  report.data["native_difficulty_firing_activity"]={\n'
         '      {"parameter_calls",native_difficulty_firing_after.parameter_calls-native_difficulty_firing_before.parameter_calls},\n'
         '      {"current_difficulty_calls",native_difficulty_firing_after.current_difficulty_calls-native_difficulty_firing_before.current_difficulty_calls}};\n'
         '  report.data.update({{"shots",shots},{"minimum_body",minimum},{"source_dead",dead},\n'))
    for before, after in replacements:
        if text.count(before) != 1: raise ValueError('Frozen destruction diagnostic anchor changed')
        text = text.replace(before, after, 1)
    return text


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--manifest', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    owner = Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True); source = owner / 'source'
    if source.is_symlink() or source_hashes(source) != json.loads(args.manifest.read_text()):
        raise ValueError('Canonical source changed after the declared delivery build')
    for command in json.loads((EVIDENCE / 'session-v1/session.json').read_text())['commands']:
        if 'finished' not in command and not any('\n' not in arg and Path(arg).name == Path(__file__).name for arg in command['argv']):
            raise ValueError('Another source/build command is still active')
    admission_path = EVIDENCE / 'native-difficulty-routes-v1/admission.json'
    qualification_path = EVIDENCE / 'native-difficulty-installed-v3/report.json'
    admission = json.loads(admission_path.read_text()); qualified = json.loads(qualification_path.read_text())
    installer = ROOT / 'tools/native_difficulty_parameter_kernel_tu15.py'
    if (not admission['passed'] or admission['installer_sha256'] != sha(installer) or
            not qualified['passed'] or not qualified['production_route_installed'] or
            qualified['fault_cases'] != 40 or qualified['cases'] != 528):
        raise ValueError('Installed native difficulty qualification is incomplete')
    changes = {}
    for row in admission['routes']:
        name = row['path']; before = (source / name).read_text()
        after = install_native_difficulty_routes(before, row['route'])
        if strip_native_difficulty_routes(after) != before:
            raise ValueError('Difficulty admission changes another frozen body')
        changes[name] = after.encode()
    for name in ('src/gameplay/objects/source_difficulty_parameter_native_tu15_internal.hpp',
                 'include/halo2/source_difficulty_parameter_native_tu15.hpp'):
        if sha(ROOT / name) != qualified['source_hashes'][name]:
            raise ValueError('Qualified difficulty kernel/header changed')
        changes[name] = (ROOT / name).read_bytes()
    changes['tools/native_difficulty_parameter_kernel_tu15.py'] = installer.read_bytes()
    for name in ('tools/generate_source_direct_damage_runtime.py', 'tools/generate_source_vitality_runtime.py'):
        text = (source / name).read_text()
        anchor = '    a.output.write_text(install_paired_model_lookup(a.output.read_text()))\n'
        if text.count(anchor) != 1: raise ValueError('Frozen paired generator anchor changed')
        changes[name] = text.replace(anchor, anchor +
            '    from native_difficulty_parameter_kernel_tu15 import install_native_difficulty_routes\n'
            '    a.output.write_text(install_native_difficulty_routes(a.output.read_text(),\'canonical\'))\n', 1).encode()
    name = 'tools/generate_ai_frame_tu15.py'; text = (source / name).read_text()
    anchor = '    a.output.write_text(add_hooks(source))\n'
    if text.count(anchor) != 1: raise ValueError('Frozen AI generator anchor changed')
    changes[name] = text.replace(anchor,
        '    from native_difficulty_parameter_kernel_tu15 import install_native_difficulty_routes\n'
        '    a.output.write_text(install_native_difficulty_routes(add_hooks(source),\'actual\'))\n', 1).encode()
    name = 'tools/fix_damage_delivery_observation_tu15.py'; text = (source / name).read_text()
    baseline = "BASELINE_SHA256='32a7d51b0159e2927cf34a75dcf5d00b6be28c42bd2ae42b3e5465c7f45a6142'\n"
    if text.count(baseline) != 1: raise ValueError('Frozen canonical baseline guard changed')
    text = text.replace(baseline, baseline +
        "QUALIFIED_BASELINES={BASELINE_SHA256,\n    'ec3083adcfe0d956ca81bb726e2c8065fb6f0342a646a6086d952afa469d8780'}\n", 1)
    old_guard = "    assert hashlib.sha256(source.encode()).hexdigest()==BASELINE_SHA256,'Damage source changed; retain and re-audit its independent edits'\n"
    if text.count(old_guard) != 1: raise ValueError('Frozen canonical guard statement changed')
    text = text.replace(old_guard,
        "    canonical_pin=hashlib.sha256(source.encode()).hexdigest()\n"
        "    assert canonical_pin in QUALIFIED_BASELINES,'Damage source changed; retain and re-audit its independent edits'\n", 1)
    old_field = 'canonical_baseline_sha256=BASELINE_SHA256,'
    if text.count(old_field) != 1: raise ValueError('Frozen canonical baseline report changed')
    text = text.replace(old_field, 'canonical_baseline_sha256=canonical_pin,', 1)
    for anchor, addition in (
        ('def canonical(source):\n', '    from native_difficulty_parameter_kernel_tu15 import strip_native_difficulty_routes\n    source=strip_native_difficulty_routes(source)\n'),
        ('    output=install_native_model_lookup(output)\n', '    from native_difficulty_parameter_kernel_tu15 import install_native_difficulty_routes\n    output=install_native_difficulty_routes(output,\'delivery\')\n')):
        if text.count(anchor) != 1: raise ValueError('Frozen observation generator anchor changed')
        text = text.replace(anchor, anchor + addition, 1)
    changes[name] = text.encode()
    fixture = 'tests/destruction_runtime_tu15_tests.cpp'
    changes[fixture] = add_fixture_diagnostics((source / fixture).read_text()).encode()
    # Register only the changed damage consumer cohort, retaining canonical
    # CMake bytes so the subsequent production build can remove registrations.
    cohort = json.loads((EVIDENCE / 'delivery-cohort-v2/prepared.json').read_text())
    prepared = json.loads((EVIDENCE / 'delivery-snapshot-v2/prepared.json').read_text())
    maps = Path(prepared['runtime_cache_inputs']['maps_directory'])
    cmake = (source / 'CMakeLists.txt').read_text()
    append = '\n# VEHICLE_DIFFICULTY_NATIVE_QUALIFICATION\nif(NOT TARGET destruction_runtime_tu15_tests)\n  add_executable(destruction_runtime_tu15_tests tests/destruction_runtime_tu15_tests.cpp)\n  target_link_libraries(destruction_runtime_tu15_tests PRIVATE halo2_systems halo2_authority_codec OpenSSL::Crypto)\nendif()\n'
    for case, map_name in cohort['destruction_cases'].items():
        append += (f'add_test(NAME vehicle_difficulty_destruction_{case} COMMAND "${{Python3_EXECUTABLE}}" "${{CMAKE_CURRENT_SOURCE_DIR}}/tools/run_test.py" -- '
            f'$<TARGET_FILE:destruction_runtime_tu15_tests> "{maps / (map_name + ".map")}" "{maps}" "{case}" "${{CMAKE_CURRENT_BINARY_DIR}}/vehicle-difficulty-{case}.json")\n'
            f'set_tests_properties(vehicle_difficulty_destruction_{case} PROPERTIES TIMEOUT 300 RUN_SERIAL TRUE ENVIRONMENT '
            '"HALO2_DESTRUCTION_CONTINUATION=1;HALO2_DESTRUCTION_FIXED_CLOCK=1;HALO2_DESTRUCTION_REQUIRE_NATIVE_DIFFICULTY=1")\n')
    changes['CMakeLists.txt'] = (cmake + append).encode()
    args.output.mkdir(parents=True, exist_ok=False); rows = []
    for name, after in changes.items():
        path = source / name; before = path.read_bytes() if path.exists() else b''
        before_pin = sha(path) if path.exists() else None
        path.parent.mkdir(parents=True, exist_ok=True); path.write_bytes(after)
        rows.append(dict(path=name, before=before_pin, after=sha(path)))
        (args.output / (name.replace('/', '__') + '.patch')).write_text(''.join(difflib.unified_diff(
            before.decode().splitlines(True), after.decode().splitlines(True), fromfile='before/' + name, tofile='after/' + name, n=2)))
    updated = source_hashes(source)
    (args.output / 'source-hashes.json').write_text(json.dumps(updated, indent=2) + '\n')
    report = dict(owner=str(owner), validation_not_run=True, changes=rows,
        base_manifest_sha256=sha(args.manifest), admission_sha256=sha(admission_path),
        installed_qualification_sha256=sha(qualification_path), destruction_cases=cohort['destruction_cases'],
        canonical_cmake_sha256=hashlib.sha256(cmake.encode()).hexdigest(),
        canonical_cmake_bytes=len(cmake.encode()), full_vehicle_parity=False, tool_sha256=sha(Path(__file__)))
    (args.output / 'prepared.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(dict(status='prepared', files=len(rows), report=str(args.output / 'prepared.json'))), flush=True)


if __name__ == '__main__':
    main()
