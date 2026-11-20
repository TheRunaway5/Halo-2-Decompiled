#!/usr/bin/env python3
"""Prove the native TU1.5 2F33B0 key sort: the logic in
native_tu15_world_leaves_systems.hpp (key_sort::sort, hkAlgorithm::
quickSortRecursive over unsigned keys) through its boundary adapter in
native_tu15_world_leaves.hpp (kernel_2f33b0, which runs its recursion inline).

Run through tools/run_test.py. Extracts the unmodified canonical translated
body run_2ef8f0 from the checked-in physics world unit (pinned by
tools/native_key_sort_replacements.py), its selected() operand case and the
binder's verification reference, and compiles
tests/native_tu15_key_sort_tests.cpp against them. Compares every seeded case
(ordered store events with preimages, bank, registers, flags, XMM, x87, MXCSR,
cursor; faults by exception, cursor and bank) in Release, census and verify
(record/replay) modes and an ASan/UBSan build, and requires every branch.
Checks the production binding (install, idempotence, refusal of a changed
body or operand). Then screens deliberate defects in the logic and the
adapter: each mutant is an out-of-tree copy of one header placed first on the
include path and must be rejected in release or verify mode. Source hashes
are rechecked after the screen.
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
TEST = ROOT / 'tests/native_tu15_key_sort_tests.cpp'
HELPER = ROOT / 'tools/native_key_sort_replacements.py'
MUTANTS = [
    (SYSTEMS, 'sort-middle-rounding', 'const auto middle=std::uint32_t(std::int32_t(low+high)>>1);',
     'const auto middle=std::uint32_t(std::int32_t(low+high+1u)>>1);'),
    (SYSTEMS, 'sort-scan-up', 'if(memory.load32(scan_up_first,key(j))<pivot)', 'if(memory.load32(scan_up_first,key(j))<=pivot)'),
    (SYSTEMS, 'sort-scan-down', 'do --i; while(pivot<memory.load32(scan_down,key(i)));', 'do --i; while(pivot<=memory.load32(scan_down,key(i)));'),
    (SYSTEMS, 'sort-break', 'if(std::int32_t(i)<std::int32_t(j))break;', 'if(std::int32_t(i)<=std::int32_t(j))break;'),
    (SYSTEMS, 'sort-same-swap', 'if(i!=j) {', 'if(true) {'),
    (SYSTEMS, 'sort-recursion-test', 'if(std::int32_t(low)<std::int32_t(i))', 'if(std::int32_t(low)<=std::int32_t(i))'),
    (SYSTEMS, 'sort-tail-test', 'if(!(std::int32_t(j)<std::int32_t(high)))return;', 'if(!(std::int32_t(j)<=std::int32_t(high)))return;'),
    (SYSTEMS, 'sort-tail-low', 'memory.store32(low_store,local.low,low);', 'memory.store32(low_store,local.low,low+1u);'),
    (SYSTEMS, 'sort-swap-values', 'memory.store32(swap_high_store,key(i),at_j);', 'memory.store32(swap_high_store,key(i),at_i);'),
    (SYSTEMS, 'sort-signed-scan', 'while(std::int32_t(j)<=std::int32_t(i));', 'while(j<=i);'),
    (HEADER, 'sort-recursion-pushes', 'port.push(0x2f340cu,reg(eax));port.push(0x2f340du,reg(edx));',
     'port.push(0x2f340cu,reg(edx));port.push(0x2f340du,reg(eax));'),
    (HEADER, 'sort-entry-slot', 'case low_argument:port.push(0x2f33b6u,reg(ecx));break;', 'case low_argument:port.push(0x2f33b6u,reg(edx));break;'),
    (HEADER, 'sort-exit-flags', 'port.flags.minus(gp[edi],gp[ebx],4);  // CMP EDI,EBX', 'port.flags.minus(gp[ebx],gp[edi],4);'),
    (HEADER, 'sort-partition-edi', 'reg(ecx)=pivot;reg(eax)=i;reg(edi)=j;', 'reg(ecx)=pivot;reg(eax)=i;reg(edi)=i;'),
    (HEADER, 'sort-recursion-cleanup', 'reg(esp)=port.flags.plus(reg(esp),0x10u,4);', 'reg(esp)=port.flags.plus(reg(esp),0xcu,4);'),
    (HEADER, 'sort-saved-edi', 'case keys_argument:reg(esi)=value;port.push(0x2f33c2u,reg(edi));break;',
     'case keys_argument:reg(esi)=value;port.push(0x2f33c2u,reg(esi));break;'),
    (HEADER, 'sort-pivot-register', 'reg(ecx)=pivot;reg(eax)=i;', 'reg(eax)=i;'),
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
    module = load(HELPER, 'native_key_sort_replacements')
    if not module.admitted(text):
        raise ValueError('Canonical 2F33B0 body or operand drifted from the pin')
    (scratch / 'sort_bodies.inc').write_text(f'static void run_{module.NAME}(ObservedWorldState &);\n' + module.body(text, module.NAME)[0] + '\n')
    (scratch / 'sort_reference.inc').write_text(module.reference(module.body(text, module.NAME)))
    (scratch / 'sort_selected.inc').write_text(''.join(f'case 0x{a:x}u:return 0x{b:x}u;\n' for a, b in sorted(module.OPERANDS.items())))
    # Production binding: installs, idempotent, refuses a changed body or a
    # changed operand mapping.
    output, installed = module.install(text, text)
    if installed != [module.ENTRY] or module.prefix() not in output or module.REFERENCE not in output:
        raise ValueError('2F33B0 binding did not install')
    if module.install(text, output)[1]:
        raise ValueError('2F33B0 binding is not idempotent')
    found = module.body(text, module.NAME)
    changed = text.replace(found[0], found[0].replace('0xfffffff0u', '0xfffffff8u', 1), 1)
    if changed == text or module.install(changed, changed)[1]:
        raise ValueError('2F33B0 binding accepted a changed body')
    remapped = text.replace('case 0x2ef954u:return 0x2f3414u;', 'case 0x2ef954u:return 0x2f3418u;', 1)
    if remapped == text or module.install(remapped, remapped)[1]:
        raise ValueError('2F33B0 binding accepted a changed operand mapping')
    report = dict(passed=False, unit=str(UNIT.relative_to(ROOT)), pin=module.PIN)
    release = compile_harness(scratch, 'sort-release', [])
    result = run(release, args.cases)
    census = run(release, max(args.cases // 4, 1000), 'census')
    verify = run(release, max(args.cases // 4, 1000), 'verify')
    # The phantom/world composition's canonical-PC remap (release, census, verify).
    report['remap'] = remap_sites.run_modes(run, release, max(args.cases // 4, 1000))
    header = HEADER.read_text()
    offset = sum(len(enum_names(header, name)) - 1 for name in
                 ('Branch', 'ArrayBranch', 'LocalBufferBranch', 'CollectionAgentBranch', 'ReportBranch', 'WeldBranch'))
    branches = [n for n in enum_names(header, 'SortBranch') if n != 'sort_branch_end']
    coverage = result['coverage'] | verify['coverage']
    hit = {name for index, name in enumerate(branches) if coverage >> (offset + index) & 1}
    missing = sorted(set(branches) - hit)
    report.update(release=result, census=census, verify=verify, branches=branches, covered=sorted(hit), missing=missing)
    if missing:
        raise ValueError(f'Uncovered 2F33B0 branches: {missing}')
    if census['compared'] == 0 or verify['compared'] == 0:
        raise ValueError('Census or verify never ran')
    if not args.skip_sanitizers:
        sanitized = compile_harness(scratch, 'sort-sanitized', [], sanitizers=True)
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
    print(f"PASS {result['compared']} native 2F33B0 comparisons ({result['ordered_stores']} ordered stores, "
          f"{result['faults']} faults), census {census['compared']}, verify {verify['compared']}, "
          f"{len(mutants)} mutants rejected")
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
