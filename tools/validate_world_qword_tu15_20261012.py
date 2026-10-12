#!/usr/bin/env python3
"""Freeze and seal the TU1.5 qword correction without changing prior evidence."""
import argparse
import ast
import hashlib
import json
import shutil
import xml.etree.ElementTree as ET
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
E = ROOT/'analysis/multiplayer/sandbox-source/world-qword-tu15-20261012'
P = ROOT/'analysis/multiplayer/sandbox-source/phantom-world-tu15-20261011'
W = ROOT/'analysis/multiplayer/integration/physics-engine-20261010'
P_PIN = '9f883c33d30cf5469ff87587cde06623ebb138bc5352da6c2da62c6dbac8e857'
W_PIN = '87abf847fc5688a1e6567fe426b44dc6a1e4427805c798f5363a338411889c94'
OWNED = (
    'src/physics_step_internal.hpp', 'src/physics_world_runtime_generated.cpp',
    'tools/generate_physics_game_frame_runtime_20261011.py',
    'tools/oracle_world_qword_tu15_20261012.py',
    'tools/validate_world_qword_tu15_20261012.py',
    'tests/physics_scalar_width_tests.cpp', 'cmake/PhysicsWorldQwordTu15.cmake',
    'docs/PHYSICS_WORLD_QWORD_TU15_20261012.md')
EXTRA = ('tools/research_physics_game_frame_runtime_20261011.py',
    'tools/oracle_physics_game_frame_hardware_20261011.py',
    'tools/original_i386_game_frame_bridge_20261011.py',
    'tools/physics_game_frame_hardware_trace_20261011.cpp')
REGISTRATION = 'include(cmake/PhysicsWorldQwordTu15.cmake)'
TESTS = {'original_physics_vehicle_runtime_u5','original_physics_vehicle_update_u5',
    'original_physics_vehicle_motion_u5','original_physics_vehicle_motion_math_u5',
    'original_physics_vehicle_contact_state_tu15','original_physics_vehicle_constraint_cleanup_tu15',
    'original_physics_vehicle_environment_state_tu15','original_physics_phantom_force_tu15',
    'original_physics_phantom_world_tu15','original_physics_world_qword_tu15',
    'physics_scalar_widths'}


def sha(p): return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def read(p): return json.loads(Path(p).read_text())


def freeze():
    assert sha(P/'source-manifest-v5.json') == P_PIN
    parent = read(P/'source-manifest-v5.json')
    assert sha(W/'source-manifest-v19.json') == W_PIN
    world = read(W/'source-manifest-v19.json')
    for name, pin in parent['files'].items(): assert sha(P/'source-v5'/name) == pin, name
    for name, pin in world['files'].items(): assert sha(W/'source-v19'/name) == pin, name
    assert (ROOT/'src/physics_world_runtime_generated.cpp').read_text() == (E/'runtime-v1.cpp').read_text()
    old = (W/'source-v19/src/physics_world_runtime_generated.cpp').read_text()
    wrong = 's.fp.push(ext(f(r(s.memory,(selected(0x4655d0u,s.image)),8))));'
    right = 's.fp.push(static_cast<long double>(std::bit_cast<double>(r64(s.memory,(selected(0x4655d0u,s.image))))));'
    assert old.count(wrong) == 1 and old.replace(wrong,right) == (E/'runtime-v1.cpp').read_text()
    guard = '    if (n == 0 || n > 4)\n        throw std::runtime_error("Invalid original physics scalar read width");\n'
    old_header = (P/'source-v5/src/physics_step_internal.hpp').read_text()
    assert old_header.replace('    std::array<std::byte, 4> b{};',guard+'    std::array<std::byte, 4> b{};',1) == (ROOT/'src/physics_step_internal.hpp').read_text()
    origins = {name: P/'source-v5'/name for name in parent['files']}
    origins.update({name: ROOT/name for name in OWNED+EXTRA})
    # Capture local Python imports used by the new oracle/generator. Retained
    # native parents always come from immutable source-v5, never other edits.
    pending, seen = list(origins), set()
    while pending:
        name = pending.pop()
        if name in seen: continue
        seen.add(name)
        if not name.endswith('.py'): continue
        for node in ast.walk(ast.parse(origins[name].read_text())):
            modules = [node.module] if isinstance(node,ast.ImportFrom) and node.module else [v.name for v in node.names] if isinstance(node,ast.Import) else []
            for module in modules:
                paths = (ROOT/'tools'/(module.replace('.','/')+'.py'),ROOT/'tools/reference/halo2-decompiled/tools'/(module.replace('.','/')+'.py'))
                found = next((p for p in paths if p.is_file()),None)
                if found:
                    relative = str(found.relative_to(ROOT))
                    if relative not in origins: origins[relative] = found; pending.append(relative)
    target = E/'source-v2'
    target.mkdir()
    pins = {name: sha(origin) for name,origin in origins.items()}
    for name, origin in sorted(origins.items()):
        dest = target/name; dest.parent.mkdir(parents=True,exist_ok=True); shutil.copyfile(origin,dest)
        assert sha(dest) == pins[name] == sha(origin), name
        if name.endswith(('.hpp','.cpp','.cmake')) and name not in OWNED:
            assert sha(ROOT/name) == pins[name], 'Shared native dependency drift: '+name
    top = (P/'source-v5/CMakeLists.txt').read_text()+'\n'+REGISTRATION+'\n'
    (target/'CMakeLists.txt').write_text(top)
    for name in ('assets','analysis'): (target/name).symlink_to(ROOT/name,target_is_directory=True)
    assert (ROOT/'CMakeLists.txt').read_text().count(REGISTRATION) == 1
    result = dict(schema=1,parity_target='English TU1.5 only',files=pins,owned=OWNED,
        parent_activation_manifest_sha256=P_PIN,parent_world_manifest_sha256=W_PIN,
        top_sha256=sha(target/'CMakeLists.txt'),parent_archives_reused=False,
        scope='Frozen activation source-v5 plus current world-v19 body graph, exact qword/width overlays and TU1.5-only procedure tests. No live world or original result replay in production.')
    (E/'source-manifest-v2.json').write_text(json.dumps(result,sort_keys=True,indent=2)+'\n')
    print(json.dumps(dict(files=len(pins),manifest_sha256=sha(E/'source-manifest-v2.json'))))


