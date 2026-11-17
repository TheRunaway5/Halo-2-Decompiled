#!/usr/bin/env python3
"""Prove the native TU1.5 cluster distance flood (131470, campaign environment unit).

Run through tools/run_test.py. Extracts the unmodified translated body from
the checked-in unit (pinned) and the verification reference the production
binder generates, compiles tests/native_tu15_cluster_distance_flood_tests.cpp
against them and compares every seeded case (ordered stores with preimages,
14DD70 child log, bank, registers, flags, XMM, MXCSR, cursor; faults by
exception, cursor and store prefix) in Release, census and verify modes and
an ASan/UBSan build. The inline modes run the 14DD70 leaf kernel inside the
closure against the unmodified translated 14DD70 of the scene device unit
(pinned) reached through a modeled required owner round trip, with live x87
stacks, every x87 control and the MXCSR modes. Requires complete branch
coverage, checks the production
binding (install, idempotence, refusal of a changed body or a direct 14DD70
route) and screens deliberate defects in out-of-tree header copies. Source
hashes are rechecked after the screen.
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
UNIT = ROOT / 'src/campaign/source_campaign_environment_tu15_generated.cpp'
HEADER = ROOT / 'src/physics/runtime/native_tu15_cluster_distance_flood.hpp'
SYSTEMS = ROOT / 'src/physics/runtime/native_tu15_cluster_distance_flood_systems.hpp'
TICK_SYSTEMS = ROOT / 'src/physics/runtime/native_tu15_object_tick_systems.hpp'
SHARED = ROOT / 'src/physics/runtime/native_tu15_object_tick.hpp'
TEST = ROOT / 'tests/native_tu15_cluster_distance_flood_tests.cpp'
HELPER = ROOT / 'tools/native_cluster_distance_replacements.py'
LEAVES = ROOT / 'src/physics/runtime/native_tu15_matrix_leaves.hpp'
SEGMENT_UNIT = ROOT / 'src/rendering/renderer/source_scene_device_constructor_tu15_generated.cpp'
SPECIALIZER = ROOT / 'tools/specialize_native_operands.py'
# (name, file, anchor, replacement[, harness mode]); the logic lives in
# SYSTEMS and the ABI in the HEADER adapter. The inline defects run in the
# inline mode (leaf kernel inside the closure against the round trip).
MUTANTS = [
    # Adapter (native_tu15_cluster_distance_flood.hpp)
    ('inline-room', HEADER, 'native_tu15_leaves::x87_room(v,2u)', 'native_tu15_leaves::x87_room(v,1u)', 'inline'),
    ('inline-twice', HEADER, 'if(point_distance(v))this->reached(inline_child);',
     'if(point_distance(v)){this->reached(inline_child);point_distance(v);}', 'inline'),
    ('inline-no-fallback', HEADER, 'else {this->reached(routed_child);dispatch(v,0x14dd70u,0x13155du);}',
     'else this->reached(routed_child);', 'inline'),
    ('frame-size', HEADER, 'entry_sp=gp[esp];gp[esp]-=0x454u;', 'entry_sp=gp[esp];gp[esp]-=0x450u;'),
    ('limit-load', HEADER, 'case limit:this->s.xmm[7]=load_ss(v);break;', 'case limit:this->s.xmm[7]=_mm_set1_ps(0)+load_ss(v);break;'),
    ('clear-direction', HEADER, 'at+=down?0xfffffffcu:4u;', 'at+=4u;'),
    ('child-flags', HEADER, 'this->compare32(gp[esi],logic::none);', ''),
    ('child-point', HEADER, 'case Local::point:return sp+0x468u;', 'case Local::point:return sp+0x46cu;'),
    ('visited-reload', HEADER, 'case kept_distance:this->s.xmm[0]=load_ss(v);break;',
     'case kept_distance:this->s.xmm[0]=_mm_move_ss(this->s.xmm[0],load_ss(v));break;'),
    ('exit-flags', HEADER, 'gp[esp]=this->flags.plus(gp[esp],0x454u,4);', 'gp[esp]+=0x454u;'),
    ('exit-ret', HEADER, 'this->ret(0x131606u,8);', 'this->ret(0x131606u,4);'),
    ('walk-handback', HEADER, 'w.bsp=gp[ebp];w.cluster=gp[edi];w.other=gp[esi];w.tail=gp[ebx];',
     'w.bsp=gp[ebp];w.cluster=gp[edi];w.other=gp[esi];'),
    ('output-prologue', HEADER, 'case Local::output:return frame_built?sp+0x46cu:entry_sp+8u;',
     'case Local::output:return frame_built?sp+0x46cu:entry_sp+4u;'),
    ('count-register', HEADER, 'case count_check:gp[eax]=v;break;', ''),
    # Logic (native_tu15_cluster_distance_flood_systems.hpp)
    ('start-bit', SYSTEMS, 'constexpr std::uint32_t visited_bit(std::uint32_t cluster){return 1u<<(cluster&31u);}',
     'constexpr std::uint32_t visited_bit(std::uint32_t cluster){return 2u<<(cluster&31u);}'),
    ('visited-word', SYSTEMS, 'return std::uint32_t(signed_value(cluster)>>5);', 'return cluster>>5;'),
    ('entry-stride', SYSTEMS, '} else entry+=std::uint32_t(sizeof(ClusterDistance));', '} else entry+=20u;'),
    ('record-size', SYSTEMS, 'offsetof(ClusterDistance,cluster))),sizeof(Cluster));', 'offsetof(ClusterDistance,cluster))),0xacu);'),
    ('portal-sign', SYSTEMS, 'if(signed_value(portals_here)>0) {', 'if(portals_here!=0) {'),
    ('portal-stride', SYSTEMS, 'number,sizeof(ClusterPortal));', 'number,32u);'),
    ('far-side', SYSTEMS, 'walk.other=widen16(memory.load16(back,', 'walk.other=(memory.load16(back,'),
    ('visited-compare', SYSTEMS, 'if(!sse::not_greater(distance,memory.load32(visited_distance,reached)))',
     'if(!sse::not_greater(memory.load32(visited_distance,reached),distance))'),
    ('limit-compare', SYSTEMS, '} else if(!sse::not_greater(limit_bits,distance)) {', '} else if(sse::not_greater(distance,limit_bits)) {'),
    ('append-order', SYSTEMS,
     '                        memory.store32(append_cluster,walk.tail-0x14u,walk.other);\n'
     '                        memory.store32(append_distance,walk.tail-0x10u,distance);\n',
     '                        memory.store32(append_distance,walk.tail-0x10u,distance);\n'
     '                        memory.store32(append_cluster,walk.tail-0x14u,walk.other);\n'),
    ('append-slot', SYSTEMS, 'memory.store16(entry_index_store,', 'memory.store32(entry_index_store,'),
    ('portal-bound', SYSTEMS, 'if(!(signed_value(portal)<signed_value(bound)))break;', 'if(!(signed_value(portal)<=signed_value(bound)))break;'),
    ('queue-bound', SYSTEMS, 'if(!(signed_value(read)<signed_value(length)))break;', 'if(!(signed_value(read)<=signed_value(length)))break;'),
    ('next-entry', SYSTEMS, 'entry=memory.load32(next_reload,local(Local::next_entry));', 'entry+=std::uint32_t(sizeof(ClusterDistance));'),
    ('start-order', SYSTEMS,
     '    memory.store32(start_cluster,at(output,offsetof(ClusterDistance,cluster)),start);\n'
     '    memory.store32(start_visited_mark,start_word,start_bits|visited_bit(start));\n',
     '    memory.store32(start_visited_mark,start_word,start_bits|visited_bit(start));\n'
     '    memory.store32(start_cluster,at(output,offsetof(ClusterDistance,cluster)),start);\n'),
    ('saved-order', HEADER, 'gp[edi]=this->pop(0x1315fcu);gp[esi]=this->pop(0x1315fdu);',
     'gp[esi]=this->pop(0x1315fcu);gp[edi]=this->pop(0x1315fdu);'),
]


def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def helper():
    spec = importlib.util.spec_from_file_location('native_cluster_distance_replacements', HELPER)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def compile_harness(scratch, name, includes, sanitizers=False):
    binary = scratch / name
    command = ['c++', '-std=c++20', '-O2', '-fno-fast-math', '-ffp-contract=off',
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
    parser.add_argument('--cases', type=int, default=8000)
    parser.add_argument('--mutant-cases', type=int, default=2000)
    parser.add_argument('--jobs', type=int, default=4)
    parser.add_argument('--skip-sanitizers', action='store_true')
    parser.add_argument('--skip-mutants', action='store_true')
    args = parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):
        parser.error('Run through tools/run_test.py')
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    sources = [UNIT, HEADER, SYSTEMS, SHARED, TICK_SYSTEMS, TEST, HELPER, Path(__file__).resolve()]
    before = {str(path.relative_to(ROOT)): digest(path) for path in sources}
    text = UNIT.read_text()
    module = helper()
    if not module.admitted(text):
        raise ValueError('Canonical 131470 body drifted from its pin')
    found = module.body(text, '131470')
    (scratch / 'flood.inc').write_text(found[0] + '\n')
    (scratch / 'flood_reference.inc').write_text(module.reference(found))
    output, installed = module.install(text, text)
    if installed != ['131470'] or module.PREFIX not in output:
        raise ValueError('Cluster distance binding did not install')
    if module.install(text, output)[1]:
        raise ValueError('Cluster distance binding is not idempotent')
    changed = text.replace(found[0], found[0].replace('0x454u', '0x450u', 1), 1)
    if changed == text or module.install(changed, changed)[1]:
        raise ValueError('Cluster distance binding accepted a changed body')
    direct = text.replace(found[0], found[0] + '\nstatic void run_14dd70(WorldState &s) {\n}', 1)
    if module.install(direct, direct)[1]:
        raise ValueError('Cluster distance binding accepted a direct 14DD70 route')
    # The translated 14DD70 the inline route replaces, pinned as its leaf.
    spec = importlib.util.spec_from_file_location('specialize_native_operands', SPECIALIZER)
    specializer = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(specializer)
    segment = specializer.leaf_body(SEGMENT_UNIT.read_text(), '14dd70')
    segment_pin = specializer.LEAVES['14dd70'][0]
    if not segment or specializer.digest(segment[0].replace(segment[1] + ' &s', 'STATE &s')) != segment_pin:
        raise ValueError('Canonical 14DD70 body drifted from its leaf pin')
    (scratch / 'segment_body.inc').write_text(segment[0] + '\n')
    sources += [LEAVES, SEGMENT_UNIT]
    before.update({str(path.relative_to(ROOT)): digest(path) for path in (LEAVES, SEGMENT_UNIT)})
    report = dict(passed=False, unit=str(UNIT.relative_to(ROOT)), pin=module.PIN, segment_pin=segment_pin)
    release = compile_harness(scratch, 'flood-release', [])
    result = run(release, args.cases)
    census = run(release, max(args.cases // 4, 1000), 'census')
    verify = run(release, max(args.cases // 4, 1000), 'verify')
    inline = run(release, args.cases, 'inline')
    inline_census = run(release, max(args.cases // 4, 1000), 'inline-census')
    inline_verify = run(release, max(args.cases // 4, 1000), 'inline-verify')
    block = re.search(r'enum Branch : unsigned \{(.*?)\};', HEADER.read_text(), re.S)[1]
    names = [name for name in re.sub(r'\s', '', block).split(',') if name and name != 'branch_count']
    coverage = result['coverage'] | verify['coverage'] | inline['coverage'] | inline_verify['coverage']
    hit = {name for index, name in enumerate(names) if coverage >> index & 1}
    missing = sorted(set(names) - hit)
    report.update(release=result, census=census, verify=verify, inline=inline, inline_census=inline_census,
                  inline_verify=inline_verify, branches=names, covered=sorted(hit), missing=missing)
    inline_bit, routed_bit = 1 << names.index('inline_child'), 1 << names.index('routed_child')
    if not (inline['coverage'] & inline_bit) or not (inline['coverage'] & routed_bit) or \
            not (inline_census['coverage'] & inline_bit) or (result['coverage'] & inline_bit):
        raise ValueError('Inline and dispatch 14DD70 routes were not each exercised in their modes')
    if missing:
        raise ValueError(f'Uncovered native cluster distance branches: {missing}')
    if not args.skip_sanitizers:
        sanitized = compile_harness(scratch, 'flood-sanitized', [], sanitizers=True)
        report['sanitized'] = run(sanitized, max(args.cases // 5, 1000))
        report['sanitized_verify'] = run(sanitized, 400, 'verify')
        report['sanitized_inline'] = run(sanitized, max(args.cases // 5, 1000), 'inline')
        report['sanitized_inline_verify'] = run(sanitized, 400, 'inline-verify')
        sanitized.unlink()
    texts = {path: path.read_text() for path in (HEADER, SYSTEMS)}

    def screen(mutant):
        name, target, old, new = mutant[:4]
        mode = mutant[4:] and [mutant[4]] or []
        text = texts[target]
        if text.count(old) != 1:
            return dict(name=name, rejected=False, error='mutation anchor missing')
        directory = scratch / 'mutants' / name / 'physics/runtime'
        directory.mkdir(parents=True)
        (directory / target.name).write_text(text.replace(old, new, 1))
        (directory / SHARED.name).write_text(SHARED.read_text())
        try:
            binary = compile_harness(scratch, f'mutant-{name}', [scratch / 'mutants' / name])
        except subprocess.CalledProcessError:
            return dict(name=name, rejected=False, error='mutant failed to compile')
        runs = subprocess.run([str(binary), str(args.mutant_cases), *mode], capture_output=True, text=True)
        binary.unlink()
        return dict(name=name, rejected=runs.returncode != 0, diagnostic=runs.stderr.strip()[-200:])
    mutants = []
    if not args.skip_mutants:
        with concurrent.futures.ThreadPoolExecutor(args.jobs) as pool:
            mutants = list(pool.map(screen, MUTANTS))
    report['mutants'] = mutants
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
    print(f"PASS {result['compared']} native cluster distance comparisons ({result['ordered_stores']} ordered stores, "
          f"{result['children']} genuine children, {result['faults']} faults), census {census['compared']}, "
          f"verify {verify['compared']}, inline {inline['compared']} (census {inline_census['compared']}, "
          f"verify {inline_verify['compared']}), {len(mutants)} mutants rejected")
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
