#!/usr/bin/env python3
"""Prove the native TU1.5 304350 welded collection agent: the logic in
native_tu15_world_leaves_systems.hpp (collection_agent::process_collision)
through its boundary adapter in native_tu15_world_leaves.hpp (kernel_304350,
with 303C70, 303D00, 303EE0, 303BA0, 2E47B0 and 30BAD0 inline).

Run through tools/run_test.py. Extracts the unmodified canonical translated
body run_300870 and the bodies of the inline children from the checked-in
physics world unit (pinned by tools/native_collection_agent_replacements.py and
the children's binders), their selected() operand cases and the binders'
verification references, and compiles tests/native_tu15_collection_agent_tests.cpp
against them. Compares every seeded case (ordered store events with
preimages, bank, registers, flags, XMM, x87, MXCSR, cursor; faults by
exception, cursor and bank) in Release, census and verify (record/replay with
every child boundary) modes and an ASan/UBSan build, and requires every
branch. Checks the production binding (install, idempotence, refusal of a
changed body, operand or inline child). Then screens deliberate defects in
the logic and the adapter: each mutant is an out-of-tree copy of one header
placed first on the include path and must be rejected in release or verify
mode. Source hashes are
rechecked after the screen.
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
TEST = ROOT / 'tests/native_tu15_collection_agent_tests.cpp'
HELPER = ROOT / 'tools/native_collection_agent_replacements.py'
BUFFERS = ROOT / 'tools/native_local_buffers_replacements.py'
ARRAY = ROOT / 'tools/native_contact_array_replacements.py'
LEAVES = ROOT / 'tools/native_world_leaves_replacements.py'
MUTANTS = [
    (SYSTEMS, 'gather-null-agent', 'if(memory.load32(entry_agent,at(entry,offsetof(ChildEntry,agent)))!=null) {',
     'if(memory.load32(entry_agent,at(entry,offsetof(ChildEntry,agent)))!=null+1u) {'),
    (SYSTEMS, 'gather-entry-stride', 'entry_offset=memory.load32(entry_offset_reload,local.entry_offset)+std::uint32_t(sizeof(ChildEntry));',
     'entry_offset=memory.load32(entry_offset_reload,local.entry_offset)+8u;'),
    (SYSTEMS, 'gather-buffer-stride', 'memory.load32(buffer_offset_next,local.buffer_offset)+local_buffers::shape_buffer_size;',
     'memory.load32(buffer_offset_next,local.buffer_offset)+0x80u;'),
    (SYSTEMS, 'gather-loop-bound', 'if(!(std::int32_t(index)<std::int32_t(size)))return agent;',
     'if(!(std::int32_t(index)<=std::int32_t(size)))return agent;'),
    (SYSTEMS, 'gather-predicate', 'if(memory.load8(verdict,frame.weld_predicate(dispatcher,type_a,type_b))) {',
     'if(!memory.load8(verdict,frame.weld_predicate(dispatcher,type_a,type_b))) {'),
    (SYSTEMS, 'gather-record-shape', 'memory.store32(shape_store,at(last_record(data,size),offsetof(ContactRecord,word_74)),child);',
     'memory.store32(shape_store,at(last_record(data,size),offsetof(ContactRecord,word_74)),key_again);'),
    (SYSTEMS, 'gather-state', 'kind==1u || kind==2u?1u:0u',
     'kind==1u?1u:0u'),
    (SYSTEMS, 'gather-rotated-source', 'frame.rotate_contact_normal(local.rotated_normal,motion+motion_rotation,at(contact,offsetof(ContactRecord,vector_40)));',
     'frame.rotate_contact_normal(local.rotated_normal,motion+motion_rotation,at(contact,offsetof(ContactRecord,vector_30)));'),
    (SYSTEMS, 'gather-welded-w', 'at(record,offsetof(ContactRecord,second_contact.position)+12u),x87::store(distance));',
     'at(record,offsetof(ContactRecord,second_contact.position)+8u),x87::store(distance));'),
    (SYSTEMS, 'x87-ties', 'inline bool greater(Extended a,float b){const auto operand=load(b);return a>operand;}',
     'inline bool greater(Extended a,float b){const auto operand=load(b);return a>=operand;}'),
    (SYSTEMS, 'x87-operand-conversion', 'const auto operand=load(b);volatile Extended difference=a-operand;',
     'volatile Extended difference=a-static_cast<Extended>(b);'),
    (SYSTEMS, 'x87-subtract-order', 'volatile Extended difference=a-operand;',
     'volatile Extended difference=operand-a;'),
    (SYSTEMS, 'sort-group-threshold', 'if(std::int32_t(n-index)>=4) {',
     'if(std::int32_t(n-index)>=5) {'),
    (SYSTEMS, 'sort-group-end', '} while(std::int32_t(candidate)<std::int32_t(n-3u));',
     '} while(std::int32_t(candidate)<std::int32_t(n-4u));'),
    (SYSTEMS, 'sort-swap-vectors', 'memory.store_vector(swap_to_first_vector_30,at(first,offsetof(ContactRecord,vector_30)),best_30);',
     'memory.store_vector(swap_to_first_vector_30,at(first,offsetof(ContactRecord,vector_30)),best_40);'),
    (SYSTEMS, 'sort-weld-first', 'const auto after=memory.load32(weld_index_reload,local.sort_index)+1u;',
     'const auto after=memory.load32(weld_index_reload,local.sort_index)+2u;'),
    (SYSTEMS, 'sort-next-offset', 'memory.store32(sort_offset_store,local.sort_offset,offset+record_size);',
     'memory.store32(sort_offset_store,local.sort_offset,offset+0x40u);'),
    (SYSTEMS, 'points-stride', 'point+=std::uint32_t(sizeof(ChildPoint));',
     'point+=0x20u;'),
    (SYSTEMS, 'points-distance-lane', 'memory.load_float(point_distance,at(point,offsetof(ChildPoint,normal)+12u))',
     'memory.load_float(point_distance,at(point,offsetof(ChildPoint,normal)+8u))'),
    (SYSTEMS, 'emit-confirm-bound', 'if(std::int32_t(index)<std::int32_t(n) && memory.load8(emit_has_contact',
     'if(std::int32_t(index)<=std::int32_t(n) && memory.load8(emit_has_contact'),
    (SYSTEMS, 'records-free-size', '(result.records_capacity&contact_array::capacity_mask)*record_size,',
     '(result.records_capacity&contact_array::capacity_mask)*0x100u,'),
    (HEADER, 'agent-handler', 'inline constexpr std::uint32_t collection_agent_handler=0x37edf6u;',
     'inline constexpr std::uint32_t collection_agent_handler=0x37edf7u;'),
    (HEADER, 'agent-unwind-state', 'case buffers_size:port.store(0x304422u,frame_base+0x32cu,1,1u);break;',
     'case buffers_size:port.store(0x304422u,frame_base+0x32cu,1,2u);break;'),
    (HEADER, 'agent-entry-register', 'reg(ebx)=entry_offset=flags.plus(reg(ebx),0xcu,4);',
     'entry_offset=flags.plus(reg(ebx),0xcu,4);'),
    (HEADER, 'agent-child-flags', 'reg(esi)=flags.plus(buffer_offset,buffers_base,4);',
     'reg(esi)=buffer_offset+buffers_base;'),
    (HEADER, 'agent-verdict-flags', 'byte_register(ecx,value);reg(esp)=flags.plus(reg(esp),0x10u,4);flags.logic(value,1);',
     'byte_register(ecx,value);reg(esp)=flags.plus(reg(esp),0x10u,4);'),
    (HEADER, 'agent-weld-this', 'reg(ecx)=port.read(0x30487bu,local.agent,4);',
     'port.read(0x30487bu,local.agent,4);'),
    (HEADER, 'agent-weld-flags', 'reg(edx)=port.flags.increment(reg(edx),4,false);',
     'reg(edx)=reg(edx)+1u;'),
    (HEADER, 'agent-point-pushes', 'port.push(0x30491cu,reg(ebx));port.push(0x30491du,0u);',
     'port.push(0x30491cu,0u);port.push(0x30491du,reg(ebx));'),
    (HEADER, 'agent-report-esi', 'reg(esi)=port.flags.plus(reg(esi),logic::collection_agent::point_list,4);port.push(0x304a00u,reg(esi));',
     'port.push(0x304a00u,reg(esi)+0x30u);'),
    (HEADER, 'agent-confirm-cleanup', 'reg(esp)=port.flags.plus(reg(esp),0x1cu,4);',
     'reg(esp)=port.flags.plus(reg(esp),0x18u,4);'),
    (HEADER, 'agent-x87-discard', 'case swap_first_position:discard();break;',
     'case swap_first_position:break;'),
    (HEADER, 'agent-capacity-state', 'port.store(0x304a78u,frame_base+0x32cu,4,0xffffffffu);',
     'port.store(0x304a78u,frame_base+0x32cu,1,0xffu);'),
    (HEADER, 'agent-exit-ecx', 'gp[ecx]=port.read(0x304aa3u,base+0x324u,4);',
     'gp[ecx]=port.read(0x304aa3u,base+0x320u,4);'),
    (HEADER, 'agent-x87-compare', 'case group_distance:case rest_distance:\n            port.s.require_x87();port.s.fp.comparison(',
     'case group_distance:case rest_distance:\n            port.s.require_x87();if(false)port.s.fp.comparison('),
]


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
    sources = [UNIT, HEADER, SYSTEMS, TEST, HELPER, BUFFERS, ARRAY, LEAVES, Path(__file__).resolve()]
    before = {str(path.relative_to(ROOT)): digest(path) for path in sources}
    text = UNIT.read_text()
    module = load(HELPER, 'native_collection_agent_replacements')
    buffers = load(BUFFERS, 'native_local_buffers_replacements')
    array = load(ARRAY, 'native_contact_array_replacements')
    leaves = load(LEAVES, 'native_world_leaves_replacements')
    if not module.admitted(text):
        raise ValueError('Canonical 304350 body, operands or children drifted from the pins')
    # The bodies: the owner and the children it runs inline (2E47B0 and 303BA0
    # in the leaves binder's reference form, as the build installs them).
    names = ['307fe0', '300100', '2e0cc0', '300400', '3001d0', '300260', module.NAME]
    bodies = ''.join(f'static void run_{name}(ObservedWorldState &);\n' for name in names)
    for name in names:
        found = module.body(text, name)[0]
        bodies += (leaves.reference(found) if name in ('300100', '2e0cc0') else found) + '\n'
    (scratch / 'agent_bodies.inc').write_text(bodies)
    references = (buffers.reference(module.body(text, '3001d0'), '3001d0') + buffers.reference(module.body(text, '300260'), '300260') +
                  array.reference(module.body(text, '300400')) + module.reference(module.body(text, module.NAME)))
    (scratch / 'agent_reference.inc').write_text(references)
    selected = dict(module.OPERANDS)
    selected.update(module.NULL_AGENT[2])
    selected.update(array.OPERANDS)
    for name in buffers.OWNERS:
        selected.update(buffers.OWNERS[name][2])
    (scratch / 'agent_selected.inc').write_text(''.join(f'case 0x{a:x}u:return 0x{b:x}u;\n' for a, b in sorted(selected.items())))
    # Production binding: installs, idempotent, refuses a changed body, a
    # changed operand or a changed inline child.
    output, installed = module.install(text, text)
    if installed != [module.ENTRY] or module.prefix() not in output or module.REFERENCE not in output:
        raise ValueError('304350 binding did not install')
    if module.install(text, output)[1]:
        raise ValueError('304350 binding is not idempotent')
    found = module.body(text, module.NAME)
    changed = text.replace(found[0], found[0].replace('0x318u,4)', '0x31cu,4)', 1), 1)
    if changed == text or module.install(changed, changed)[1]:
        raise ValueError('304350 binding accepted a changed body')
    remapped = text.replace('case 0x4693acu:return 0x46e46cu;', 'case 0x4693acu:return 0x46e470u;', 1)
    if remapped == text or module.install(remapped, remapped)[1]:
        raise ValueError('304350 binding accepted a changed operand mapping')
    null = module.body(text, '307fe0')
    child_changed = text.replace(null[0], null[0].replace('0x487ac4u', '0x487ac8u', 1), 1)
    if child_changed == text or module.install(child_changed, child_changed)[1]:
        raise ValueError('304350 binding accepted a changed inline child')
    report = dict(passed=False, unit=str(UNIT.relative_to(ROOT)), pin=module.PIN)
    release = compile_harness(scratch, 'agent-release', [])
    result = run(release, args.cases)
    census = run(release, max(args.cases // 4, 1000), 'census')
    verify = run(release, max(args.cases // 4, 1000), 'verify')
    header = HEADER.read_text()
    offset = (len(enum_names(header, 'Branch')) - 1 + len(enum_names(header, 'ArrayBranch')) - 1 +
              len(enum_names(header, 'LocalBufferBranch')) - 1)
    names = [n for n in enum_names(header, 'CollectionAgentBranch') if n != 'collection_branch_end']
    coverage = result['coverage'] | verify['coverage']
    hit = {name for index, name in enumerate(names) if coverage >> (offset + index) & 1}
    missing = sorted(set(names) - hit)
    report.update(release=result, census=census, verify=verify, branches=names, covered=sorted(hit), missing=missing)
    if missing:
        raise ValueError(f'Uncovered 304350 branches: {missing}')
    if census['compared'] == 0 or verify['compared'] == 0 or result['child_calls'] == 0 or result['gathered_records'] == 0:
        raise ValueError('Census, verify, the children or the gathered records never ran')
    if not args.skip_sanitizers:
        sanitized = compile_harness(scratch, 'agent-sanitized', [], sanitizers=True)
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
        # body, which is how an access-only defect (such as the search's
        # grouping) shows.
        runs = subprocess.run([str(binary), str(args.mutant_cases)], capture_output=True, text=True)
        if runs.returncode == 0:
            runs = subprocess.run([str(binary), str(max(args.mutant_cases // 3, 200)), 'verify'], capture_output=True, text=True)
        binary.unlink()
        return dict(name=name, rejected=runs.returncode != 0, diagnostic=runs.stderr.strip()[-200:])
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
    print(f"PASS {result['compared']} native 304350 comparisons ({result['ordered_stores']} ordered stores, "
          f"{result['faults']} faults, {result['child_calls']} child calls, {result['gathered_records']} gathered records), "
          f"census {census['compared']}, verify {verify['compared']}, {len(mutants)} mutants rejected")
    return 0

if __name__ == '__main__':
    raise SystemExit(main())
