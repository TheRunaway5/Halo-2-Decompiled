#!/usr/bin/env python3
"""Prove the native TU1.5 1F76A0 shape key filter: the logic in
native_tu15_world_leaves_systems.hpp (shape_key_filter::may_collide) through
its boundary adapter in native_tu15_world_leaves.hpp (kernel_1f76a0, which
owns the jump-table read and the 185AA0 dispatch call).

Run through tools/run_test.py. Extracts the unmodified canonical translated
body run_1f2db0 from the checked-in physics world unit (pinned by
tools/native_shape_key_filter_replacements.py), its selected() operand cases,
the unit's canonical_branch() and sign_extend(), and the binder's verification
reference, and compiles tests/native_tu15_shape_key_filter_tests.cpp against
them. Compares every seeded case (ordered store events with preimages, bank,
registers, flags, XMM, x87, MXCSR, cursor; faults by exception, cursor and
bank) in Release, census and verify (record/replay with the child boundary)
modes and an ASan/UBSan build, and requires every branch. Checks the
production binding (install, idempotence, refusal of a changed body, operand,
jump target or dispatch site). Then screens deliberate defects in the logic
and the adapter (out-of-tree header copies placed first on the include path;
each must be rejected in release or verify mode). Source hashes are rechecked
after the screen.
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
TEST = ROOT / 'tests/native_tu15_shape_key_filter_tests.cpp'
HELPER = ROOT / 'tools/native_shape_key_filter_replacements.py'
MUTANTS = [
    (SYSTEMS, 'filter-definition-sign', 'const auto definition=std::uint32_t(std::int32_t(std::int16_t(memory.load16(definition_index,',
     'const auto definition=std::uint32_t(std::int32_t(std::uint16_t(memory.load16(definition_index,'),
    (SYSTEMS, 'filter-instance-stride', 'inline constexpr std::uint32_t instance_size=0x58u,', 'inline constexpr std::uint32_t instance_size=0x50u,'),
    (SYSTEMS, 'filter-bit-word', 'const auto word=(breakable>>5)+set*8u;', 'const auto word=(breakable>>5)+set*4u;'),
    (SYSTEMS, 'filter-bit-mask', '&(1u<<(breakable&31u)))!=0};', '&(1u<<(breakable&15u)))!=0};'),
    (SYSTEMS, 'filter-bit-offset', 'memory.load32(breakable_word,bits+word*4u+1u)', 'memory.load32(breakable_word,bits+word*4u)'),
    (SYSTEMS, 'filter-missing-object', 'if(handle==no_object)return {kind,false};', 'if(handle==no_object+1u)return {kind,false};'),
    (SYSTEMS, 'filter-excluded', '&excluded_flag)return {kind,false};', '&breakable_flag)return {kind,false};'),
    (SYSTEMS, 'filter-permutation', 'return {kind,permutation==std::int32_t((key.secondary>>5)&0xffu)};',
     'return {kind,permutation==std::int32_t((key.secondary>>4)&0xffu)};'),
    (SYSTEMS, 'filter-region-sign', 'const auto permutation=std::int32_t(std::int8_t(', 'const auto permutation=std::int32_t(std::uint8_t('),
    (SYSTEMS, 'filter-intact', 'return {kind,frame.breakable_surface_intact(', 'return {kind,!frame.breakable_surface_intact('),
    (SYSTEMS, 'filter-secondary', 'return {key>>29,(key>>16)&0x1fffu,key&0xffffu};', 'return {key>>29,(key>>16)&0xfffu,key&0xffffu};'),
    (HEADER, 'filter-table-entry', 'port.read(0x1f76bau,filter_table+reg(edx)*4u,4);', 'port.read(0x1f76bau,filter_table+reg(edx)*4u+4u,4);'),
    (HEADER, 'filter-unknown-target', '        children.canonical(target);  // throws',
     '        (void)target;  // throws'),
    (HEADER, 'filter-fail-flags', 'gp[ebx]=(gp[ebx]&~0xffu)|flags.logic(0u,1);', 'gp[ebx]&=~0xffu;'),
    (HEADER, 'filter-fail-exit', 'result.kind==Kind::instanced_surface?0x1f770au:', 'result.kind==Kind::instanced_surface?0x1f77a5u:'),
    (HEADER, 'filter-pop-edi', 'reg(edi)=port.pop(0x1f76f2u);', 'reg(edi)=port.read(0x1f76f2u,reg(esp),4);'),
    (HEADER, 'filter-call-flags', 'port.flags.logic(reg(eax)&0xffu,1);  // TEST AL,AL', '(void)0;'),
    (HEADER, 'filter-region-register', 'reg(ecx)=std::uint32_t(std::int32_t(std::int8_t(value)));', 'reg(ecx)=value;'),
    (HEADER, 'filter-index-mask', 'gp[ecx]=flags.logic(gp[ecx]&0xffffu,4);', 'gp[ecx]=flags.logic(gp[ecx],4);'),
    (HEADER, 'filter-result-byte', 'gp[eax]=(gp[eax]&~0xffu)|(gp[ebx]&0xffu);', 'gp[eax]=gp[ebx]&0xffu;'),
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
    sources = [UNIT, HEADER, SYSTEMS, TEST, HELPER, Path(__file__).resolve()]
    before = {str(path.relative_to(ROOT)): digest(path) for path in sources}
    text = UNIT.read_text()
    module = load(HELPER, 'native_shape_key_filter_replacements')
    if not module.admitted(text):
        raise ValueError('Canonical 1F76A0 body, operands, jump targets or site drifted from the pins')
    (scratch / 'filter_bodies.inc').write_text(f'static void run_{module.NAME}(ObservedWorldState &);\n' + module.body(text, module.NAME)[0] + '\n')
    (scratch / 'filter_reference.inc').write_text(module.reference(module.body(text, module.NAME)))
    (scratch / 'filter_selected.inc').write_text(''.join(f'case 0x{a:x}u:return 0x{b:x}u;\n' for a, b in sorted(module.OPERANDS.items())))
    branch = re.search(r'std::uint32_t canonical_branch\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}', text, re.S)[0]
    extend = re.search(r'std::uint32_t sign_extend\(std::uint32_t x,unsigned n\) \{.*?\n\}', text, re.S)[0]
    (scratch / 'filter_branch.inc').write_text(extend + '\n' + branch + '\n')
    # Production binding: installs, idempotent, refuses a changed body, operand
    # mapping, jump target or dispatch site.
    output, installed = module.install(text, text)
    if installed != [module.ENTRY] or module.prefix() not in output or module.REFERENCE not in output:
        raise ValueError('1F76A0 binding did not install')
    if module.install(text, output)[1]:
        raise ValueError('1F76A0 binding is not idempotent')
    found = module.body(text, module.NAME)
    for old, new, what in ((found[0], found[0].replace('0x1fffu', '0x0fffu', 1), 'body'),
                           ('case 0x4e7918u:return 0x4ecc18u;', 'case 0x4e7918u:return 0x4ecc1cu;', 'operand mapping'),
                           ('case 0x1f76c1u:return 0x1f2dd1u;', 'case 0x1f76c1u:return 0x1f2dd5u;', 'jump target'),
                           ('dispatch(s,target,0x1f2e0du,false)', 'dispatch(s,target,0x1f2e0eu,false)', 'dispatch site')):
        changed = text.replace(old, new, 1)
        if changed == text or module.install(changed, changed)[1]:
            raise ValueError(f'1F76A0 binding accepted a changed {what}')
    report = dict(passed=False, unit=str(UNIT.relative_to(ROOT)), pin=module.PIN)
    release = compile_harness(scratch, 'filter-release', [])
    result = run(release, args.cases)
    census = run(release, max(args.cases // 4, 1000), 'census')
    verify = run(release, max(args.cases // 4, 1000), 'verify')
    header = HEADER.read_text()
    offset = sum(len(enum_names(header, name)) - 1 for name in
                 ('Branch', 'ArrayBranch', 'LocalBufferBranch', 'CollectionAgentBranch', 'ReportBranch', 'WeldBranch', 'SortBranch'))
    branches = [n for n in enum_names(header, 'FilterBranch') if n != 'filter_branch_end']
    coverage = result['coverage'] | verify['coverage']
    hit = {name for index, name in enumerate(branches) if coverage >> (offset + index) & 1}
    missing = sorted(set(branches) - hit)
    report.update(release=result, census=census, verify=verify, branches=branches, covered=sorted(hit), missing=missing)
    if missing:
        raise ValueError(f'Uncovered 1F76A0 branches: {missing}')
    if census['compared'] == 0 or verify['compared'] == 0:
        raise ValueError('Census or verify never ran')
    if not args.skip_sanitizers:
        sanitized = compile_harness(scratch, 'filter-sanitized', [], sanitizers=True)
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
    print(f"PASS {result['compared']} native 1F76A0 comparisons ({result['ordered_stores']} ordered stores, "
          f"{result['faults']} faults), census {census['compared']}, verify {verify['compared']}, "
          f"{len(mutants)} mutants rejected")
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
