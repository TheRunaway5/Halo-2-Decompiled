#!/usr/bin/env python3
"""Prove the native TU1.5 physics world leaves (1F5FE0, 2E4550, 303BA0, 2E47B0,
2E4760):
the logic in native_tu15_world_leaves_systems.hpp through the boundary adapters
in native_tu15_world_leaves.hpp.

Run through tools/run_test.py. Extracts the unmodified canonical translated
bodies from the checked-in physics world unit (pinned; packed SSE with the
production binder's explicit destinations) with their selected() operand cases, compiles tests/native_tu15_world_leaves_tests.cpp against it,
and compares every seeded case (ordered store events with preimages, bank,
registers, flags, XMM, MXCSR, cursor; faults by exception, cursor and store
prefix) in Release, census and verify (record/replay) modes and an ASan/UBSan
build. Requires complete reachable branch coverage. Checks the production
binding (install, idempotence, refusal of a changed body or operand). Then
screens deliberate defects in the logic and the adapters: each mutant is an
out-of-tree copy of one header placed first on the include path and must be
rejected. Source hashes are rechecked after the screen.
"""
import argparse
import concurrent.futures
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[1]
UNIT = ROOT / 'src/physics/runtime/physics_world_runtime_generated.cpp'
HEADER = ROOT / 'src/physics/runtime/native_tu15_world_leaves.hpp'
SYSTEMS = ROOT / 'src/physics/runtime/native_tu15_world_leaves_systems.hpp'
TEST = ROOT / 'tests/native_tu15_world_leaves_tests.cpp'
HELPER = ROOT / 'tools/native_world_leaves_replacements.py'
OWNERS = ('1f16f0', '2e0a60', '300100', '2e0cc0', '2e0c70')
UNREACHABLE = set()
# Deliberate defects in the logic (SYSTEMS) and the boundary adapters (HEADER),
# the same defect classes as the machine-shaped kernels' screen. Each must be
# rejected.
MUTANTS = [
    # 1F5FE0 logic
    (SYSTEMS, 'scale-compare', 'if(!sse::ordered_equal(scale_value,unit)) {', 'if(sse::ordered_equal(scale_value,unit)) {'),
    (SYSTEMS, 'side-select', 'std::int32_t(right)==std::int32_t(walked)?1u:0u;', 'std::int32_t(right)==std::int32_t(walked)?0u:1u;'),
    (SYSTEMS, 'side-sign', 'const auto right=std::int16_t(', 'const auto right=std::uint16_t('),
    (SYSTEMS, 'vertex-stride', 'auto corner=element(vertices,vertex_index,sizeof(BspVertex));', 'auto corner=element(vertices,vertex_index,8u);'),
    (SYSTEMS, 'edge-stride', 'const auto edge=element(edges,edge_index,sizeof(BspEdge));', 'const auto edge=element(edges,edge_index,8u);'),
    (SYSTEMS, 'w-word', 'vertex_id_tag=0x3f000000u;', 'vertex_id_tag=0x3f800000u;'),
    (SYSTEMS, 'translation-operand', 'return sse::add(sse::add(sse::add(up_term,left_term),forward_term),position);',
     'return sse::add(position,sse::add(sse::add(up_term,left_term),forward_term));'),
    (SYSTEMS, 'store-order', '    memory.store_float(point_y,at(slot,offsetof(PolygonVertex,point[1])),y);\n    memory.store_float(point_z,at(slot,offsetof(PolygonVertex,point[2])),z);',
     '    memory.store_float(point_z,at(slot,offsetof(PolygonVertex,point[2])),z);\n    memory.store_float(point_y,at(slot,offsetof(PolygonVertex,point[1])),y);'),
    (SYSTEMS, 'scaled-source', 'y=sse::multiply(memory.load_float(scale_y,at(matrix,offsetof(RealMatrix4x3,scale))),y);', 'y=sse::multiply(x,y);'),
    (SYSTEMS, 'word-width', 'memory.store16(material_store,', 'memory.store32(material_store,'),
    (SYSTEMS, 'byte-field', 'at(shape,offsetof(PolygonShapeHeader,surface_flags)),flags);', 'at(shape,offsetof(PolygonShapeHeader,key_type)),flags);'),
    (SYSTEMS, 'local-read', 'corner=frame.transformed_corner();', 'corner=frame.transformed_corner()+4u;'),
    (SYSTEMS, 'vertex-count', 'offsetof(PolygonShapeHeader,vertex_count)),count+1u);', 'offsetof(PolygonShapeHeader,vertex_count)),count+2u);'),
    # 1F5FE0 adapter
    (HEADER, 'reload-slot', 'return port.read(0x1f603fu,entry+0x10u,4);', 'return port.read(0x1f603fu,entry+0xcu,4);'),
    (HEADER, 'exit-flags', 'gp[esp]=port.flags.plus(gp[esp],0xcu,4);', 'gp[esp]+=0xcu;'),
    (HEADER, 'first-edge-slot', 'port.store(0x1f6039u,entry+0x8u,4,edge);', 'port.store(0x1f6039u,entry+0x4u,4,edge);'),
    (HEADER, 'cursor', 'port.publish(0x1f61a6u);', 'port.publish(0x1f61a5u);'),
    (HEADER, 'ret-bytes', 'port.ret(0x1f61a6u,0x14u);', 'port.ret(0x1f61a6u,0x10u);'),
    (HEADER, 'save-order', 'port.push(0x1f600du,port.gp[ebx]);port.push(0x1f600eu,port.gp[ebp]);',
     'port.push(0x1f600du,port.gp[ebp]);port.push(0x1f600eu,port.gp[ebx]);'),
    (HEADER, 'transform-residue', 'port.set_xmm(6,scalar(t->forward_y_times_x));', 'port.set_xmm(6,scalar(t->left_z_times_y));'),
    # 2E4550 logic and adapter
    (SYSTEMS, 'relative-row-count', 'for(std::uint32_t row=0;row<4;++row) {', 'for(std::uint32_t row=0;row<3;++row) {'),
    (SYSTEMS, 'relative-transpose', 'return vector({lanes(rotation.column[0])[i],lanes(rotation.column[1])[i],z,z});',
     'return vector({lanes(rotation.column[1])[i],lanes(rotation.column[0])[i],z,z});'),
    (SYSTEMS, 'relative-sum-order', 'return {sse::add(sse::add(x_terms,y_terms),z_terms),', 'return {sse::add(sse::add(x_terms,z_terms),y_terms),'),
    (SYSTEMS, 'relative-subtract', 'sse::subtract(rotated_b_translation,rotated_a_translation.value)', 'sse::subtract(rotated_a_translation.value,rotated_b_translation)'),
    (SYSTEMS, 'relative-translation-source', 'memory.load_vector(out_translation,at(out,', 'memory.load_vector(out_translation,at(a,'),
    (HEADER, 'relative-dec-flags', 'port.flags.increment(1u,4,true);', 'port.flags.minus(1u,1u,4);'),
    (HEADER, 'relative-stack-align', 'gp[esp]=(gp[esp]&0xfffffff0u)-8u;', 'gp[esp]=(gp[esp]&0xfffffff8u)-8u;'),
    (HEADER, 'relative-edi-restore', 'port.reached(b_translation);port.gp[edi]=port.pop(0x2e4639u);', 'port.reached(b_translation);'),
    # 303BA0 logic and adapter
    (SYSTEMS, 'copy-row', 'vector_field(second_position,offsetof(ContactRecord,second_contact.position));', 'vector_field(second_position,offsetof(ContactRecord,vector_40));'),
    (HEADER, 'copy-byte', 'gp[ecx]=(source&~0xffu)|residue.byte_7d;', 'gp[ecx]=residue.byte_7d;'),
    (HEADER, 'copy-ret', 'port.ret(0x303bfeu,4u);', 'port.ret(0x303bfeu,0u);'),
    # 2E47B0 logic and adapter
    (SYSTEMS, 'rotate-lane', 'sse::multiply(transposed_column(rotation,1),splat(v,1));', 'sse::multiply(transposed_column(rotation,1),splat(v,2));'),
    (SYSTEMS, 'rotate-operand', 'sse::multiply(transposed_column(rotation,2),splat(v,2));', 'sse::multiply(splat(v,2),transposed_column(rotation,2));'),
    (HEADER, 'rotate-flags', 'gp[esp]=port.flags.logic(gp[esp]&0xfffffff0u,4);', 'gp[esp]&=0xfffffff0u;'),
    # 2E4760 logic and adapter
    (SYSTEMS, 'forward-sum-order', 'return {sse::add(sse::add(x_term,y_term),z_term),splat(v,0),y_term,z_term};',
     'return {sse::add(sse::add(y_term,x_term),z_term),splat(v,0),y_term,z_term};'),
    (SYSTEMS, 'forward-operand', 'const auto y_term=sse::multiply(rotation.column[1],splat(v,1));',
     'const auto y_term=sse::multiply(splat(v,1),rotation.column[1]);'),
    (SYSTEMS, 'forward-column', 'offsetof(HavokTransform,column[0])));\n    const auto rotated=rotate(rotation,v);',
     'offsetof(HavokTransform,column[1])));\n    const auto rotated=rotate(rotation,v);'),
    (HEADER, 'forward-eax', 'gp[eax]=frame.rotation_address;', 'gp[eax]=out;'),
    (HEADER, 'forward-residue', 'port.set_xmm(0,rotated.value);port.set_xmm(1,rotated.x_splat);', 'port.set_xmm(0,rotated.value);port.set_xmm(1,rotated.y_term);'),
    (HEADER, 'forward-flags', 'if(a==logic::havok_rotation::ForwardAccess::result)port.gp[esp]=port.flags.logic(port.gp[esp]&0xfffffff0u,4);',
     'if(a==logic::havok_rotation::ForwardAccess::result)port.gp[esp]&=0xfffffff0u;'),
    (HEADER, 'forward-ret', 'port.ret(0x2e47a4u,8u);', 'port.ret(0x2e47a4u,4u);'),
    # remap admission (the phantom/world composition)
    (HEADER, 'remap-delta-2e4760', 'inline constexpr Function f_2e4760[]{{0x2e4760u,0x2e47a4u,0x3af0u,0x3af0u}};',
     'inline constexpr Function f_2e4760[]{{0x2e4760u,0x2e47a4u,0x3af0u,0x3b00u}};'),
    (HEADER, 'remap-store-pc', '        return pc-function(pc).published_offset;',
     '        return pc;'),
    (HEADER, 'remap-function-range', 'for(const auto &f:remapped)if(pc>=f.first && pc<=f.last)return f;',
     'for(const auto &f:remapped)if(pc>=f.first && pc<f.last)return f;'),
]


