#!/usr/bin/env python3
"""Prove the native TU1.5 convex pair owners: the logic in
native_tu15_world_leaves_systems.hpp (closest_point::find, 2F8630, with
havok_rotation::rotate, 2E4760, inline; swept_pair::closest_point, 2E4FE0)
through its boundary adapters in native_tu15_world_leaves.hpp (kernel_2f8630,
kernel_2e4760, kernel_2e4fe0).

Run through tools/run_test.py. Extracts the unmodified canonical translated
bodies (run_2f4bd0, run_2e14e0; pinned by
tools/native_convex_pair_replacements.py), the translated 2E4550 and 2E4760
the references run inline, their selected() operand cases and the binder's
verification references, and compiles tests/native_tu15_convex_pair_tests.cpp
against them. In 2E4FE0's copy its 2F8630 call goes to a stand-in, as its
other children do. Compares every seeded case (ordered store events with
preimages, bank, registers, flags, XMM, x87, MXCSR, cursor; faults by
exception, cursor and bank) in Release, census and verify (record/replay with
every child boundary) modes and an ASan/UBSan build, and requires every
branch. Checks the production binding (install, idempotence, refusal of a
changed body, operand or dispatch site). Then screens deliberate defects in
the logic and the adapter: each mutant is an out-of-tree copy of one header
placed first on the include path and must be rejected in release or verify
mode. Source hashes are rechecked after the screen.
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
TEST = ROOT / 'tests/native_tu15_convex_pair_tests.cpp'
HELPER = ROOT / 'tools/native_convex_pair_replacements.py'
# The coverage bits of these owners start in word 1 (filter_branch_end).
COVERAGE_OFFSET = 64
MUTANTS = [
    # 2F8630 logic
    (SYSTEMS, 'closest-plane-trust', 'const bool plane_trusted=x87::greater(distance,', 'const bool plane_trusted=!x87::less(distance,'),
    (SYSTEMS, 'closest-closing-order', 'check.closing_terms=sse::multiply(sse::subtract(check.change_b,change_a),normal);',
     'check.closing_terms=sse::multiply(normal,sse::subtract(check.change_b,change_a));'),
    (SYSTEMS, 'closest-change-a', 'const auto change_a=sse::subtract(a_end,a_start);', 'const auto change_a=sse::subtract(a_start,a_end);'),
    (SYSTEMS, 'closest-bound-sum', 'closed=x87::add(closed,memory.load_float(a_bound,', 'closed=x87::subtract(closed,memory.load_float(a_bound,'),
    (SYSTEMS, 'closest-reverse-subtract', 'volatile Extended difference=operand-b;return difference;}  // FSUBR m32',
     'volatile Extended difference=b-operand;return difference;}  // FSUBR m32'),
    (SYSTEMS, 'closest-fst-compare', 'return x87::greater(reduced,memory.load_float(plane_bound_again,',
     'return x87::greater(x87::load(x87::store(reduced)),memory.load_float(plane_bound_again,'),
    (SYSTEMS, 'closest-gap-rounding', 'if(x87::less(rounded,memory.load_float(tolerance,', 'if(x87::less(gap,memory.load_float(tolerance,'),
    (SYSTEMS, 'closest-radius-sign', 'memory.store_float(negated_radius_store,local.negated,x87::store(x87::negate(radius)));',
     'memory.store_float(negated_radius_store,local.negated,x87::store(radius));'),
    (SYSTEMS, 'closest-point-a-order', 'sse::add(point,sse::multiply(radius_step,normal)));', 'sse::add(sse::multiply(radius_step,normal),point));'),
    (SYSTEMS, 'closest-translation', 'const auto in_a=sse::subtract(sse::add(on_surface,sse::multiply(gap_step,normal_again)),translation);',
     'const auto in_a=sse::add(on_surface,sse::multiply(gap_step,normal_again));'),
    (SYSTEMS, 'closest-cache-fraction', 'given_up=x87::multiply(given_up,memory.load_float(cache_fraction,',
     'given_up=x87::add(given_up,memory.load_float(cache_fraction,'),
    (SYSTEMS, 'closest-cache-flag', 'if(memory.load32(plane_not_cached,', 'if(!memory.load32(plane_not_cached,'),
    (SYSTEMS, 'closest-contact-result', 'memory.store8(contact_result,result,1u);', 'memory.store8(contact_result,result,2u);'),
    (SYSTEMS, 'closest-timer-advance', 'memory.store32(a.advance,collection_agent::globals::monitor_next,next+12u);',
     'memory.store32(a.advance,collection_agent::globals::monitor_next,next+16u);'),
    (SYSTEMS, 'closest-timer-room', 'const auto end=memory.load32(a.end,collection_agent::globals::monitor_end);\n    if(next>=end)return;',
     'const auto end=memory.load32(a.end,collection_agent::globals::monitor_end);\n    if(next>end)return;'),
    (SYSTEMS, 'closest-cached-normal', 'memory.store_vector(plane_normal_store,at(agent,offsetof(GskAgent,separating_plane)),normal);',
     'memory.store_vector(plane_normal_store,at(agent,offsetof(GskAgent,separating_plane)),sse::add(normal,normal));'),
    # 2E4760 logic (inline)
    (SYSTEMS, 'forward-sum-order', 'return {sse::add(sse::add(x_term,y_term),z_term),splat(v,0),y_term,z_term};',
     'return {sse::add(x_term,sse::add(y_term,z_term)),splat(v,0),y_term,z_term};'),
    (SYSTEMS, 'forward-product-order', 'const auto z_term=sse::multiply(rotation.column[2],splat(v,2));',
     'const auto z_term=sse::multiply(splat(v,2),rotation.column[2]);'),
    # 2F8630 adapter
    (HEADER, 'closest-save-order', 'port.push(0x2f8642u,reg(esi));port.push(0x2f8643u,reg(edi));',
     'port.push(0x2f8642u,reg(edi));port.push(0x2f8643u,reg(esi));'),
    (HEADER, 'closest-plane-compare', 'case plane_bound:x87.compare_pop(value);break;', 'case plane_bound:break;'),
    (HEADER, 'closest-tolerance-status', 'case tolerance:x87.compare_pop(value);x87.test_status(5u);break;',
     'case tolerance:x87.compare_pop(value);x87.test_status(4u);break;'),
    (HEADER, 'closest-closing-slot', '.time_stamp=t+0x14u,.closing=t+0x14u,', '.time_stamp=t+0x14u,.closing=t+0x18u,'),
    (HEADER, 'closest-features-pushes', 'port.push(0x2f8754u,reg(eax));port.push(0x2f8755u,reg(ecx));',
     'port.push(0x2f8754u,reg(ecx));port.push(0x2f8755u,reg(eax));'),
    (HEADER, 'closest-features-test', 'port.flags.logic(reg(eax),4);\n        if(reg(eax))port.reached(p_features_apart);',
     'if(reg(eax))port.reached(p_features_apart);'),
    (HEADER, 'closest-plane-xmm3', 'port.set_xmm(3,r.plane.change_b);', ''),
    (HEADER, 'closest-contact-xmm2', 'port.set_xmm(2,vector({c[0],c[1],c[0],c[1]}));', 'port.set_xmm(2,vector({c[0],c[1],c[2],c[3]}));'),
    (HEADER, 'closest-rdtsc-cursor', 'port.at(search_timer?0x2f8711u:0x2f867bu);', 'port.at(search_timer?0x2f867bu:0x2f8711u);'),
    (HEADER, 'closest-exit-pops', 'gp[edi]=port.pop(0x2f8885u);gp[esi]=port.pop(0x2f8886u);', 'gp[esi]=port.pop(0x2f8885u);gp[edi]=port.pop(0x2f8886u);'),
    (HEADER, 'closest-no-contact-pops', 'case no_contact_result:reg(edi)=port.pop(0x2f88bbu);reg(esi)=port.pop(0x2f88bcu);break;',
     'case no_contact_result:reg(esi)=port.pop(0x2f88bbu);reg(edi)=port.pop(0x2f88bcu);break;'),
    (HEADER, 'closest-ret', 'port.ret(exit,0x14u);', 'port.ret(exit,0x10u);'),
    (HEADER, 'closest-fst-pop', 'case gap_store:case negated_radius_store:case negated_gap_store:case cache_distance_store:x87.pop();break;',
     'case plane_distance_store:case gap_store:case negated_radius_store:case negated_gap_store:case cache_distance_store:x87.pop();break;'),
    (HEADER, 'closest-stream-advance', 'case plane_advance:case search_advance:reg(esi)=flags.plus(reg(esi),0xcu,4);break;',
     'case plane_advance:case search_advance:reg(esi)=flags.plus(reg(esi),0x10u,4);break;'),
    (HEADER, 'closest-gap-depth', 'case plane_distance:case b_bound:case core_distance:case gap_load:case contact_radius:case contact_gap:',
     'case plane_distance:case b_bound:case core_distance:case gap_load:case contact_radius:'),
    (HEADER, 'forward-vector-slot', 'std::uint32_t vector(){return port.read(0x2e4763u,ebp+0xcu,4);}',
     'std::uint32_t vector(){return port.read(0x2e4763u,ebp+8u,4);}'),
    (HEADER, 'forward-xmm1', 'port.set_xmm(1,rotated.x_splat);', 'port.set_xmm(1,rotated.value);'),
    # 2E4FE0 logic
    (SYSTEMS, 'swept-type', '==convex_pair::swept_shape_type)\n        sweep_a(', '!=convex_pair::swept_shape_type)\n        sweep_a('),
    (SYSTEMS, 'swept-speed-compare', 'const bool fast=x87::greater(squared,memory.load_float(a_threshold,',
     'const bool fast=!x87::less(squared,memory.load_float(a_threshold,'),
    (SYSTEMS, 'swept-speed-square', 'speed.squared=sse::multiply(velocity,velocity);', 'speed.squared=sse::add(velocity,velocity);'),
    (SYSTEMS, 'swept-scale-order', 'sweep.scaled=sse::multiply(manifold_report::broadcast(sweep.step_time),sweep.loaded);',
     'sweep.scaled=sse::multiply(sweep.loaded,manifold_report::broadcast(sweep.step_time));'),
    (SYSTEMS, 'swept-flag-a', 'memory.store32(a_flags_store,local.sweep_flags,swept_a);', 'memory.store32(a_flags_store,local.sweep_flags,swept_b);'),
    (SYSTEMS, 'swept-flag-b', 'const auto flags=memory.load32(b_flags_load,local.sweep_flags)+swept_b;',
     'const auto flags=memory.load32(b_flags_load,local.sweep_flags)+swept_a;'),
    (SYSTEMS, 'swept-changed', 'if(flags!=memory.load32(previous_flags,', 'if(flags==memory.load32(previous_flags,'),
    (SYSTEMS, 'swept-reset-swept', '} else reset=memory.load8(swept_load,local.swept)!=0;', '} else reset=memory.load8(swept_load,local.swept)==0;'),
    (SYSTEMS, 'swept-no-plane', 'inline constexpr std::uint32_t no_plane=0xbf800000u;', 'inline constexpr std::uint32_t no_plane=0x3f800000u;'),
    (SYSTEMS, 'swept-cache-type', '==convex_pair::cache_type_5) {', '!=convex_pair::cache_type_5) {'),
    (SYSTEMS, 'swept-restore', 'memory.store8(settings_byte_restore,settings_again,saved);', 'memory.store8(settings_byte_restore,settings_again,0u);'),
    (SYSTEMS, 'swept-found', 'memory.store8(result_store,result,found);', 'memory.store8(result_store,result,found^1u);'),
    (SYSTEMS, 'swept-parent-walk', 'while(parent){root=parent;parent=memory.load32(next,at(root,offsetof(CdBody,parent)));}',
     'if(parent){root=parent;}'),
    (SYSTEMS, 'swept-zero', 'memory.store_vector(a_sweep_zero,at(shape,offsetof(SweptShape,sweep)),_mm_setzero_ps());',
     'memory.store_vector(a_sweep_zero,at(shape,offsetof(SweptShape,sweep)),speed.squared);'),
    (SYSTEMS, 'swept-owner-keep', 'memory.store32(b_owner_keep,local.owner_b,owner);', 'memory.store32(b_owner_keep,local.owner_b,owner+4u);'),
    (SYSTEMS, 'swept-settings-clear', 'memory.store8(settings_byte_clear,changed,0u);', 'memory.store8(settings_byte_clear,changed,1u);'),
    # 2E4FE0 adapter
    (HEADER, 'swept-save-order', 'port.push(0x2e4ffdu,reg(esi));port.push(0x2e4ffeu,reg(edi));',
     'port.push(0x2e4ffdu,reg(edi));port.push(0x2e4ffeu,reg(esi));'),
    (HEADER, 'swept-type-site', 'const std::uint32_t site=query==shape_a?0x2e1510u:', 'const std::uint32_t site=query==shape_a?0x2e1511u:'),
    (HEADER, 'swept-type-compare', 'port.flags.minus(type,logic::convex_pair::swept_shape_type,4);',
     'port.flags.minus(logic::convex_pair::swept_shape_type,type,4);'),
    (HEADER, 'swept-predicate-test', 'reg(esp)=port.flags.plus(reg(esp),4u,4);\n        port.flags.logic(reg(eax)&0xffu,1);\n        const bool agreed=',
     'reg(esp)=port.flags.plus(reg(esp),4u,4);\n        const bool agreed='),
    (HEADER, 'swept-speed-xmm1', 'port.set_xmm(1,vector({std::bit_cast<std::uint32_t>(speed.yx),squared[1],squared[1],squared[1]}));',
     'port.set_xmm(1,vector({std::bit_cast<std::uint32_t>(speed.yx),squared[0],squared[1],squared[1]}));'),
    (HEADER, 'swept-sweep-xmm0', 'port.set_xmm(0,scalar(sweep.step_time));', 'port.set_xmm(0,sweep.scaled);'),
    (HEADER, 'swept-a-input-flags', 'reg(edx)=value;reg(ecx)=flags.plus(reg(ecx),0x40u,4);reg(eax)=flags.plus(reg(eax),0x20u,4);',
     'reg(edx)=value;reg(eax)=flags.plus(reg(eax),0x20u,4);reg(ecx)=flags.plus(reg(ecx),0x40u,4);'),
    (HEADER, 'swept-b-status', 'case b_owner_keep:x87.test_status(0x41u);break;', 'case b_owner_keep:break;'),
    (HEADER, 'swept-relative-push', 'port.push(0x2e5284u,reg(eax));', 'port.push(0x2e5284u,reg(edx));'),
    (HEADER, 'swept-found-register', 'case found_load:byte_register(ecx,value);break;', 'case found_load:byte_register(edx,value);break;'),
    (HEADER, 'swept-ret', 'port.ret(0x2e5312u,0x14u);', 'port.ret(0x2e5312u,0x10u);'),
    (HEADER, 'swept-esi-pop', 'case result_store:reg(esi)=port.pop(0x2e530bu);break;', 'case result_store:reg(edi)=port.pop(0x2e530bu);break;'),
    (HEADER, 'swept-velocity-eax', 'if(a==a_velocity || a==b_velocity)reg(eax)=port.flags.plus(reg(eax),logic::convex_pair::motion_velocity,4);',
     'if(a==b_velocity)reg(eax)=port.flags.plus(reg(eax),logic::convex_pair::motion_velocity,4);'),
    (HEADER, 'swept-cache-add', 'case cache_5_agent:reg(ecx)=flags.plus(value,0xcu,4);break;', 'case cache_5_agent:reg(ecx)=flags.plus(value,0x10u,4);break;'),
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
    """Write the harness inputs (bodies, references, selected() cases)."""
    names = list(module.OWNERS)
    for name in names:
        if not module.admitted(text, name):
            raise ValueError(f'Canonical {name} body, operands or children drifted from the pins')
    inline = ('2e0a60', '2e0c70')
    bodies = [module.ordered(module.body(text, name)[0]) for name in inline + tuple(names)]
    # 2E4FE0's 2F8630 is a stand-in in its own proof.
    bodies[-1] = bodies[-1].replace('run_2f4bd0(s)', 'stand_in_2f8630(s)')
    (directory / 'pair_bodies.inc').write_text(
        ''.join(f'static void run_{name}(ObservedWorldState &);\n' for name in inline + tuple(names)) +
        ''.join(b + '\n' for b in bodies))
    (directory / 'pair_reference.inc').write_text(''.join(module.reference(module.body(text, name), name) for name in names))
    selected = {}
    for name in names:
        selected.update(module.OWNERS[name][2])
    (directory / 'pair_selected.inc').write_text(''.join(f'case 0x{a:x}u:return 0x{b:x}u;\n' for a, b in sorted(selected.items())))
    return names


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
    module = load(HELPER, 'native_convex_pair_replacements')
    names = generate(module, text, scratch)
    # Production binding: installs, idempotent, refuses a changed body, a
    # changed operand mapping or a changed dispatch site.
    output, installed = module.install(text, text)
    if installed != [module.OWNERS[name][0] for name in names]:
        raise ValueError(f'Convex pair binding installed {installed}')
    for name in names:
        if module.prefix(name) not in output or module.reference_name(name) not in output:
            raise ValueError(f'{name} binding did not install')
    if 'ordered_mul_ps' not in module.body(output, module.FORWARD[0])[0]:
        raise ValueError('2E4760 translated body was not given ordered SSE')
    if module.install(text, output)[1]:
        raise ValueError('Convex pair binding is not idempotent')
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
        if module.OWNERS[name][3]:
            site = module.OWNERS[name][3][0]
            moved = text.replace(found[0], found[0].replace(f'0x{site:x}u,false)', f'0x{site + 1:x}u,false)', 1), 1)
            if moved == text or module.OWNERS[name][0] in module.install(moved, moved)[1]:
                raise ValueError(f'{name} binding accepted a changed dispatch site')
    forward = module.body(text, module.FORWARD[0])
    drifted = text.replace(forward[0], forward[0].replace('0xfffffff0u', '0xfffffff8u', 1), 1)
    if drifted == text or '2f8630' in module.install(drifted, drifted)[1]:
        raise ValueError('2F8630 binding accepted a changed inline 2E4760')
    report = dict(passed=False, unit=str(UNIT.relative_to(ROOT)), pins={name: module.OWNERS[name][1] for name in names},
                  forward_pin=module.FORWARD[1])
    release = compile_harness(scratch, 'pair-release', [])
    result = run(release, args.cases)
    census = run(release, max(args.cases // 4, 1000), 'census')
    verify = run(release, max(args.cases // 4, 1000), 'verify')
    # The phantom/world composition's canonical-PC remap (release, census, verify).
    report['remap'] = remap_sites.run_modes(run, release, max(args.cases // 4, 1000))
    header = HEADER.read_text()
    branches = [n for n in enum_names(header, 'ClosestPointBranch') + enum_names(header, 'SweptPairBranch')
                if n not in ('closest_point_branch_end', 'swept_pair_branch_end')]
    coverage = (result['coverage'] | result['coverage_high'] << 64) | (verify['coverage'] | verify['coverage_high'] << 64)
    hit = {name for index, name in enumerate(branches) if coverage >> (COVERAGE_OFFSET + index) & 1}
    missing = sorted(set(branches) - hit)
    report.update(release=result, census=census, verify=verify, branches=branches, covered=sorted(hit), missing=missing)
    if missing:
        raise ValueError(f'Uncovered convex pair branches: {missing}')
    if census['compared'] == 0 or verify['compared'] == 0 or result['closest']['child_calls'] == 0 or \
            result['swept']['child_calls'] == 0:
        raise ValueError('Census, verify or the children never ran')
    if not args.skip_sanitizers:
        sanitized = compile_harness(scratch, 'pair-sanitized', [], sanitizers=True)
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
    print(f"PASS {result['compared']} native convex pair comparisons ({result['ordered_stores']} ordered stores, "
          f"{result['faults']} faults), census {census['compared']}, verify {verify['compared']}, "
          f"{len(mutants)} mutants rejected")
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
