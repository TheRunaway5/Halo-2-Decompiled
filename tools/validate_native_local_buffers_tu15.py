#!/usr/bin/env python3
"""Prove the native TU1.5 303C70/303D00 local shape buffers: the logic in
native_tu15_world_leaves_systems.hpp (local_buffers::construct and destroy)
through its boundary adapters in native_tu15_world_leaves.hpp (kernel_303c70,
kernel_303d00).

Run through tools/run_test.py. Extracts the unmodified canonical translated
bodies run_3001d0 (303C70) and run_300260 (303D00) from the checked-in physics
world unit (pinned by tools/native_local_buffers_replacements.py), their
selected() operand cases and the binder's verification references, and
compiles tests/native_tu15_local_buffers_tests.cpp against them. Compares every
seeded case (ordered store events with preimages, bank, registers, flags, XMM,
x87, MXCSR, cursor; faults by exception, cursor and bank) in Release, census
and verify (record/replay with the dispatch boundaries) modes and an
ASan/UBSan build, and requires every branch. Checks the production binding
(install, idempotence, refusal of a changed body or operand). Then screens
deliberate defects in the logic and the adapters: each mutant is an
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
TEST = ROOT / 'tests/native_tu15_local_buffers_tests.cpp'
HELPER = ROOT / 'tools/native_local_buffers_replacements.py'
MUTANTS = [
    # logic
    (SYSTEMS, 'buffers-signed-compare', 'if(std::int32_t(bytes)>std::int32_t(left)) {', 'if(bytes>left) {'),
    (SYSTEMS, 'buffers-element-size', 'inline constexpr std::uint32_t shape_buffer_size=0x100u;', 'inline constexpr std::uint32_t shape_buffer_size=0x80u;'),
    (SYSTEMS, 'buffers-bump-left', 'at(thread,offsetof(ThreadMemory,stack_left)),left-bytes);', 'at(thread,offsetof(ThreadMemory,stack_left)),left+bytes);'),
    (SYSTEMS, 'buffers-block-field', 'memory.store32(block_store,at(array,offsetof(LocalArray,stack_block)),made.block);',
     'memory.store32(block_store,at(array,offsetof(LocalArray,stack_block)),made.bytes);'),
    (SYSTEMS, 'buffers-capacity-flag', 'capacity|storage_not_owned);', 'capacity);'),
    (SYSTEMS, 'buffers-mark', 'done.released_virtually=block_address==*done.mark || *done.mark==0u;', 'done.released_virtually=block_address==*done.mark;'),
    (SYSTEMS, 'buffers-empty-block', '    if(done.bytes) {\n', '    if(true) {\n'),
    (SYSTEMS, 'buffers-owned', 'if(!(done.capacity&storage_not_owned)) {', 'if(done.capacity&storage_not_owned) {'),
    (SYSTEMS, 'buffers-owned-size', '(done.capacity&contact_array::capacity_mask)*shape_buffer_size,array_storage_class',
     '(done.capacity&contact_array::capacity_mask)*0x80u,array_storage_class'),
    (SYSTEMS, 'buffers-pop-order',
     '        memory.store32(popped_next_store,at(thread,offsetof(ThreadMemory,stack_next)),done.popped_next);\n'
     '        memory.store32(popped_left_store,at(thread,offsetof(ThreadMemory,stack_left)),done.popped_left);',
     '        memory.store32(popped_left_store,at(thread,offsetof(ThreadMemory,stack_left)),done.popped_left);\n'
     '        memory.store32(popped_next_store,at(thread,offsetof(ThreadMemory,stack_next)),done.popped_next);'),
    # adapters
    (HEADER, 'buffers-handler', 'inline constexpr std::uint32_t local_buffers_handler=0x37ed68u;',
     'inline constexpr std::uint32_t local_buffers_handler=0x37ed69u;'),
    (HEADER, 'buffers-unwind-state', 'port.store(0x303ca9u,entry-4u,4,0u);', 'port.store(0x303ca9u,entry-8u,4,0u);'),
    (HEADER, 'buffers-this-slot', 'port.store(0x303c8cu,entry-0x10u,4,array);', 'port.store(0x303c8cu,entry-0x14u,4,array);'),
    (HEADER, 'buffers-child-flags', '            port.flags.minus(gp[eax],value,4);', '            port.flags.minus(value,gp[eax],4);'),
    (HEADER, 'buffers-child-vtable-register', 'gp[edx]=port.read(0x303cbcu,thread,4);', 'port.read(0x303cbcu,thread,4);'),
    (HEADER, 'buffers-bump-edx', 'gp[edx]=port.flags.minus(gp[edx],gp[eax],4);gp[eax]=next;', 'port.flags.minus(gp[edx],gp[eax],4);gp[eax]=next;'),
    (HEADER, 'buffers-exit-flags', '    gp[esp]=port.flags.plus(gp[esp],0x10u,4);\n    port.ret(0x303cfdu,4u);',
     '    gp[esp]+=0x10u;\n    port.ret(0x303cfdu,4u);'),
    (HEADER, 'buffers-ret', 'port.ret(0x303cfdu,4u);', 'port.ret(0x303cfdu,0u);'),
    (HEADER, 'destroy-mark-flags', 'if(gp[edx]!=value)port.flags.logic(value,4);', 'if(false)port.flags.logic(value,4);'),
    (HEADER, 'destroy-pushed-class', 'port.push(0x303d7du,logic::local_buffers::array_storage_class);', 'port.push(0x303d7du,0x13u);'),
    (HEADER, 'destroy-state-width', 'port.store(0x303d66u,entry-4u,4,0xffffffffu);', 'port.store(0x303d66u,entry-4u,1,0xffu);'),
    (HEADER, 'destroy-pop-order', 'gp[edi]=port.pop(0x303d8du);gp[esi]=port.pop(0x303d8eu);', 'gp[esi]=port.pop(0x303d8du);gp[edi]=port.pop(0x303d8eu);'),
    (HEADER, 'destroy-capacity-register', 'gp[eax]=port.flags.logic(value,4);capacity=value;', 'port.flags.logic(value,4);capacity=value;'),
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
    parser.add_argument('--mutant-cases', type=int, default=4000)
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
    module = load(HELPER, 'native_local_buffers_replacements')
    references = ''
    selected = {}
    for name in module.OWNERS:
        if not module.admitted(text, name):
            raise ValueError(f'Canonical {name} body or operands drifted from the pin')
        found = module.body(text, name)
        (scratch / f'buffers_{name}.inc').write_text(found[0] + '\n')
        references += module.reference(found, name)
        selected.update(module.OWNERS[name][2])
    (scratch / 'buffers_reference.inc').write_text(references)
    (scratch / 'buffers_selected.inc').write_text(''.join(f'case 0x{a:x}u:return 0x{b:x}u;\n' for a, b in sorted(selected.items())))
    # Production binding: installs both, idempotent, refuses a changed body or
    # a changed operand binding.
    output, installed = module.install(text, text)
    if sorted(installed) != ['303c70', '303d00'] or any(module.prefix(name) not in output for name in module.OWNERS):
        raise ValueError('Local buffer bindings did not install')
    if module.install(text, output)[1]:
        raise ValueError('Local buffer bindings are not idempotent')
    for name in module.OWNERS:
        found = module.body(text, name)
        changed = text.replace(found[0], found[0].replace('0xfu,4)', '0xeu,4)', 1), 1)
        if changed == text or module.OWNERS[name][0] in module.install(changed, changed)[1]:
            raise ValueError(f'{name} binding accepted a changed body')
    remapped = text.replace('case 0x487638u:return 0x48c938u;', 'case 0x487638u:return 0x48c93cu;', 1)
    if remapped == text or module.install(remapped, remapped)[1]:
        raise ValueError('Local buffer bindings accepted a changed operand mapping')
    report = dict(passed=False, unit=str(UNIT.relative_to(ROOT)), pins={n: o[1] for n, o in module.OWNERS.items()})
    release = compile_harness(scratch, 'buffers-release', [])
    result = run(release, args.cases)
    census = run(release, max(args.cases // 4, 1000), 'census')
    verify = run(release, max(args.cases // 4, 1000), 'verify')
    header = HEADER.read_text()
    offset = len(enum_names(header, 'Branch')) - 1 + len(enum_names(header, 'ArrayBranch')) - 1
    names = [n for n in enum_names(header, 'LocalBufferBranch') if n != 'local_buffer_branch_end']
    coverage = result['coverage'] | verify['coverage']
    hit = {name for index, name in enumerate(names) if coverage >> (offset + index) & 1}
    missing = sorted(set(names) - hit)
    report.update(release=result, census=census, verify=verify, branches=names, covered=sorted(hit), missing=missing)
    if missing:
        raise ValueError(f'Uncovered local buffer branches: {missing}')
    if census['compared'] == 0 or verify['compared'] == 0 or result['child_calls'] == 0:
        raise ValueError('Census, verify or the dispatch children never ran')
    if not args.skip_sanitizers:
        sanitized = compile_harness(scratch, 'buffers-sanitized', [], sanitizers=True)
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
    print(f"PASS {result['compared']} native 303C70/303D00 comparisons ({result['ordered_stores']} ordered stores, "
          f"{result['faults']} faults, {result['child_calls']} dispatch children), census {census['compared']}, "
          f"verify {verify['compared']}, {len(mutants)} mutants rejected")
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
