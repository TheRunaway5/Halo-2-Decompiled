#!/usr/bin/env python3
"""Audit the frozen fresh TU1.5 physics-frame cohort and its original CPU comparisons.

This verifies the compiled cohort, not subsequent edits in the shared checkout.
Player construction, populated-world physics and playable delivery stay open.
"""
import hashlib
import json
import xml.etree.ElementTree as ET
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
E = ROOT/'analysis/multiplayer/sandbox-source/physics-native-frame-integration-20261012'
S = E/'source-v2'
V = E/'verification-v1'


def sha(path):
    h = hashlib.sha256()
    with Path(path).open('rb') as f:
        while b := f.read(1 << 20):
            h.update(b)
    return h.hexdigest()


def read(path):
    return json.loads(Path(path).read_text())


def main():
    m = read(E/'source-manifest-v2.json')
    assert sha(E/'source-manifest-v1.json') == m['parent_manifest_sha256']
    for name, pin in {**m['headers'], **m['files']}.items():
        assert sha(S/name) == pin, name
    assert sha(S/'CMakeLists.txt') == m['cmake_sha256']
    assert len(m['fresh_physics_sources']) == m['fresh_physics_source_total'] == 23
    assert set(m['archives']) == {'libhalo2_cache.a'}
    assert sha(S/'reference-libraries/libhalo2_cache.a') == m['archives']['libhalo2_cache.a']['sha256']
    for name, pin in m['world_archive_input_pins'].items():
        assert sha(S/name) == pin, name
    verification = read(E/'verification-manifest-v1.json')
    for name, pin in verification['files'].items():
        assert sha(V/name) == pin, name
    data = read(E/'data-manifest-v1.json')
    assert not data['bios_files']
    for name, pin in data['files'].items():
        assert sha(name) == pin, name
    original_pin = '21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'
    assert any(pin == original_pin for pin in data['files'].values())
    # The shared owner can gain other source paths after this snapshot. The
    # small public frame entry being checked must still be this exact body.
    marker = 'SourcePhysicsWorldRuntimeAbi PhysicsLiveWorldTu15::step_physics('
    frozen = (S/'src/physics/physics_live_world_tu15.cpp').read_text().split(marker)[1].split('\n}\n')[0]
    current = (ROOT/'src/physics/physics_live_world_tu15.cpp').read_text().split(marker)[1].split('\n}\n')[0]
    assert current == frozen
    assert sha(ROOT/'tests/physics_native_frame_tu15_tests.cpp') == m['files']['tests/physics_native_frame_tu15_tests.cpp']
    builds = {}
    expected = {'native_physics_frame_tu15_lockout', 'native_physics_frame_tu15_coagulation'}
    for mode in ('release', 'sanitizer'):
        b = E/f'build-{mode}-v2'
        rows = ET.parse(b/'tests.xml').findall('.//testcase')
        assert len(rows) == 2 and {r.attrib['name'] for r in rows} == expected
        for row in rows:
            assert all(not row.findall(tag) for tag in ('failure', 'error', 'skipped'))
            assert '120 source frames, 20880 ordered writes' in row.findtext('system-out')
            assert 'no player or populated-world claim' in row.findtext('system-out')
        log = (E/f'tests-{mode}-v2.log').read_text()
        assert '100% tests passed' in log
        commands = read(b/'compile_commands.json')
        assert len(commands) == 44
        for row in commands:
            path = Path(row['file'])
            assert path.is_relative_to(S) and sha(path) == m['files'][str(path.relative_to(S))]
            assert all(flag in row['command'] for flag in ('-fno-fast-math', '-ffp-contract=off', '-frounding-math'))
            if mode == 'sanitizer':
                assert '-fsanitize=address,undefined' in row['command']
        builds[mode] = dict(binary_sha256=sha(b/'physics_native_frame_tu15_tests'),
                            tests=2, frames=240, passed=True,
                            compile_commands_sha256=sha(b/'compile_commands.json'), results_sha256=sha(b/'tests.xml'))
    oracle = []
    for map_name in ('lockout', 'coagulation'):
        for frame in (0, 1):
            p = E/f'{map_name}-hardware-v1-{frame}'
            q = read(p/'comparison.json')
            assert q['passed'] and not q['errors'] and q['original_entry'] == '001cc8d0'
            assert q['original_sha256'] == original_pin
            assert q['original_instructions'] == 987
            assert q['ordered_nonstack_stores'] == q['native_nonstack_stores'] == 43
            assert q['final_retained_bytes_compared'] == 58581070
            assert not q['bios_input_used'] and not q['original_execution_native_expected_data_used']
            assert not q['original_instruction_patches'] and q['original_read_policy'] == 'reject'
            assert not q['whole_boot_proof'] and not q['live_multiplayer_physics_parity'] and not q['xbox_hardware_proof']
            assert q['defined_integer_flag_mask'] == 0xcd5
            assert q['original_exit']['x87_control_word'] == q['native_exit']['x87_control_word'] == 0x27f
            for field, name in (
                ('tool_sha256', 'tools/differential_physics_native_frame_hardware_20261012.py'),
                ('hardware_tool_sha256', 'tools/oracle_physics_game_frame_hardware_20261011.py'),
                ('trace_observer_sha256', 'tools/physics_game_frame_hardware_trace_20261011.cpp'),
                ('bridge_input_tool_sha256', 'tools/original_i386_game_frame_bridge_20261011.py')):
                assert q[field] == verification['files'][name]
            for field, name in (('original_input_sha256', 'input.bin'), ('original_output_sha256', 'output.bin'), ('original_trace_sha256', 'trace.json')):
                assert sha(p/name) == q[field]
            for name, pin in q['native_evidence'].items():
                assert sha(E/f'{map_name}-frames-v1/{frame}'/name) == pin
                assert sha(E/f'{map_name}-frames-v2/{frame}'/name) == pin
            oracle.append(dict(map=map_name, frame=frame, report_sha256=sha(p/'comparison.json')))
    rejected = read(E/'lockout-frames-v1/0/original-comparison.json')
    assert rejected['error'] and 'exit ABI differs' in rejected['error']
    assert rejected['original_instructions'] == 987 and rejected['final_retained_bytes_compared'] == 58581070
    trace = (E/'native-files-v2.log').read_text()
    assert 'exited with 0' in trace
    assert all(name not in trace.lower() for name in ('complex_4627', '/bios/', 'eeprom', 'kernel-text', 'ranges.bin', 'values.bin'))
    result = dict(schema=1, scope=__doc__, validator_sha256=sha(__file__),
                  source_manifest_sha256=sha(E/'source-manifest-v2.json'),
                  verification_manifest_sha256=sha(E/'verification-manifest-v1.json'),
                  owned_data_manifest_sha256=sha(E/'data-manifest-v1.json'),
                  reconciliation_sha256=sha(E/'fresh-physics-reconciliation-v2.json'),
                  builds=builds, original_cpu_comparisons=oracle,
                  original_instructions=3948, nonstack_stores=172, compared_bank_bytes_total=234324280,
                  native_file_trace_sha256=sha(E/'native-files-v2.log'), bios_runtime_required=False,
                  sanitizer_scope='All fresh frame/native-owner and 23 physics/platform/heap sources; reused cache archive is uninstrumented',
                  native_clock_advance_tested=False, populated_world_tested=False,
                  full_multiplayer_physics_parity=False, updated_playable_executable_ready=False)
    out = E/'validation-frozen-v2.json'
    raw = json.dumps(result, sort_keys=True, indent=2)+'\n'
    if out.exists():
        assert out.read_text() == raw, 'Preserve the previous immutable audit'
    else:
        out.write_text(raw)
    print('PASS frozen native TU1.5 frame: 4 original CPU comparisons, 2 Release and 2 sanitizer tests; '+sha(out))
    drift = [name for name, pin in {**m['headers'], **m['files']}.items()
             if not (ROOT/name).exists() or sha(ROOT/name) != pin]
    print('Later shared-checkout changes excluded from this audit: '+', '.join(drift))


if __name__ == '__main__':
    main()
