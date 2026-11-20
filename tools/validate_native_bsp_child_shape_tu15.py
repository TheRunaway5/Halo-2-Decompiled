#!/usr/bin/env python3
"""Prove the native TU1.5 1F7C10 BSP child shape: the logic in
native_tu15_world_leaves_systems.hpp (bsp_child_shape::child_shape) through its
boundary adapter in native_tu15_world_leaves.hpp (kernel_1f7c10).

Run through tools/run_test.py. Extracts the unmodified canonical translated
body (run_1f3320; pinned by tools/native_bsp_child_shape_replacements.py), its
selected() operand cases and the binder's verification reference, and compiles
tests/native_tu15_bsp_child_shape_tests.cpp against them. Compares every
seeded case (ordered store events with preimages, bank, registers, flags,
XMM, x87, MXCSR, cursor; faults by exception, cursor and bank) in Release,
census and verify (record/replay with every child boundary) modes and an
ASan/UBSan build, and requires every branch. Checks the production binding
(install, idempotence, refusal of a changed body, operand or dispatch site).
Then screens deliberate defects in the logic and the adapter: each mutant is an
out-of-tree copy of one header placed first on the include path and must be
rejected in release or verify mode. Source hashes are rechecked after the
screen.
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
TEST = ROOT / 'tests/native_tu15_bsp_child_shape_tests.cpp'
HELPER = ROOT / 'tools/native_bsp_child_shape_replacements.py'
MUTANTS = [
    # logic
    (SYSTEMS, 'child-filter', 'if(!frame.may_collide(key)) {', 'if(frame.may_collide(key)) {'),
    (SYSTEMS, 'child-empty-count', 'memory.store16(empty_count,at(buffer,offsetof(ReferencedObject,reference_count)),1u);',
     'memory.store16(empty_count,at(buffer,offsetof(ReferencedObject,reference_count)),0u);'),
    (SYSTEMS, 'child-empty-order', 'memory.store32(empty_user_data,at(buffer,offsetof(ReferencedObject,user_data)),0u);\n        memory.store32(empty_vtable,buffer,empty_shape_vtable);',
     'memory.store32(empty_vtable,buffer,empty_shape_vtable);\n        memory.store32(empty_user_data,at(buffer,offsetof(ReferencedObject,user_data)),0u);'),
    (SYSTEMS, 'child-empty-vtable', 'inline constexpr std::uint32_t empty_shape_vtable=0x462180u;', 'inline constexpr std::uint32_t empty_shape_vtable=0x462184u;'),
    (SYSTEMS, 'child-surface-kind', 'inline constexpr std::uint32_t structure_surface_kind=1u,instance_kind=2u,instance_surface_kind=5u;',
     'inline constexpr std::uint32_t structure_surface_kind=1u,instance_kind=2u,instance_surface_kind=4u;'),
    (SYSTEMS, 'child-owner', 'inline constexpr std::uint32_t no_owner=0xffffffffu;', 'inline constexpr std::uint32_t no_owner=0u;'),
    (SYSTEMS, 'child-instance-stride', 'const auto instance=memory.load32(instance_list,at(structure,structure_instances))+index*instance_size;',
     'const auto instance=memory.load32(instance_list,at(structure,structure_instances))+index*0x50u;'),
    (SYSTEMS, 'child-definition-sign', 'const auto definition=std::uint32_t(std::int32_t(std::int16_t(memory.load16(instance_definition_index,',
     'const auto definition=std::uint32_t(std::int32_t(std::uint16_t(memory.load16(instance_definition_index,'),
    (SYSTEMS, 'child-object-offset', 'inline constexpr std::uint32_t collision_shape_object=0x40u;', 'inline constexpr std::uint32_t collision_shape_object=0x44u;'),
    (SYSTEMS, 'child-release', 'memory.store16(reference_count_store,count,memory.load16(reference_count,count)-1u);',
     'memory.store16(reference_count_store,count,memory.load16(reference_count,count)+1u);'),
    (SYSTEMS, 'child-release-test', 'if(memory.load16(reference_count_check,count)==0) {', 'if(memory.load16(reference_count_check,count)==1) {'),
    (SYSTEMS, 'child-static', 'if(frame.is_static(object))memory.store16(reference_kept,count,1u);',
     'if(!frame.is_static(object))memory.store16(reference_kept,count,1u);'),
    (SYSTEMS, 'child-placed', 'inline constexpr std::uint32_t placed_transform=0x10u;', 'inline constexpr std::uint32_t placed_transform=0x14u;'),
    (SYSTEMS, 'child-no-buffer', 'const auto shape=buffer?frame.build_instance_shape(buffer,object):0u;',
     'const auto shape=frame.build_instance_shape(buffer,object);'),
    (SYSTEMS, 'child-surface-index', 'const auto instance=parts.index*instance_size+instances;', 'const auto instance=parts.secondary*instance_size+instances;'),
    (SYSTEMS, 'child-collision-offset', 'inline constexpr std::uint32_t collision_data=0xb4u;', 'inline constexpr std::uint32_t collision_data=0xb8u;'),
    (SYSTEMS, 'child-surface-empty', 'const auto buffer=memory.load32(surface_buffer,local.buffer);\n        if(!buffer)return 0;',
     'const auto buffer=memory.load32(surface_buffer,local.buffer);'),
    # adapter
    (HEADER, 'child-this-slot', 'port.store(0x1f7c18u,entry-4u,4,reg(ecx));', 'port.store(0x1f7c18u,entry-4u,4,reg(esi));'),
    (HEADER, 'child-decode-shift', 'reg(ecx)=flags.right(reg(esi),16u,4,false);reg(edx)=reg(esi);',
     'reg(ecx)=flags.right(reg(esi),15u,4,false);reg(edx)=reg(esi);'),
    (HEADER, 'child-decode', 'reg(eax)=flags.logic(reg(eax)&0xffffu,4);reg(ecx)=flags.logic(reg(ecx)&0x1fffu,4);',
     'reg(eax)=flags.logic(reg(eax)&0xffffu,4);reg(ecx)=flags.logic(reg(ecx)&0xfffu,4);'),
    (HEADER, 'child-polygon-pushes', 'port.push(0x1f7c4eu,reg(esi));port.push(0x1f7c4fu,reg(eax));', 'port.push(0x1f7c4eu,reg(eax));port.push(0x1f7c4fu,reg(esi));'),
    (HEADER, 'child-matrix', 'reg(edi)=port.flags.logic(0u,4);\n        if(reg(ecx)!=buffer', 'reg(edi)=port.flags.logic(1u,4);\n        if(reg(ecx)!=buffer'),
    (HEADER, 'child-ebx-clear', 'if(!built)reg(ebx)=flags.logic(0u,4);  // XOR EBX,EBX: no buffer, no shape', ''),
    (HEADER, 'child-count-compare', 'flags.minus(value,0u,2);  // CMP WORD [ESI+6],0', 'flags.minus(value,0u,4);  // CMP WORD [ESI+6],0'),
    (HEADER, 'child-ebx-push', 'port.push(0x1f7ca8u,reg(ebx));', 'port.push(0x1f7ca8u,reg(edi));'),
    (HEADER, 'child-build-site', 'call_dispatch(port,child_shape_build_target,0x1f33bcu,children.dispatch);',
     'call_dispatch(port,child_shape_build_target,0x1f33bdu,children.dispatch);'),
    (HEADER, 'child-static-cleanup', 'reg(esp)=port.flags.plus(reg(esp),4u,4);\n        port.flags.logic(reg(eax)&0xffu,1);\n        const bool kept=',
     'reg(esp)=port.flags.plus(reg(esp),4u,4);\n        const bool kept='),
    (HEADER, 'child-destroy-flag', 'port.push(0x1f7ceau,1u);reg(ecx)=reg(esi);', 'port.push(0x1f7ceau,0u);reg(ecx)=reg(esi);'),
    (HEADER, 'child-place-site', 'kept?0x1f33eau:0x1f3405u,children.dispatch', 'kept?0x1f3405u:0x1f33eau,children.dispatch'),
    (HEADER, 'child-surface-edx', 'reg(edx)=reg(esi);\n        if(reg(ecx)!=buffer || reg(eax)!=collision', 'if(reg(ecx)!=buffer || reg(eax)!=collision'),
    (HEADER, 'child-other-pushes', 'port.push(0x1f7d48u,reg(eax));port.push(0x1f7d49u,reg(esi));', 'port.push(0x1f7d48u,reg(esi));port.push(0x1f7d49u,reg(eax));'),
    (HEADER, 'child-surface-flags', 'case surface_structure:case other_buffer:flags.minus(reg(edx),2u,4);flags.minus(reg(edx),5u,4);break;',
     'case surface_structure:case other_buffer:flags.minus(reg(edx),2u,4);break;'),
    (HEADER, 'child-exit-pops', 'case Exit::instance:gp[ebx]=port.pop(0x1f7cfcu);gp[edi]=port.pop(0x1f7cfdu);',
     'case Exit::instance:gp[edi]=port.pop(0x1f7cfcu);gp[ebx]=port.pop(0x1f7cfdu);'),
    (HEADER, 'child-empty-eax', 'case Exit::surface_empty:gp[edi]=port.pop(0x1f7c66u);gp[eax]=flags.logic(0u,4);pc=0x1f7c69u;break;',
     'case Exit::surface_empty:gp[edi]=port.pop(0x1f7c66u);pc=0x1f7c69u;break;'),
    (HEADER, 'child-ret', 'port.ret(pc+2u,8u);port.publish(pc+2u);', 'port.ret(pc+2u,4u);port.publish(pc+2u);'),
    (HEADER, 'child-instance-edi', 'case instance_list:reg(eax)=flags.plus(reg(eax),value,4);reg(edi)=reg(eax);break;',
     'case instance_list:reg(eax)=flags.plus(reg(eax),value,4);break;'),
]


REMAP = Path(__file__).resolve().with_name('native_world_leaves_remap_sites.py')


def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def load(path, name):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def generate(module, text, directory):
    """Write the harness inputs (body, reference, selected() cases)."""
    if not module.admitted(text):
        raise ValueError('Canonical 1F7C10 body, operands or children drifted from the pins')
    # The unit's sign_extend() (MOVSX), which the translated body calls.
    extend = re.search(r'std::uint32_t sign_extend\(std::uint32_t x,unsigned n\) \{.*?\n\}', text, re.S)[0]
    (directory / 'child_bodies.inc').write_text(extend + '\n' + f'static void run_{module.NAME}(ObservedWorldState &);\n' +
                                                module.body(text, module.NAME)[0] + '\n')
    (directory / 'child_reference.inc').write_text(module.reference(module.body(text, module.NAME)))
    (directory / 'child_selected.inc').write_text(''.join(f'case 0x{a:x}u:return 0x{b:x}u;\n' for a, b in sorted(module.OPERANDS.items())))


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


def enum_names(text, name):
    block = re.search(r'enum ' + name + r' : unsigned \{(.*?)\};', text, re.S)[1]
    return [entry.split('=')[0] for entry in re.sub(r'\s|//[^\n]*', '', block).split(',') if entry]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--report', type=Path, required=True)
    parser.add_argument('--cases', type=int, default=20000)
    parser.add_argument('--mutant-cases', type=int, default=3000)
    parser.add_argument('--jobs', type=int, default=4)
    parser.add_argument('--skip-sanitizers', action='store_true')
    parser.add_argument('--skip-mutants', action='store_true')
    args = parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):
        parser.error('Run through tools/run_test.py')
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    spec = importlib.util.spec_from_file_location('native_world_leaves_remap_sites', REMAP)
    remap_sites = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(remap_sites)
    remap_sites.write(scratch, 'world_sites')
    sources = [UNIT, HEADER, SYSTEMS, TEST, HELPER, Path(__file__).resolve()] + remap_sites.SOURCES
    before = {str(path.relative_to(ROOT)): digest(path) for path in sources}
    text = UNIT.read_text()
    module = load(HELPER, 'native_bsp_child_shape_replacements')
    generate(module, text, scratch)
    # Production binding: installs, idempotent, refuses a changed body, a
    # changed operand mapping or a changed dispatch site.
    output, installed = module.install(text, text)
    if installed != [module.ENTRY] or module.prefix() not in output or module.REFERENCE not in output:
        raise ValueError(f'1F7C10 binding installed {installed}')
    if module.install(text, output)[1]:
        raise ValueError('1F7C10 binding is not idempotent')
    found = module.body(text, module.NAME)
    changed = text.replace(found[0], found[0].replace('0xffffu', '0xfffeu', 1), 1)
    if changed == text or module.install(changed, changed)[1]:
        raise ValueError('1F7C10 binding accepted a changed body')
    operand = sorted(module.OPERANDS)[0]
    remapped = text.replace(f'case 0x{operand:x}u:return 0x{module.OPERANDS[operand]:x}u;',
                            f'case 0x{operand:x}u:return 0x{module.OPERANDS[operand] + 4:x}u;', 1)
    if remapped == text or module.install(remapped, remapped)[1]:
        raise ValueError('1F7C10 binding accepted a changed operand mapping')
    site = module.SITES[0]
    moved = text.replace(found[0], found[0].replace(f'0x{site:x}u,false)', f'0x{site + 1:x}u,false)', 1), 1)
    if moved == text or module.install(moved, moved)[1]:
        raise ValueError('1F7C10 binding accepted a changed dispatch site')
    report = dict(passed=False, unit=str(UNIT.relative_to(ROOT)), pin=module.PIN)
    release = compile_harness(scratch, 'child-release', [])
    result = run(release, args.cases)
    census = run(release, max(args.cases // 4, 1000), 'census')
    verify = run(release, max(args.cases // 4, 1000), 'verify')
    # The phantom/world composition's canonical-PC remap (release, census, verify).
    report['remap'] = remap_sites.run_modes(run, release, max(args.cases // 4, 1000))
    header = HEADER.read_text()
    # The bits after 2F8630's and 2E4FE0's in coverage word 1.
    offset = 64 + len(enum_names(header, 'ClosestPointBranch')) - 1 + len(enum_names(header, 'SweptPairBranch')) - 1
    branches = [n for n in enum_names(header, 'ChildShapeBranch') if n != 'child_shape_branch_end']
    coverage = (result['coverage'] | result['coverage_high'] << 64) | (verify['coverage'] | verify['coverage_high'] << 64)
    hit = {name for index, name in enumerate(branches) if coverage >> (offset + index) & 1}
    missing = sorted(set(branches) - hit)
    report.update(release=result, census=census, verify=verify, branches=branches, covered=sorted(hit), missing=missing)
    if missing:
        raise ValueError(f'Uncovered 1F7C10 branches: {missing}')
    if census['compared'] == 0 or verify['compared'] == 0 or result['child_calls'] == 0:
        raise ValueError('Census, verify or the children never ran')
    if not args.skip_sanitizers:
        sanitized = compile_harness(scratch, 'child-sanitized', [], sanitizers=True)
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
        # Release compares stores, registers and the cursor; verify also
        # replays every read (order, address, width) against the translated
        # body.
        runs = subprocess.run([str(binary), str(args.mutant_cases)], capture_output=True, text=True)
        mode = 'release'
        if runs.returncode == 0:
            runs = subprocess.run([str(binary), str(max(args.mutant_cases // 3, 200)), 'verify'], capture_output=True, text=True)
            mode = 'verify'
        binary.unlink()
        return dict(name=name, rejected=runs.returncode != 0, mode=mode, diagnostic=runs.stderr.strip()[-200:])
    mutants = []
    if not args.skip_mutants:
        with concurrent.futures.ThreadPoolExecutor(args.jobs) as pool:
            mutants = list(pool.map(screen, MUTANTS))
    report['mutants'] = mutants
    release.unlink()
    after = {str(path.relative_to(ROOT)): digest(path) for path in sources}
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
    print(f"PASS {result['compared']} native 1F7C10 comparisons ({result['ordered_stores']} ordered stores, "
          f"{result['faults']} faults), census {census['compared']}, verify {verify['compared']}, "
          f"{len(mutants)} mutants rejected")
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