def validate():
    manifest = read(E/'source-manifest-v2.json')
    assert manifest['parent_activation_manifest_sha256'] == P_PIN
    assert manifest['parent_world_manifest_sha256'] == W_PIN
    for name,pin in manifest['files'].items(): assert sha(ROOT/name) == pin == sha(E/'source-v2'/name), name
    assert sha(E/'source-v2/CMakeLists.txt') == manifest['top_sha256']
    assert REGISTRATION in (ROOT/'CMakeLists.txt').read_text()
    generated = read(E/'generated-v1.json')
    assert sha(E/'runtime-v1.cpp') == generated['generated_cpp_sha256'] == sha(ROOT/'src/physics_world_runtime_generated.cpp')
    assert generated['generator_sha256'] == sha(ROOT/'tools/generate_physics_game_frame_runtime_20261011.py')
    assert sha(E/'runtime-repeat.cpp') == sha(E/'runtime-v1.cpp')
    assert sha(E/'generated-repeat.json') == sha(E/'generated-v1.json')
    fixture_pin = sha(E/'original-verified/u5-native.json')
    assert fixture_pin == sha(E/'original-repeat/u5-native.json')
    fixture = read(E/'original-verified/u5-native.json')
    assert fixture['image'] == '1.5' and fixture['original_sha256'] == '21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'
    assert fixture['tool_sha256'] == sha(ROOT/'tools/oracle_world_qword_tu15_20261012.py')
    assert fixture['hardware_tool_sha256'] == sha(ROOT/'tools/oracle_physics_game_frame_hardware_20261011.py')
    assert fixture['trace_observer_sha256'] == sha(ROOT/'tools/physics_game_frame_hardware_trace_20261011.cpp')
    assert fixture['unretained_data_read_policy'] == 'reject'
    assert not fixture['original_instruction_patches'] and not fixture['native_expected_data_used'] and not fixture['xbox_hardware_proof']
    assert len(fixture['cases']) == 288 and all(c['complete'] for c in fixture['cases'])
    assert sum(0x2830fe in c['instructions'] for c in fixture['cases']) == 240
    assert fixture['single_stepped_original_instructions'] == 31488
    assert sum(len(c['writes']) for c in fixture['cases']) == 4512
    assert {c['exit']['gp'][0]&255 for c in fixture['cases']} == {0,1}
    assert 'stack smashing detected' in (E/'red-before.log').read_text()
    builds = {}
    for name in ('release','sanitizer'):
        directory = E/('build-'+name)
        tests = ET.parse(directory/'tu15-tests.xml').getroot()
        cases = list(tests.iter('testcase'))
        assert {c.attrib['name'] for c in cases} == TESTS
        assert not list(tests.iter('failure')) and not list(tests.iter('skipped')) and not list(tests.iter('error'))
        deps = read(directory/'dependency-pins.json')
        assert deps['source_manifest_sha256'] == sha(E/'source-manifest-v2.json')
        for p,pin in deps['files'].items(): assert sha(p) == pin, p
        builds[name] = dict(tests=len(cases),results_sha256=sha(directory/'tu15-tests.xml'),
            dependencies_sha256=sha(directory/'dependency-pins.json'),
            test_binary_sha256=sha(directory/'physics_phantom_world_tu15_tests'),
            scalar_test_sha256=sha(directory/'physics_scalar_width_tests'))
    result = dict(schema=1,parity_target='English TU1.5 only',source_manifest_sha256=sha(E/'source-manifest-v2.json'),
        original_hardware_fixture_sha256=fixture_pin,generated_manifest_sha256=sha(E/'generated-v1.json'),
        cases=288,instructions=31488,qword_load_cases=240,ordered_writes=4512,
        builds=builds,live_physics_parity=False,world_body_ownership_still_required=True)
    (E/'validation.json').write_text(json.dumps(result,sort_keys=True,indent=2)+'\n')
    print(json.dumps(result))


if __name__ == '__main__':
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--freeze',action='store_true');a=ap.parse_args()
    freeze() if a.freeze else validate()
