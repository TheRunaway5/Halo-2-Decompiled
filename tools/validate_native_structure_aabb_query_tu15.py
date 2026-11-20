#!/usr/bin/env python3
"""Prove the native TU1.5 1F6D10 structure AABB query: the logic in
native_tu15_world_leaves_systems.hpp (structure_aabb_query::query_aabb) through
its boundary adapter in native_tu15_world_leaves.hpp (kernel_1f6d10, with
2E4550 inline).

Run through tools/run_test.py. Extracts the unmodified canonical translated
body (run_1f2420; pinned by tools/native_structure_aabb_query_replacements.py)
and the translated 2E4550 its reference runs, their selected() operand cases
and the binder's verification reference, and compiles
tests/native_tu15_structure_aabb_query_tests.cpp against them. Compares every
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
TEST = ROOT / 'tests/native_tu15_structure_aabb_query_tests.cpp'
HELPER = ROOT / 'tools/native_structure_aabb_query_replacements.py'
MUTANTS = [
    # logic
    (SYSTEMS, 'query-kind', 'kind_mask=0xe0000000u,instance_kind_bits=0x40000000u,', 'kind_mask=0xe0000000u,instance_kind_bits=0x20000000u,'),
    (SYSTEMS, 'query-index-mask', 'instance_kind_bits=0x40000000u,index_mask=0xffffu;', 'instance_kind_bits=0x40000000u,index_mask=0x7fffu;'),
    (SYSTEMS, 'query-expandable', 'definition_surface_count)))>expandable_surfaces)return index;',
     'definition_surface_count)))>=expandable_surfaces)return index;'),
    (SYSTEMS, 'query-count-offset', 'inline constexpr std::uint32_t definition_surface_count=0x98u;',
     'inline constexpr std::uint32_t definition_surface_count=0x9cu;'),
    (SYSTEMS, 'query-count-sign', 'if(std::int32_t(memory.load32(surface_count,at(definition,definition_surface_count)))>expandable_surfaces)',
     'if(memory.load32(surface_count,at(definition,definition_surface_count))>std::uint32_t(expandable_surfaces))'),
    (SYSTEMS, 'query-definition-sign', 'const auto offset=std::uint32_t(std::int32_t(std::int16_t(memory.load16(definition_index,',
     'const auto offset=std::uint32_t(std::int32_t(std::uint16_t(memory.load16(definition_index,'),
    (SYSTEMS, 'query-shape-offset', 'inline constexpr std::uint32_t instance_query_shape=0x50u;', 'inline constexpr std::uint32_t instance_query_shape=0x54u;'),
    (SYSTEMS, 'query-vtable-slot', 'inline constexpr std::uint32_t query_obb=0x2cu;', 'inline constexpr std::uint32_t query_obb=0x30u;'),
    (SYSTEMS, 'query-tolerance', 'inline constexpr std::uint32_t query_tolerance=0x38d1b717u;', 'inline constexpr std::uint32_t query_tolerance=0x38d1b718u;'),
    (SYSTEMS, 'query-subobject', 'inline constexpr std::uint32_t filter_subobject=0x14u;', 'inline constexpr std::uint32_t filter_subobject=0x10u;'),
    (SYSTEMS, 'query-surface-fill', 'inline constexpr std::uint32_t surface_kind_fill=0xffffa000u;', 'inline constexpr std::uint32_t surface_kind_fill=0xffff8000u;'),
    (SYSTEMS, 'query-surface-shift', 'const auto surface_key=((surface|surface_kind_fill)<<16)|', 'const auto surface_key=((surface|surface_kind_fill)<<15)|'),
    (SYSTEMS, 'query-resume', 'const auto resume=index-1u;', 'const auto resume=index;'),
    (SYSTEMS, 'query-kept-step', 'if(kept)++k;  // a removed key', '++k;  // a removed key'),
    (SYSTEMS, 'query-removal-sign', 'for(auto bound=size;std::int32_t(index)<std::int32_t(bound);++index) {',
     'for(auto bound=size;index<bound;++index) {'),
    (SYSTEMS, 'query-removal-bound', 'bound=memory.load32(role.size_reload,size_address);', 'memory.load32(role.size_reload,size_address);'),
    (SYSTEMS, 'query-box-order', 'put(column(0,0),forward_x);\n    const auto forward_y=field(offsetof(RealMatrix4x3,forward[1]));',
     'const auto forward_y=field(offsetof(RealMatrix4x3,forward[1]));\n    put(column(0,0),forward_x);'),
    (SYSTEMS, 'query-box-column', 'put(column(2,2),up_z);', 'put(column(2,1),up_z);'),
    (SYSTEMS, 'query-half-order', 'sse::multiply(splat(vector({half_factor,0u,0u,0u}),0),sse::subtract(maximum,minimum))',
     'sse::multiply(sse::subtract(maximum,minimum),splat(vector({half_factor,0u,0u,0u}),0))'),
    (SYSTEMS, 'query-center-order', 'const auto center=sse::add(minimum,half_extents);', 'const auto center=sse::add(half_extents,minimum);'),
    (SYSTEMS, 'query-diagonal', 'memory.store32(one_store,at(local.box_transform,c*sizeof(Lanes)+c*4u),unit);',
     'memory.store32(one_store,at(local.box_transform,c*sizeof(Lanes)+4u),unit);'),
    (SYSTEMS, 'query-zero-lane', 'for(const auto address:{column(0,3),column(1,3),column(2,3),staged(3)})',
     'for(const auto address:{column(0,3),column(1,3),column(2,3),column(0,3)})'),
    (SYSTEMS, 'query-aabb-reload', '        aabb=memory.load32(aabb_reload,local.aabb_argument);\n', ''),
    (SYSTEMS, 'query-walk-empty', 'if(std::int32_t(memory.load32(walk_size,size_address))<=0)return;',
     'if(std::int32_t(memory.load32(walk_size,size_address))<0)return;'),
    (SYSTEMS, 'query-transform-args', 'frame.relative_transform(local.query_transform,local.instance_transform,local.box_transform);',
     'frame.relative_transform(local.query_transform,local.box_transform,local.instance_transform);'),
    (SYSTEMS, 'query-index-reload', '|memory.load32(instance_index_reload,local.instance_index);', '|memory.load32(instance_index_reload,local.resume_index);'),
    # adapter
    (HEADER, 'query-save-order', 'port.push(0x1f6d24u,reg(edi));port.push(0x1f6d25u,reg(esi));reg(edi)=reg(ecx);port.push(0x1f6d28u,reg(ebx));',
     'port.push(0x1f6d24u,reg(edi));port.push(0x1f6d25u,reg(ebx));reg(edi)=reg(ecx);port.push(0x1f6d28u,reg(esi));'),
    (HEADER, 'query-kind-ecx', 'reg(eax)=flags.logic(reg(ecx)&0xffffu,4);reg(ecx)=flags.logic(reg(ecx)&0xe0000000u,4);',
     'reg(eax)=flags.logic(reg(ecx)&0xffffu,4);reg(ecx)=flags.logic(reg(ecx)&0xc0000000u,4);'),
    (HEADER, 'query-resume-edi', 'reg(edx)=t+0xc0u;reg(edi)=flags.increment(reg(edi),4,true);port.push(0x1f6f19u,reg(edx));',
     'reg(edx)=t+0xc0u;reg(edi)=flags.increment(reg(edi),4,false);port.push(0x1f6f19u,reg(edx));'),
    (HEADER, 'query-shape-flags', 'reg(ecx)=flags.plus(reg(ecx),logic::structure_aabb_query::instance_query_shape,4);',
     'reg(ecx)=reg(ecx)+logic::structure_aabb_query::instance_query_shape;'),
    (HEADER, 'query-walk-flags', 'reg(edi)=flags.increment(reg(edi),4,false);flags.minus(reg(edi),value,4);break;',
     'reg(edi)=flags.increment(reg(edi),4,false);flags.minus(value,reg(edi),4);break;'),
    (HEADER, 'query-rejected-ebx', '            if(!kept)reg(ebx)=flags.increment(reg(ebx),4,true);\n', ''),
    (HEADER, 'query-walk-edi', '// XOR EDI,EDI; TEST EAX,EAX\n            reg(eax)=value;reg(edi)=flags.logic(0u,4);flags.logic(value,4);',
     '// XOR EDI,EDI; TEST EAX,EAX\n            reg(eax)=value;flags.logic(value,4);'),
    (HEADER, 'query-exit-pops', 'gp[edi]=port.pop(0x1f6f92u);gp[esi]=port.pop(0x1f6f93u);', 'gp[esi]=port.pop(0x1f6f92u);gp[edi]=port.pop(0x1f6f93u);'),
    (HEADER, 'query-ret', 'port.ret(0x1f6f98u,8u);port.publish(0x1f6f98u);', 'port.ret(0x1f6f98u,4u);port.publish(0x1f6f98u);'),
    (HEADER, 'query-structure-return', 'port.push(0x1f6d2du,0x1f6d32u);', 'port.push(0x1f6d2du,0x1f6d33u);'),
    (HEADER, 'query-site', 'aabb_query_filter_target=0x1f6fa0u,\n    aabb_query_site=0x1f262eu;', 'aabb_query_filter_target=0x1f6fa0u,\n    aabb_query_site=0x1f262fu;'),
    (HEADER, 'query-push-order', 'port.push(0x1f6f00u,reg(esi));port.push(0x1f6f01u,logic::structure_aabb_query::query_tolerance);',
     'port.push(0x1f6f00u,logic::structure_aabb_query::query_tolerance);port.push(0x1f6f01u,reg(esi));'),
    (HEADER, 'query-filter-eax', 'INC EBX; CMP EBX,EAX\n            reg(eax)=value;\n', 'INC EBX; CMP EBX,EAX\n'),
    (HEADER, 'query-accepted-edx', '        case accepted_data:reg(edx)=value;break;\n', ''),
    (HEADER, 'query-frame-size', 'gp[esp]=flags.minus(gp[esp],0xf4u,4);        // SUB ESP,F4\n    AabbQueryAbi',
     'gp[esp]=flags.minus(gp[esp],0xf0u,4);        // SUB ESP,F4\n    AabbQueryAbi'),
    (HEADER, 'query-matrix-pc', "case 0x10u:return 0x1f6da1u;case 0x04u:return 0x1f6da6u;",
     "case 0x10u:return 0x1f6da6u;case 0x04u:return 0x1f6da1u;"),
    (HEADER, 'query-filter-edx', '        reg(edx)=reg(edi);\n        if(reg(edi)!=key)throw', '        if(reg(edi)!=key)throw'),
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
    """Write the harness inputs (bodies, reference, selected() cases)."""
    if not module.admitted(text):
        raise ValueError('Canonical 1F6D10 body, operands or children drifted from the pins')
    # The unit's sign_extend() (MOVSX), which the translated body calls.
    extend = re.search(r'std::uint32_t sign_extend\(std::uint32_t x,unsigned n\) \{.*?\n\}', text, re.S)[0]
    names = module.INLINE + (module.NAME,)
    (directory / 'query_bodies.inc').write_text(
        extend + '\n' + ''.join(f'static void run_{name}(ObservedWorldState &);\n' for name in names) +
        ''.join(module.ordered(module.body(text, name)[0]) + '\n' for name in names))
    (directory / 'query_reference.inc').write_text(module.reference(module.body(text, module.NAME)))
    (directory / 'query_selected.inc').write_text(''.join(f'case 0x{a:x}u:return 0x{b:x}u;\n' for a, b in sorted(module.OPERANDS.items())))


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
    module = load(HELPER, 'native_structure_aabb_query_replacements')
    generate(module, text, scratch)
    # Production binding: installs, idempotent, refuses a changed body, a
    # changed operand mapping or a changed dispatch site.
    output, installed = module.install(text, text)
    if installed != [module.ENTRY] or module.prefix() not in output or module.REFERENCE not in output:
        raise ValueError(f'1F6D10 binding installed {installed}')
    if module.install(text, output)[1]:
        raise ValueError('1F6D10 binding is not idempotent')
    found = module.body(text, module.NAME)
    changed = text.replace(found[0], found[0].replace('0xffffu', '0xfffeu', 1), 1)
    if changed == text or module.install(changed, changed)[1]:
        raise ValueError('1F6D10 binding accepted a changed body')
    operand = sorted(module.OPERANDS)[0]
    remapped = text.replace(f'case 0x{operand:x}u:return 0x{module.OPERANDS[operand]:x}u;',
                            f'case 0x{operand:x}u:return 0x{module.OPERANDS[operand] + 4:x}u;', 1)
    if remapped == text or module.install(remapped, remapped)[1]:
        raise ValueError('1F6D10 binding accepted a changed operand mapping')
    site = module.SITES[0]
    moved = text.replace(found[0], found[0].replace(f'0x{site:x}u,false)', f'0x{site + 1:x}u,false)', 1), 1)
    if moved == text or module.install(moved, moved)[1]:
        raise ValueError('1F6D10 binding accepted a changed dispatch site')
    report = dict(passed=False, unit=str(UNIT.relative_to(ROOT)), pin=module.PIN)
    release = compile_harness(scratch, 'query-release', [])
    result = run(release, args.cases)
    census = run(release, max(args.cases // 4, 1000), 'census')
    verify = run(release, max(args.cases // 4, 1000), 'verify')
    # The phantom/world composition's canonical-PC remap (release, census, verify).
    report['remap'] = remap_sites.run_modes(run, release, max(args.cases // 4, 1000))
    header = HEADER.read_text()
    # The bits after the earlier word-1 owners' in coverage word 1.
    offset = 64 + sum(len(enum_names(header, name)) - 1 for name in ('ClosestPointBranch', 'SweptPairBranch', 'ChildShapeBranch',))
    branches = [n for n in enum_names(header, 'AabbQueryBranch') if n != 'aabb_query_branch_end']
    coverage = (result['coverage'] | result['coverage_high'] << 64) | (verify['coverage'] | verify['coverage_high'] << 64)
    hit = {name for index, name in enumerate(branches) if coverage >> (offset + index) & 1}
    missing = sorted(set(branches) - hit)
    report.update(release=result, census=census, verify=verify, branches=branches, covered=sorted(hit), missing=missing)
    if missing:
        raise ValueError(f'Uncovered 1F6D10 branches: {missing}')
    if census['compared'] == 0 or verify['compared'] == 0 or result['child_calls'] == 0:
        raise ValueError('Census, verify or the children never ran')
    if not args.skip_sanitizers:
        sanitized = compile_harness(scratch, 'query-sanitized', [], sanitizers=True)
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
    print(f"PASS {result['compared']} native 1F6D10 comparisons ({result['ordered_stores']} ordered stores, "
          f"{result['faults']} faults), census {census['compared']}, verify {verify['compared']}, "
          f"{len(mutants)} mutants rejected")
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
