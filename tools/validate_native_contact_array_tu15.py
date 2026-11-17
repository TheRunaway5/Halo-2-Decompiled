#!/usr/bin/env python3
"""Prove the native TU1.5 303EE0 contact record append: the logic in
native_tu15_world_leaves_systems.hpp (contact_array::expand_one) through its
boundary adapter in native_tu15_world_leaves.hpp (kernel_303ee0).

Run through tools/run_test.py. Extracts the unmodified canonical translated
bodies run_300400 (303EE0) and run_300100 (303BA0) from the checked-in
physics world unit (pinned by tools/native_contact_array_replacements.py),
their selected() operand cases and the binder's verification reference, and
compiles tests/native_tu15_contact_array_tests.cpp against them. Compares
every seeded case (ordered store events with preimages, bank, registers,
flags, XMM, MXCSR, cursor; faults by exception, cursor and bank) in Release,
census and verify (record/replay with the child boundary) modes and an
ASan/UBSan build, and requires both branches (room, growth). Checks the
production binding (install, idempotence, refusal of a changed body or
operand). Then screens deliberate defects in the logic and the adapter: each
mutant is an out-of-tree copy of one header placed first on the include path
and must be rejected. Source hashes are rechecked after the screen.
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
TEST = ROOT / 'tests/native_tu15_contact_array_tests.cpp'
HELPER = ROOT / 'tools/native_contact_array_replacements.py'
LEAVES = ROOT / 'tools/native_world_leaves_replacements.py'
MUTANTS = [
    (SYSTEMS, 'append-mask', 'offsetof(HavokArray,capacity_and_flags)))&capacity_mask;', 'offsetof(HavokArray,capacity_and_flags)));'),
    (SYSTEMS, 'append-compare', 'if(count==limit)frame.reserve_more(', 'if(count>=limit)frame.reserve_more('),
    (SYSTEMS, 'append-stride', 'frame.copy_record(element(elements,index,sizeof(ContactRecord)),local);', 'frame.copy_record(element(elements,index,0x7eu),local);'),
    (SYSTEMS, 'append-size', 'offsetof(HavokArray,size)),index+1u);', 'offsetof(HavokArray,size)),index+2u);'),
    (SYSTEMS, 'append-result', 'element(final_data,final_size-1u,sizeof(ContactRecord))', 'element(final_data,final_size,sizeof(ContactRecord))'),
    (SYSTEMS, 'append-clear', 'at(local,offsetof(ContactRecord,state)),0u);', 'at(local,offsetof(ContactRecord,state)),1u);'),
    (HEADER, 'append-argument', 'if(a==logic::contact_array::Access::size_store)port.push(0x303f2du,local);', 'if(false)port.push(0x303f2du,local);'),
    (HEADER, 'append-child-registers', 'port.gp[eax]=full_size;port.gp[ecx]=full_size;', 'port.gp[ecx]=full_size;'),
    (HEADER, 'append-child-flags', 'port.gp[ecx]=full_size;port.flags.minus(full_size,full_size,4);', 'port.gp[ecx]=full_size;'),
    (HEADER, 'append-cdecl', 'port.gp[esp]=port.flags.plus(port.gp[esp],8u,4);', 'port.gp[esp]=port.flags.plus(port.gp[esp],4u,4);'),
    (HEADER, 'append-exit-flags', 'port.flags.shift(result.size,7u,4);', 'port.flags.shift(result.size,6u,4);'),
    (HEADER, 'append-local', 'frame{port,aligned-0x80u,reserve};', 'frame{port,aligned-0x7cu,reserve};'),
    (HEADER, 'append-data-register', 'gp[eax]=result.record;gp[ecx]=result.data;', 'gp[eax]=result.record;'),
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
    sources = [UNIT, HEADER, SYSTEMS, TEST, HELPER, LEAVES, Path(__file__).resolve()]
    before = {str(path.relative_to(ROOT)): digest(path) for path in sources}
    text = UNIT.read_text()
    module = load(HELPER, 'native_contact_array_replacements')
    leaves = load(LEAVES, 'native_world_leaves_replacements')
    if not module.admitted(text):
        raise ValueError('Canonical 303EE0 body, operands or children drifted from the pin')
    found = module.body(text, module.NAME)
    (scratch / 'array_300400.inc').write_text(found[0] + '\n')
    (scratch / 'array_300100.inc').write_text(leaves.reference(module.body(text, module.COPY)[0]) + '\n')
    (scratch / 'array_reference.inc').write_text(module.reference(found))
    (scratch / 'array_selected.inc').write_text(''.join(f'case 0x{a:x}u:return 0x{b:x}u;\n' for a, b in sorted(module.OPERANDS.items())))
    # Production binding: installs, idempotent, refuses a changed body or a
    # changed operand binding.
    output, installed = module.install(text, text)
    if installed != [module.ENTRY] or module.prefix() not in output or module.REFERENCE not in output:
        raise ValueError('303EE0 binding did not install')
    if module.install(text, output)[1]:
        raise ValueError('303EE0 binding is not idempotent')
    changed = text.replace(found[0], found[0].replace('0x7fffffffu', '0x7ffffffeu', 1), 1)
    if changed == text or module.install(changed, changed)[1]:
        raise ValueError('303EE0 binding accepted a changed body')
    remapped = text.replace('case 0x300439u:return 0x303f19u;', 'case 0x300439u:return 0x303f1au;', 1)
    if remapped == text or module.install(remapped, remapped)[1]:
        raise ValueError('303EE0 binding accepted a changed operand mapping')
    report = dict(passed=False, unit=str(UNIT.relative_to(ROOT)), pin=module.PIN)
    release = compile_harness(scratch, 'array-release', [])
    result = run(release, args.cases)
    census = run(release, max(args.cases // 4, 1000), 'census')
    verify = run(release, max(args.cases // 4, 1000), 'verify')
    block = re.search(r'enum ArrayBranch : unsigned \{(.*?)\};', HEADER.read_text(), re.S)[1]
    names = [name.split('=')[0] for name in re.sub(r'\s', '', block).split(',') if name and name != 'array_branch_end']
    leaf_count = len([name for name in re.sub(r'\s|//[^\n]*', '', re.search(r'enum Branch : unsigned \{(.*?)\};', HEADER.read_text(), re.S)[1]).split(',') if name and name != 'branch_count'])
    coverage = result['coverage'] | verify['coverage']
    hit = {name for index, name in enumerate(names) if coverage >> (leaf_count + index) & 1}
    missing = sorted(set(names) - hit)
    report.update(release=result, census=census, verify=verify, branches=names, covered=sorted(hit), missing=missing)
    if missing:
        raise ValueError(f'Uncovered 303EE0 branches: {missing}')
    if census['compared'] == 0 or verify['compared'] == 0 or result['grown'] == 0:
        raise ValueError('Census, verify or the growth path never ran')
    if not args.skip_sanitizers:
        sanitized = compile_harness(scratch, 'array-sanitized', [], sanitizers=True)
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
    print(f"PASS {result['compared']} native 303EE0 comparisons ({result['ordered_stores']} ordered stores, "
          f"{result['faults']} faults, {result['grown']} growth stores), census {census['compared']}, "
          f"verify {verify['compared']}, {len(mutants)} mutants rejected")
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
