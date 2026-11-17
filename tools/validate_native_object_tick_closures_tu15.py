#!/usr/bin/env python3
"""Prove the native TU1.5 per-tick object update owners (B8F40/B9090/B9180/BE4A0/10DD10).

Run through tools/run_test.py. Extracts the five unmodified translated bodies
from the checked-in live-support unit (pinned) and the verification
references the production binder generates, compiles
tests/native_tu15_object_tick_tests.cpp against them, and compares every
seeded case (ordered store events with preimages, child call log, bank,
registers, flags, XMM, cursor; faults by exception, cursor and store prefix)
in Release, census and verify modes and an ASan/UBSan build. Requires
complete reachable branch coverage. Checks the production binding (install,
idempotence, refusal of changed or childless bodies, references without an
unrouted child). Then screens deliberate defects: each mutant is an
out-of-tree copy of the native header placed first on the include path and
must be rejected. Source hashes are rechecked after the screen.
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
UNIT = ROOT / 'src/physics/physics_live_support_tu15_generated.cpp'
HEADER = ROOT / 'src/physics/runtime/native_tu15_object_tick.hpp'
SYSTEMS = ROOT / 'src/physics/runtime/native_tu15_object_tick_systems.hpp'
TEST = ROOT / 'tests/native_tu15_object_tick_tests.cpp'
HELPER = ROOT / 'tools/native_object_tick_replacements.py'
OWNERS = ('b8f40', 'b9090', 'b9180', 'be4a0', '10dd10')
# Unreachable by construction: a bit found by the forward scan has an index
# at least the scan start, which the preceding TEST/JL proved non-negative,
# so the CMP EAX,-1 exits never fire.
UNREACHABLE = {'a_scan_invalid', 'b_scan_invalid', 'c_scan_invalid', 'c_late_invalid'}
# (name, file, anchor, replacement). The logic lives in SYSTEMS and the ABI
# in the HEADER adapter; each mutant is an out-of-tree copy of its file.
MUTANTS = [
    # Logic (native_tu15_object_tick_systems.hpp)
    ('root-count-sign', SYSTEMS, 'if(signed_value(count)>0) do {', 'if(signed_value(count)>=0) do {'),
    ('root-awake', SYSTEMS, 'return (flags&header_flags::active) && (flags&header_flags::awake) && !(flags&header_flags::late_update);',
     'return (flags&header_flags::active) && !(flags&header_flags::late_update);'),
    ('attach-reread', SYSTEMS, 'if(moves_now(memory.load8(root_flags_after,at(walk.header,offsetof(ObjectHeader,flags)))))',
     'if(moves_now(memory.load8(root_flags_after,at(walk.handle,offsetof(ObjectHeader,flags)))))'),
    ('scan-bit', SYSTEMS, '&(1u<<(index&31u)))) {', '&(1u<<((index+1u)&31u)))) {'),
    ('handle-salt', SYSTEMS, 'return (identifier<<16)|index;', 'return (identifier<<15)|index;'),
    ('disabled-bit', SYSTEMS, 'inline constexpr std::uint32_t disabled=0x08u;', 'inline constexpr std::uint32_t disabled=0x04u;'),
    ('move-flag', SYSTEMS, 'constexpr bool moves_now(std::uint32_t flags){return (flags&header_flags::moved) && !(flags&header_flags::late_update);}',
     'constexpr bool moves_now(std::uint32_t flags){return (flags&header_flags::moved);}'),
    ('late-clear', SYSTEMS, 'flags&=~header_flags::held;', 'flags&=0xffu;'),
    ('late-read-order', SYSTEMS,
     '        auto flags=memory.load8(datum_flags,at(late.header,offsetof(ObjectHeader,flags)));\n'
     '        late.handle=make_handle(memory.load16(datum_identifier,at(late.header,offsetof(ObjectHeader,identifier))),late.index);\n',
     '        late.handle=make_handle(memory.load16(datum_identifier,at(late.header,offsetof(ObjectHeader,identifier))),late.index);\n'
     '        auto flags=memory.load8(datum_flags,at(late.header,offsetof(ObjectHeader,flags)));\n'),
    ('late-moved', SYSTEMS, 'offsetof(ObjectHeader,flags)))&header_flags::moved)frame.move_object(late);',
     'offsetof(ObjectHeader,flags)))&header_flags::active)frame.move_object(late);'),
    ('datum-read-order', SYSTEMS,
     '    const auto datum_size=memory.load32(size,at(array,offsetof(DataArray,datum_size)));\n'
     '    return datum_size*index+memory.load32(data,at(array,offsetof(DataArray,data)));',
     '    const auto datum_data=memory.load32(data,at(array,offsetof(DataArray,data)));\n'
     '    return memory.load32(size,at(array,offsetof(DataArray,datum_size)))*index+datum_data;'),
    ('updating-order', SYSTEMS,
     '    frame.keep_root_count(count);\n    memory.store8(updating_set,at(object_globals,offsetof(ObjectGlobals,updating)),1);',
     '    memory.store8(updating_set,at(object_globals,offsetof(ObjectGlobals,updating)),1);\n    frame.keep_root_count(count);'),
    ('single-mode', SYSTEMS, 'if(game_mode>=4 && game_mode<=5 &&', 'if(game_mode>=4 && game_mode<=6 &&'),
    ('single-quiet', SYSTEMS, 'now&~header_flags::awake);', 'now&~header_flags::moved);'),
    ('single-sibling', SYSTEMS, 'u.child=memory.load32(next_sibling,at(child,offsetof(ObjectData,next_sibling)));',
     'u.child=memory.load32(next_sibling,at(child,offsetof(ObjectData,parent)));'),
    ('single-timer-owner', SYSTEMS, 'memory.load32(timer_owner,at(block,offsetof(TimerBlock,owner)))!=none',
     'memory.load32(timer_owner,at(block,offsetof(TimerBlock,first_timer)))!=none'),
    ('types-index', SYSTEMS, 'offsetof(ObjectTypeDefinition,type_chain)),widen16(walk.position),4);',
     'offsetof(ObjectTypeDefinition,type_chain)),walk.position&0xffffu,4);'),
    ('types-or', SYSTEMS, 'walk.changed|=frame.type_update(callback,walk);', 'walk.changed=frame.type_update(callback,walk);'),
    # Adapter (native_tu15_object_tick.hpp)
    ('push-site', HEADER, 'this->push(0xb8f4cu,gp[ebx]);this->push(0xb8f4du,gp[ebp]);', 'this->push(0xb8f4du,gp[ebx]);this->push(0xb8f4cu,gp[ebp]);'),
    ('count-slot', HEADER, 'gp[eax]=this->read(0xb8fafu,gp[esp]+0x10u,4);', 'gp[eax]=this->read(0xb8fafu,gp[esp]+0x0cu,4);'),
    ('exit-register', HEADER, '*end==logic::SearchEnd::past_limit?ecx:edx]=v;', '*end==logic::SearchEnd::past_limit?edx:edx]=v;'),
    ('root-handback', HEADER, 'w.list=gp[ebx];w.position=gp[ebp];w.handle=gp[esi];w.header=gp[edi];}',
     'w.list=gp[ebx];w.handle=gp[esi];w.header=gp[edi];}'),
    ('scan-handback', HEADER, 'this->call(0xb9061u,0xb9066u,0xbe4a0u,update);\n        scan.array=gp[esi];scan.index=gp[edi];',
     'this->call(0xb9061u,0xb9066u,0xbe4a0u,update);\n        scan.array=gp[esi];'),
    ('probe-register', HEADER, 'if(!first_probe)++gp[position];', '++gp[position];'),
    ('move-transform-args', HEADER, 'keep(w);this->gp[ecx]=this->gp[edi];this->gp[eax]=this->gp[esi];',
     'keep(w);this->gp[ecx]=this->gp[esi];this->gp[eax]=this->gp[edi];'),
    ('move-search-limit', HEADER, 'search_end_flags(*this,s,edi);this->reached(search_branch(s,b_scan_negative));',
     'search_end_flags(*this,s,esi);this->reached(search_branch(s,b_scan_negative));'),
    ('move-exit-flags', HEADER, 'void finish(){this->call(0xb9121u,0xb9126u,0x102910u,finished);}',
     'void finish(){this->test32(0);this->call(0xb9121u,0xb9126u,0x102910u,finished);}'),
    ('late-arg-push', HEADER, 'this->push(0xb9267u,gp[edi]);return 0xb9268u;', 'return 0xb9268u;'),
    ('late-arg', HEADER, 'this->push(0xb92aau,1);', 'this->push(0xb92aau,0);'),
    ('late-tail-pop', HEADER, 'this->gp[ebx]=this->pop(0xb9219u);', 'this->gp[ebx]=this->pop(0xb9219u);this->gp[edi]=this->gp[ebx];'),
    ('single-arg-slot', HEADER, 'void keep_header(std::uint32_t header){this->store(0xbe4c6u,this->gp[esp]+0x10u,4,header);}',
     'void keep_header(std::uint32_t header){this->store(0xbe4c6u,this->gp[esp]+0x0cu,4,header);}'),
    ('single-deleted', HEADER, 'set_low_byte(gp[eax],1);gp[ebx]=this->pop(0xbe4d8u);', 'set_low_byte(gp[eax],0);gp[ebx]=this->pop(0xbe4d8u);'),
    ('single-owner-flags', HEADER, 'case timers_offset:this->flags.minus(v,0xffffu,2);break;', 'case timers_offset:this->flags.minus(v,0xfffeu,2);break;'),
    ('single-or', HEADER, 'this->call(0xbe5c5u,0xbe5cau,0x1d6fb0u,cluster_child);',
     'this->compare32(gp[edi],gp[eax]);this->call(0xbe5c5u,0xbe5cau,0x1d6fb0u,cluster_child);'),
    ('single-countdown', HEADER, 'this->flags.increment(gp[eax]&0xffu,1,true)', 'this->flags.increment(gp[eax]&0xffu,1,false)'),
    ('single-ret', HEADER, 'this->ret(0xbe5ffu,4);', 'this->ret(0xbe5ffu,0);'),
    ('single-changed-handback', HEADER,
     'u.handle=gp[edi];if(esi_value)*esi_value=gp[esi];u.object=gp[ebp];u.changed=std::uint8_t(gp[ebx]);',
     'u.handle=gp[edi];if(esi_value)*esi_value=gp[esi];u.object=gp[ebp];'),
    ('types-flags', HEADER, 'this->test32(callback);this->push(0x10dd5au,gp[ebp]);', 'this->push(0x10dd5au,gp[ebp]);'),
    ('cursor', HEADER, 'void publish(std::uint32_t final_pc){s.gp=gp;s.flags=flags;if(cursor)*cursor=final_pc;}',
     'void publish(std::uint32_t final_pc){s.gp=gp;s.flags=flags;if(cursor)*cursor=final_pc+1u;}'),
]


def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def helper():
    spec = importlib.util.spec_from_file_location('native_object_tick_replacements', HELPER)
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
    parser.add_argument('--cases', type=int, default=10000)
    parser.add_argument('--mutant-cases', type=int, default=2500)
    parser.add_argument('--jobs', type=int, default=4)
    parser.add_argument('--skip-sanitizers', action='store_true')
    parser.add_argument('--skip-mutants', action='store_true')
    args = parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):
        parser.error('Run through tools/run_test.py')
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    sources = [UNIT, HEADER, SYSTEMS, TEST, HELPER, Path(__file__).resolve()]
    before = {str(path.relative_to(ROOT)): digest(path) for path in sources}
    text = UNIT.read_text()
    module = helper()
    if module.admitted(text) != list(OWNERS):
        raise ValueError(f'Canonical object tick bodies drifted from their pins: {module.admitted(text)}')
    for name in OWNERS:
        found = module.body(text, name)
        (scratch / f'tick_{name}.inc').write_text(found[0] + '\n')
        (scratch / f'tick_{name}_reference.inc').write_text(module.reference(found, name))
    # Production binding: installs, idempotent, refuses a changed body or a
    # missing child, and its references route every child to the replay.
    output, installed = module.install(text, text)
    if installed != list(OWNERS) or any(module.prefix(name) not in output for name in OWNERS):
        raise ValueError('Object tick binding did not install')
    if module.install(text, output)[1]:
        raise ValueError('Object tick binding is not idempotent')
    if output.count(module.REFERENCES_BEGIN) != 1 or output.index(module.REFERENCES_BEGIN) > output.index('static void run_b8f40(WorldState &s) {'):
        raise ValueError('Object tick references are not in the unit prefix')
    original = module.body(text, 'b9090')[0]
    changed = text.replace(original, original.replace('0x1d80u', '0x1d84u', 1), 1)
    if changed == text or 'b9090' in module.install(changed, changed)[1]:
        raise ValueError('Object tick binding accepted a changed B9090 body')
    child = module.body(text, '10e630')[0]
    childless = text.replace(child, '', 1)
    if 'b8f40' in module.install(childless, childless)[1]:
        raise ValueError('Object tick binding accepted B8F40 without its 10E630 body')
    report = dict(passed=False, unit=str(UNIT.relative_to(ROOT)), pins=module.PINS)
    release = compile_harness(scratch, 'tick-release', [])
    result = run(release, args.cases)
    census = run(release, max(args.cases // 4, 1000), 'census')
    verify = run(release, max(args.cases // 4, 1000), 'verify')
    block = re.sub(r'//[^\n]*', '', re.search(r'enum Branch : unsigned \{(.*?)\};', HEADER.read_text(), re.S)[1])
    names = [name for name in re.sub(r'\s', '', block).split(',') if name and name != 'branch_count']
    coverage = result['coverage'] | verify['coverage']
    hit = {name for index, name in enumerate(names) if coverage >> index & 1}
    missing = sorted(set(names) - hit - UNREACHABLE)
    report.update(release=result, census=census, verify=verify, branches=names, covered=sorted(hit),
                  unreachable=sorted(UNREACHABLE), missing=missing)
    if missing:
        raise ValueError(f'Uncovered native object tick branches: {missing}')
    if census['coverage'] == 0:
        raise ValueError('Census mode never admitted the native closures')
    if not args.skip_sanitizers:
        sanitized = compile_harness(scratch, 'tick-sanitized', [], sanitizers=True)
        report['sanitized'] = run(sanitized, max(args.cases // 5, 1000))
        report['sanitized_verify'] = run(sanitized, 400, 'verify')
        sanitized.unlink()
    texts = {path: path.read_text() for path in (HEADER, SYSTEMS)}

    def screen(mutant):
        name, target, old, new = mutant
        text = texts[target]
        if text.count(old) != 1:
            return dict(name=name, rejected=False, error='mutation anchor missing')
        directory = scratch / 'mutants' / name / 'physics/runtime'
        directory.mkdir(parents=True)
        (directory / target.name).write_text(text.replace(old, new, 1))
        try:
            binary = compile_harness(scratch, f'mutant-{name}', [scratch / 'mutants' / name])
        except subprocess.CalledProcessError:
            return dict(name=name, rejected=False, error='mutant failed to compile')
        runs = subprocess.run([str(binary), str(args.mutant_cases)], capture_output=True, text=True)
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
    print(f"PASS {result['compared']} native object tick comparisons ({result['ordered_stores']} ordered stores, "
          f"{result['children']} genuine children, {result['faults']} faults), census {census['compared']}, "
          f"verify {verify['compared']}, {len(mutants)} mutants rejected")
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
