#!/usr/bin/env python3
"""Prove the native TU1.5 manifold owners (the children of 304350): the logic
in native_tu15_world_leaves_systems.hpp (manifold_report::verify_and_report,
30CF40; contact_weld::weld, 304130) through its boundary adapters in
native_tu15_world_leaves.hpp (kernel_30cf40, kernel_304130).

Run through tools/run_test.py. Extracts the unmodified canonical translated
bodies (run_309450, run_300650) from the checked-in physics world unit (pinned by
tools/native_manifold_replacements.py), their selected() operand cases and the
binder's verification references, and compiles
tests/native_tu15_manifold_tests.cpp against them. Compares every seeded case
(ordered store events with preimages, bank, registers, flags, XMM, x87, MXCSR,
cursor; faults by exception, cursor and bank) in Release, census and verify
(record/replay with every child boundary) modes and an ASan/UBSan build, and
requires every branch. Checks the production binding (install, idempotence,
refusal of a changed body, operand or dispatch site). Then screens deliberate
defects in the logic and the adapter: each mutant is an out-of-tree copy of
one header placed first on the include path and must be rejected in release
or verify mode. Source hashes are rechecked after the screen.
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
TEST = ROOT / 'tests/native_tu15_manifold_tests.cpp'
HELPER = ROOT / 'tools/native_manifold_replacements.py'
MUTANTS = [
    # 30CF40 logic
    (SYSTEMS, 'report-tolerance-relation',
     'const bool within=distance<x87::load(memory.load_float(plane_tolerance,',
     'const bool within=distance<=x87::load(memory.load_float(plane_tolerance,'),
    (SYSTEMS, 'report-separation-relation', 'if(!(again>x87::load(memory.load_float(separation_argument,',
     'if(!(again>=x87::load(memory.load_float(separation_argument,'),
    (SYSTEMS, 'report-drift-relation', 'if(!(squared<x87::load(memory.load_float(drift_squared_load,',
     'if(!(squared<=x87::load(memory.load_float(drift_squared_load,'),
    (SYSTEMS, 'report-drift-square', 'x87::store(x87::multiply(drift,drift))', 'x87::store(drift)'),
    (SYSTEMS, 'report-world-sum-order', 'return sse::add(sse::add(x_term,y_term),z_and_translation);',
     'return sse::add(sse::add(y_term,x_term),z_and_translation);'),
    (SYSTEMS, 'report-world-translation', 'const auto z_and_translation=sse::add(sse::multiply(t.column[2],splat(point,2)),t.translation);',
     'const auto z_and_translation=sse::add(t.translation,sse::multiply(t.column[2],splat(point,2)));'),
    (SYSTEMS, 'report-horizontal-order', 'const auto yx=sse::add(std::bit_cast<float>(l[1]),std::bit_cast<float>(l[0]));',
     'const auto yx=sse::add(std::bit_cast<float>(l[0]),std::bit_cast<float>(l[1]));'),
    (SYSTEMS, 'report-projection-sign', 'const auto offset=sse::subtract(c.world_a,sse::add(c.world_b,along));',
     'const auto offset=sse::subtract(c.world_a,sse::subtract(c.world_b,along));'),
    (SYSTEMS, 'report-capacity-mask', '&contact_array::capacity_mask;\n    const auto size=memory.load32(output_size',
     ';\n    const auto size=memory.load32(output_size'),
    (SYSTEMS, 'report-growth-test', 'if(std::int32_t(capacity)<std::int32_t(needed)) {', 'if(std::int32_t(capacity)<=std::int32_t(needed)) {'),
    (SYSTEMS, 'report-growth-size', 'const auto doubled=capacity*2u;', 'const auto doubled=capacity*2u+1u;'),
    (SYSTEMS, 'report-weight-sides', 'sse::subtract(memory.load_vector(one,globals::one),weights),c.world_b),r.weighted_a);',
     'sse::subtract(memory.load_vector(one,globals::one),weights),c.world_a),r.weighted_a);'),
    (SYSTEMS, 'report-distance-out', 'memory.store32(distance_out_store,at(out,distance_offset),distance);',
     'memory.store32(distance_out_store,at(out,distance_offset),w);'),
    (SYSTEMS, 'report-kept-count', 'memory.store32(kept_store,local.kept,memory.load32(kept_load,local.kept)+1u);',
     'memory.store32(kept_store,local.kept,memory.load32(kept_load,local.kept)+2u);'),
    (SYSTEMS, 'report-moved-point', 'const auto count=memory.load32(removed_count,count_address)-1u;',
     'const auto count=memory.load32(removed_count,count_address)-2u;'),
    (SYSTEMS, 'report-output-shrink', 'memory.store32(output_size_decrement,size_address,memory.load32(output_size_load,size_address)-1u);',
     'memory.store32(output_size_decrement,size_address,memory.load32(output_size_load,size_address));'),
    (SYSTEMS, 'report-output-stride', 'out+=std::uint32_t(sizeof(OutputPoint));', 'out+=0x20u;'),
    (SYSTEMS, 'report-count-test', 'if(std::int32_t(memory.load32(point_count_again,count_address))<=0)return;',
     'if(std::int32_t(memory.load32(point_count_again,count_address))<0)return;'),
    # 30CF40 adapter
    (HEADER, 'report-reserve-pushes', 'port.push(0x30cf90u,reg(eax));port.push(0x30cf91u,reg(ecx));',
     'port.push(0x30cf90u,reg(ecx));port.push(0x30cf91u,reg(eax));'),
    (HEADER, 'report-separation-compare', 'case separation_argument:fcomp(value,0x41u);break;', 'case separation_argument:break;'),
    (HEADER, 'report-xmm5-lanes', 'port.set_xmm(5,vector({std::bit_cast<std::uint32_t>(check.distance),separation[2],separation[2],separation[2]}));',
     'port.set_xmm(5,vector({std::bit_cast<std::uint32_t>(check.distance),separation[1],separation[2],separation[2]}));'),
    (HEADER, 'report-fld-cursor', 'case output_capacity:x87_at(0x30cf57u);', 'case output_capacity:x87_at(0x30cf59u);'),
    (HEADER, 'report-remove-flags', 'reg(edx)=flags.logic(0u,4);                  // XOR EDX,EDX', 'reg(edx)=0u;'),
    (HEADER, 'report-next-point', 'reg(edx)=flags.increment(value,4,false);reg(esi)=flags.plus(reg(esi),0x30u,4);',
     'reg(edx)=flags.increment(value,4,false);reg(esi)=flags.plus(reg(esi),0x20u,4);'),
    (HEADER, 'report-reserve-doubling', 'reg(eax)=port.flags.plus(reg(eax),reg(eax),4);', 'reg(eax)=port.flags.plus(reg(eax),reg(esi),4);'),
    (HEADER, 'report-removed-edx', 'case removed_points:reg(edx)=value;break;', 'case removed_points:break;'),
    (HEADER, 'report-loop-flags', 'case count_reload:flags.minus(reg(ecx),value,4);break;', 'case count_reload:flags.minus(value,reg(ecx),4);break;'),
    (HEADER, 'report-kept-normal', 'port.set_xmm(0,kept_normal_value);port.set_xmm(1,r.position);',
     'port.set_xmm(1,r.position);'),
    (HEADER, 'report-output-register', 'case output_size_store:reg(ebx)=flags.shift(reg(ebx)*3u,4u,4);break;',
     'case output_size_store:reg(ebx)=flags.shift(reg(ebx)*3u,5u,4);break;'),
    (HEADER, 'report-exit-pops', 'gp[edi]=port.pop(0x30d18au);gp[esi]=port.pop(0x30d18bu);',
     'gp[esi]=port.pop(0x30d18au);gp[edi]=port.pop(0x30d18bu);'),
    # 304130 logic
    (SYSTEMS, 'weld-state-test', 'at(record,offsetof(ContactRecord,state)))==tested_state) {',
     'at(record,offsetof(ContactRecord,state)))>=tested_state) {'),
    (SYSTEMS, 'weld-verdict-address', 'if(memory.load8(verdict,verdict_at)==0) {', 'if(memory.load8(verdict,verdict_at+1u)==0) {'),
    (SYSTEMS, 'weld-end-shrink', 'end_offset-=std::uint32_t(sizeof(ContactRecord));', 'end_offset-=0u;'),
    (SYSTEMS, 'weld-record-stride', 'record_offset+=std::uint32_t(sizeof(ContactRecord));', 'record_offset+=0x40u;'),
    (SYSTEMS, 'weld-x87-order', 'memory.store_float(written_word_20,memory.load32(at_word_20_load,local.at_word_20),x87::store(word_20));',
     'memory.store_float(written_word_20,memory.load32(at_word_20_load,local.at_word_20),x87::store(word_24));'),
    (SYSTEMS, 'weld-second-vectors', 'memory.store_vector(written_vector_60,field(last,offsetof(ContactRecord,second_contact.separating_normal)),held.second_normal);',
     'memory.store_vector(written_vector_60,field(last,offsetof(ContactRecord,second_contact.separating_normal)),held.second_position);'),
    (SYSTEMS, 'weld-held-state', 'memory.store8(held_state_store,local.held_state,state);', 'memory.store8(held_state_store,local.held_state,byte_7d);'),
    (SYSTEMS, 'weld-count', 'held.count=memory.load32(count_swap,local.count)-1u;', 'held.count=memory.load32(count_swap,local.count)-2u;'),
    (SYSTEMS, 'weld-spill-order', 'memory.store32(keep,slot,field(last,offset));\n        memory.store32(to,field(record,offset),memory.load32(from,field(last,offset)));',
     'memory.store32(to,field(record,offset),memory.load32(from,field(last,offset)));\n        memory.store32(keep,slot,field(last,offset));'),
    (SYSTEMS, 'weld-last-record', 'swap_with_last(memory,local,record,data+end_offset-std::uint32_t(sizeof(ContactRecord)));',
     'swap_with_last(memory,local,record,data+end_offset-2u*std::uint32_t(sizeof(ContactRecord)));'),
    (SYSTEMS, 'weld-index-step', 'memory.store32(index_store,local.index,memory.load32(index_load,local.index)+1u);',
     'memory.store32(index_store,local.index,memory.load32(index_load,local.index)+2u);'),
    (SYSTEMS, 'weld-first-test', 'if(!(std::int32_t(first)<std::int32_t(count)))return count;', 'if(!(std::int32_t(first)<=std::int32_t(count)))return count;'),
    (SYSTEMS, 'weld-written-position', 'memory.store_vector(written_position,field(last,offsetof(ContactRecord,contact.position)),held.position);',
     'memory.store_vector(written_position,field(last,offsetof(ContactRecord,contact.position)),held.normal);'),
    # 304130 adapter
    (HEADER, 'weld-push-order', 'port.push(0x30417bu,reg(eax));port.push(0x30417cu,reg(ecx));',
     'port.push(0x30417bu,reg(ecx));port.push(0x30417cu,reg(eax));'),
    (HEADER, 'weld-verdict-register', 'reg(edx)=verdict_address;port.push(0x304185u,reg(edx));', 'reg(edx)=verdict_address+1u;port.push(0x304185u,reg(edx));'),
    (HEADER, 'weld-xmm6', 'port.set_xmm(6,held.last_second_normal);', 'port.set_xmm(6,held.second_normal);'),
    (HEADER, 'weld-offset-shift', 'reg(edi)=flags.shift(reg(edi),7u,4);reg(ebx)=flags.shift(reg(ebx),7u,4);',
     'reg(edi)=flags.shift(reg(edi),6u,4);reg(ebx)=flags.shift(reg(ebx),7u,4);'),
    (HEADER, 'weld-loop-flags', 'case index_compare:flags.minus(value,reg(eax),4);break;', 'case index_compare:flags.minus(reg(eax),value,4);break;'),
    (HEADER, 'weld-count-register', 'if(a==at_byte_7d_load)reg(ecx)=flags.increment(reg(ecx),4,true);',
     'if(a==at_byte_7d_load)reg(ecx)=flags.increment(reg(ecx),4,false);'),
    (HEADER, 'weld-7d-byte', 'case held_byte_7d_load:byte_register(edx,value);break;', 'case held_byte_7d_load:byte_register(eax,value);break;'),
    (HEADER, 'weld-ret', 'port.ret(0x304341u,0x14u);', 'port.ret(0x304341u,0x10u);'),
    (HEADER, 'weld-exit-edx', 'case at_word_24_load:case at_vector_40_load:case held_word_74_load:case at_word_78_load:reg(edx)=value;break;',
     'case at_word_24_load:case at_vector_40_load:case held_word_74_load:reg(edx)=value;break;'),
    (HEADER, 'weld-x87-push', 'case held_word_20:case held_word_24:port.s.fp.push(', 'case held_word_20:port.s.fp.push('),
    (HEADER, 'report-fstp-st0', 'case point_count:port.s.require_x87();port.s.fp.st(0)=port.s.fp.st(0);port.s.fp.pop();break;',
     'case point_count:port.s.require_x87();break;'),
]


REMAP = Path(__file__).resolve().with_name('native_world_leaves_remap_sites.py')


def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def load(path, name):
    spec = importlib.util.spec_from_file_location(name, path)
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
    module = load(HELPER, 'native_manifold_replacements')
    names = list(module.OWNERS)
    for name in names:
        if not module.admitted(text, name):
            raise ValueError(f'Canonical {name} body, operands or children drifted from the pins')
    (scratch / 'manifold_bodies.inc').write_text(
        ''.join(f'static void run_{name}(ObservedWorldState &);\n' for name in names) +
        ''.join(module.ordered(module.body(text, name)[0]) + '\n' for name in names))
    (scratch / 'manifold_reference.inc').write_text(''.join(module.reference(module.body(text, name), name) for name in names))
    selected = {}
    for name in names:
        selected.update(module.OWNERS[name][2])
    (scratch / 'manifold_selected.inc').write_text(''.join(f'case 0x{a:x}u:return 0x{b:x}u;\n' for a, b in sorted(selected.items())))
    # Production binding: installs, idempotent, refuses a changed body, a
    # changed operand mapping or a changed dispatch site.
    output, installed = module.install(text, text)
    if installed != [module.OWNERS[name][0] for name in names]:
        raise ValueError(f'Manifold binding installed {installed}')
    for name in names:
        if module.prefix(name) not in output or module.reference_name(name) not in output:
            raise ValueError(f'{name} binding did not install')
    if module.install(text, output)[1]:
        raise ValueError('Manifold binding is not idempotent')
    for name in names:
        found = module.body(text, name)
        changed = text.replace(found[0], found[0].replace('0xfffffff0u', '0xfffffff8u', 1), 1)
        if changed == text or module.OWNERS[name][0] in module.install(changed, changed)[1]:
            raise ValueError(f'{name} binding accepted a changed body')
        operand = sorted(module.OWNERS[name][2])[0]
        target = module.OWNERS[name][2][operand]
        remapped = text.replace(f'case 0x{operand:x}u:return 0x{target:x}u;', f'case 0x{operand:x}u:return 0x{target + 4:x}u;', 1)
        if remapped == text or module.OWNERS[name][0] in module.install(remapped, remapped)[1]:
            raise ValueError(f'{name} binding accepted a changed operand mapping')
        site = module.OWNERS[name][3][0]
        moved = text.replace(found[0], found[0].replace(f'0x{site:x}u,false)', f'0x{site + 1:x}u,false)', 1), 1)
        if moved == text or module.OWNERS[name][0] in module.install(moved, moved)[1]:
            raise ValueError(f'{name} binding accepted a changed dispatch site')
    report = dict(passed=False, unit=str(UNIT.relative_to(ROOT)), pins={name: module.OWNERS[name][1] for name in names})
    release = compile_harness(scratch, 'manifold-release', [])
    result = run(release, args.cases)
    census = run(release, max(args.cases // 4, 1000), 'census')
    verify = run(release, max(args.cases // 4, 1000), 'verify')
    # The phantom/world composition's canonical-PC remap (release, census, verify).
    report['remap'] = remap_sites.run_modes(run, release, max(args.cases // 4, 1000))
    header = HEADER.read_text()
    offset = (len(enum_names(header, 'Branch')) - 1 + len(enum_names(header, 'ArrayBranch')) - 1 +
              len(enum_names(header, 'LocalBufferBranch')) - 1 + len(enum_names(header, 'CollectionAgentBranch')) - 1)
    branches = [n for n in enum_names(header, 'ReportBranch') + enum_names(header, 'WeldBranch')
                if n not in ('report_branch_end', 'weld_branch_end')]
    coverage = result['coverage'] | verify['coverage']
    hit = {name for index, name in enumerate(branches) if coverage >> (offset + index) & 1}
    missing = sorted(set(branches) - hit)
    report.update(release=result, census=census, verify=verify, branches=branches, covered=sorted(hit), missing=missing)
    if missing:
        raise ValueError(f'Uncovered manifold branches: {missing}')
    if census['compared'] == 0 or verify['compared'] == 0 or result['report']['child_calls'] == 0 or \
            result['weld']['child_calls'] == 0:
        raise ValueError('Census, verify or the children never ran')
    if not args.skip_sanitizers:
        sanitized = compile_harness(scratch, 'manifold-sanitized', [], sanitizers=True)
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
    print(f"PASS {result['compared']} native manifold comparisons ({result['ordered_stores']} ordered stores, "
          f"{result['faults']} faults), census {census['compared']}, verify {verify['compared']}, "
          f"{len(mutants)} mutants rejected")
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