REMAP = Path(__file__).resolve().with_name('native_world_leaves_remap_sites.py')


def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def helper(path=HELPER):
    spec = importlib.util.spec_from_file_location('native_world_leaves_replacements', path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def compile_harness(scratch, name, includes, sanitizers=False):
    binary = scratch / name
    command = ['c++', '-std=c++20', '-O2', '-fno-fast-math', '-ffp-contract=off', '-msse2',
               *[f'-I{path}' for path in includes], f'-I{ROOT / "include"}', f'-I{ROOT / "src"}',
               f'-I{scratch}', str(TEST), '-o', str(binary)]
    if sanitizers:
        command[1:1] = ['-fsanitize=address,undefined', '-fno-sanitize-recover=all', '-fno-omit-frame-pointer']
    subprocess.run(command, check=True, cwd=scratch)
    return binary


def run(binary, cases, mode=''):
    command = [str(binary), str(cases)] + ([mode] if mode else [])
    return json.loads(subprocess.run(command, check=True, capture_output=True, text=True).stdout)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--report', type=Path, required=True)
    parser.add_argument('--cases', type=int, default=20000)
    parser.add_argument('--mutant-cases', type=int, default=4000)
    parser.add_argument('--jobs', type=int, default=4)
    parser.add_argument('--skip-sanitizers', action='store_true')
    parser.add_argument('--skip-mutants', action='store_true')
    parser.add_argument('--helper', type=Path, default=HELPER, help='binder module (default: the checked-in one)')
    args = parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):
        parser.error('Run through tools/run_test.py')
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    spec = importlib.util.spec_from_file_location('native_world_leaves_remap_sites', REMAP)
    remap_sites = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(remap_sites)
    remap_sites.write(scratch, 'world_sites')
    sources = [UNIT, HEADER, SYSTEMS, TEST, args.helper.resolve(), Path(__file__).resolve()] + remap_sites.SOURCES
    before = {(str(path.relative_to(ROOT)) if path.is_relative_to(ROOT) else str(path)): digest(path) for path in sources}
    text = UNIT.read_text()
    module = helper(args.helper)
    if module.admitted(text) != list(OWNERS):
        raise ValueError(f'Canonical world leaf bodies drifted from their pins: {module.admitted(text)}')
    cases = set()
    for name in OWNERS:
        found = module.body(text, name)
        (scratch / f'world_{name}.inc').write_text(module.reference(found[0]) + '\n')
        cases.update(module.PINS[name][3].items())
    (scratch / 'world_selected.inc').write_text(''.join(f'case 0x{a:x}u:return 0x{b:x}u;\n' for a, b in sorted(cases)))
    # Production binding: installs, idempotent, refuses a changed body or a
    # changed operand binding.
    output, installed = module.install(text, text)
    if installed != [module.PINS[name][1] for name in OWNERS] or any(module.prefix(name) not in output for name in OWNERS):
        raise ValueError('World leaf binding did not install')
    if module.install(text, output)[1]:
        raise ValueError('World leaf binding is not idempotent')
    original = module.body(text, '1f16f0')[0]
    changed = text.replace(original, original.replace('0x3f000000u', '0x3f000001u', 1), 1)
    if changed == text or '1f5fe0' in module.install(changed, changed)[1]:
        raise ValueError('World leaf binding accepted a changed 1F5FE0 body')
    remapped = text.replace('case 0x486570u:return 0x48b880u;', 'case 0x486570u:return 0x48b884u;', 1)
    if remapped == text or '1f5fe0' in module.install(remapped, remapped)[1]:
        raise ValueError('World leaf binding accepted a changed operand mapping')
    report = dict(passed=False, unit=str(UNIT.relative_to(ROOT)), pins={k: v[0] for k, v in module.PINS.items()})
    release = compile_harness(scratch, 'world-release', [])
    result = run(release, args.cases)
    census = run(release, max(args.cases // 4, 1000), 'census')
    verify = run(release, max(args.cases // 4, 1000), 'verify')
    # The phantom/world composition's canonical-PC remap (release, census, verify).
    report['remap'] = remap_sites.run_modes(run, release, max(args.cases // 4, 1000))
    block = re.sub(r'//[^\n]*', '', re.search(r'enum Branch : unsigned \{(.*?)\};', HEADER.read_text(), re.S)[1])
    names = [name for name in re.sub(r'\s', '', block).split(',') if name and name != 'branch_count']
    coverage = result['coverage'] | verify['coverage']
    hit = {name for index, name in enumerate(names) if coverage >> index & 1}
    missing = sorted(set(names) - hit - UNREACHABLE)
    report.update(release=result, census=census, verify=verify, branches=names, covered=sorted(hit), missing=missing)
    if missing:
        raise ValueError(f'Uncovered native world leaf branches: {missing}')
    if len(result['per_owner']) != len(OWNERS) or 0 in result['per_owner'] + verify['per_owner']:
        raise ValueError(f"A native world leaf was never compared: {result['per_owner']}")
    if census['coverage'] == 0 or verify['coverage'] == 0:
        raise ValueError('Census or verify mode never admitted the native leaves')
    if not args.skip_sanitizers:
        sanitized = compile_harness(scratch, 'world-sanitized', [], sanitizers=True)
        report['sanitized'] = run(sanitized, max(args.cases // 5, 1000))
        report['sanitized_verify'] = run(sanitized, 1000, 'verify')
        sanitized.unlink()
    texts = {path: path.read_text() for path in (HEADER, SYSTEMS)}

    def screen(mutant):
        path, name, old, new = mutant
        if texts[path].count(old) != 1:
            return dict(name=name, rejected=False, error='mutation anchor missing')
        directory = scratch / 'mutants' / name / 'physics/runtime'
        directory.mkdir(parents=True)
        (directory / path.name).write_text(texts[path].replace(old, new, 1))
        try:
            binary = compile_harness(scratch, f'mutant-{name}', [scratch / 'mutants' / name])
        except subprocess.CalledProcessError:
            return dict(name=name, rejected=False, error='mutant failed to compile')
        runs = subprocess.run([str(binary), str(args.mutant_cases)], capture_output=True, text=True)
        mode = 'release'
        if runs.returncode == 0:
            # The composition's remap (admission, cursor and store PCs).
            runs = subprocess.run([str(binary), str(max(args.mutant_cases // 3, 200)), 'remap'], capture_output=True, text=True)
            mode = 'remap'
        binary.unlink()
        return dict(name=name, rejected=runs.returncode != 0, mode=mode, diagnostic=runs.stderr.strip()[-200:])
    mutants = []
    if not args.skip_mutants:
        with concurrent.futures.ThreadPoolExecutor(args.jobs) as pool:
            mutants = list(pool.map(screen, MUTANTS))
    report['mutants'] = mutants
    after = {(str(path.relative_to(ROOT)) if path.is_relative_to(ROOT) else str(path)): digest(path) for path in sources}
    report['source_sha256'] = after
    if after != before:
        raise ValueError('Sources changed during the mutation screen')
    survivors = [m['name'] for m in mutants if not m['rejected']]
    report['passed'] = not survivors
    report['survivors'] = survivors
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2, sort_keys=True) + '\n')
    if survivors:
        raise ValueError(f'Mutants survived: {survivors}')
    print(f"PASS {result['compared']} native world leaf comparisons ({result['ordered_stores']} ordered stores, "
          f"{result['faults']} faults), census {census['compared']}, verify {verify['compared']}, "
          f"{len(mutants)} mutants rejected")
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
