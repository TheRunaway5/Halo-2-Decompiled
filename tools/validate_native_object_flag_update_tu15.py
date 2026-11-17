#!/usr/bin/env python3
"""Prove the native TU1.5 E8D80 object flag update (an entry of the vehicle
update unit): the logic in native_tu15_world_leaves_systems.hpp
(object_flag_update::update) through its boundary adapter in
native_tu15_world_leaves.hpp (kernel_e8d80).

Run through tools/run_test.py. Extracts the unmodified canonical translated
body (run_e7e60; pinned by tools/native_object_flag_update_replacements.py), their selected() operand cases
and the binder's verification reference, and compiles
tests/native_tu15_object_flag_update_tests.cpp against them. Compares every
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
UNIT = ROOT / 'src/physics/vehicles/physics_vehicle_update_generated.cpp'
HEADER = ROOT / 'src/physics/runtime/native_tu15_world_leaves.hpp'
SYSTEMS = ROOT / 'src/physics/runtime/native_tu15_world_leaves_systems.hpp'
TEST = ROOT / 'tests/native_tu15_object_flag_update_tests.cpp'
HELPER = ROOT / 'tools/native_object_flag_update_replacements.py'
MUTANTS = [
    # logic
    (SYSTEMS, 'flags-count', 'inline constexpr std::uint32_t flag_count=0x3cu;', 'inline constexpr std::uint32_t flag_count=0x3bu;'),
    (SYSTEMS, 'flags-block-offset', 'inline constexpr std::uint32_t flag_block_offset=0x346u;', 'inline constexpr std::uint32_t flag_block_offset=0x344u;'),
    (SYSTEMS, 'flags-block-sign', 'const auto block=object+std::uint32_t(std::int32_t(std::int16_t(memory.load16(block_offset,',
     'const auto block=object+std::uint32_t(std::int32_t(std::uint16_t(memory.load16(block_offset,'),
    (SYSTEMS, 'flags-handle-index', 'inline constexpr std::uint32_t handle_index=0xffffu;', 'inline constexpr std::uint32_t handle_index=0x7fffu;'),
    (SYSTEMS, 'flags-word-index', 'const auto word=at(block,flag_words+(flag>>5)*4u);', 'const auto word=at(block,flag_words+(flag>>4)*4u);'),
    (SYSTEMS, 'flags-bit', 'const auto bit=1u<<(flag&31u);', 'const auto bit=1u<<(flag&15u);'),
    (SYSTEMS, 'flags-function-offset', 'inline constexpr std::uint32_t update_function=0x4u;      // in a flag', 'inline constexpr std::uint32_t update_function=0x0u;      // in a flag'),
    (SYSTEMS, 'flags-no-function', '        if(!update_address)continue;\n', ''),
    (SYSTEMS, 'flags-reread', 'else memory.store32(flag_clear,word,memory.load32(flag_word_again,word)&~bit);',
     'else {memory.load32(flag_word_again,word);memory.store32(flag_clear,word,memory.load32(flag_word,word)&~bit);}'),
    (SYSTEMS, 'flags-clear-mask', 'memory.load32(flag_word_again,word)&~bit);', 'memory.load32(flag_word_again,word)^bit);'),
    (SYSTEMS, 'flags-result-set', 'memory.store8(result_set,local.result,1u);', 'memory.store8(result_set,local.result,2u);'),
    (SYSTEMS, 'flags-header-object', 'offsetof(ObjectHeader,object)));\n    const auto block=', 'offsetof(ObjectHeader,flags)));\n    const auto block='),
    # adapter
    (HEADER, 'flags-save-order', 'port.push(0xe7e82u,reg(ebp));port.push(0xe7e83u,reg(esi));', 'port.push(0xe7e82u,reg(esi));port.push(0xe7e83u,reg(ebp));'),
    (HEADER, 'flags-argument-order', 'port.push(0xe7ebcu,reg(ebp));port.push(0xe7ebdu,reg(ecx));', 'port.push(0xe7ebcu,reg(ecx));port.push(0xe7ebdu,reg(ebp));'),
    (HEADER, 'flags-return', 'inline constexpr std::uint32_t flag_update_site=0xe7ebeu,flag_update_return=0xe8de0u;',
     'inline constexpr std::uint32_t flag_update_site=0xe7ebeu,flag_update_return=0xe8de1u;'),
    (HEADER, 'flags-site', 'inline constexpr std::uint32_t flag_update_site=0xe7ebeu,', 'inline constexpr std::uint32_t flag_update_site=0xe7ebfu,'),
    (HEADER, 'flags-exit-count', 'case result_read:next_flag();break;', 'case result_read:break;'),
    (HEADER, 'flags-al', 'case result_read:reg(eax)=(reg(eax)&0xffffff00u)|(value&0xffu);break;', 'case result_read:reg(eax)=value&0xffu;break;'),
    (HEADER, 'flags-edx', '        case headers_data:reg(edx)=value;break;\n', ''),
    (HEADER, 'flags-esi-shift', 'reg(esi)=flags.shift(reg(esi),2u,4);', 'reg(esi)=flags.shift(reg(esi),3u,4);'),
    (HEADER, 'flags-ret', 'port.ret(0xe7ee6u,4u);port.publish(0xe7ee6u);', 'port.ret(0xe7ee6u,8u);port.publish(0xe7ee6u);'),
    (HEADER, 'flags-keep-al', 'port.flags.logic(reg(eax)&0xffu,1);\n        return (reg(eax)&0xffu)!=0u;', 'port.flags.logic(reg(eax)&0xffu,1);\n        return (reg(eax)&0xfffu)!=0u;'),
]
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
        raise ValueError('Canonical E8D80 body or operands drifted from the pins')
    # The unit's sign_extend() (MOVSX), which the translated body calls.
    extend = re.search(r'std::uint32_t sign_extend\(std::uint32_t x,unsigned n\) \{.*?\n\}', text, re.S)[0]
    (directory / 'flags_bodies.inc').write_text(
        extend + '\n' + f'static void run_{module.NAME}(VehicleState &);\n' + module.body(text, module.NAME)[0] + '\n')
    (directory / 'flags_reference.inc').write_text(module.reference(module.body(text, module.NAME)))
    operands = module.OPERANDS
    (directory / 'flags_selected.inc').write_text(''.join(f'case 0x{a:x}u:return 0x{b:x}u;\n' for a, b in sorted(operands.items())))


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
    module = load(HELPER, 'native_object_flag_update_replacements')
    generate(module, text, scratch)
    # Production binding: installs, idempotent, refuses a changed body, a
    # changed operand mapping or a changed dispatch site.
    output, installed = module.install(text, text)
    if installed != [module.ENTRY] or module.prefix() not in output or module.REFERENCE not in output:
        raise ValueError(f'E8D80 binding installed {installed}')
    if module.install(text, output)[1]:
        raise ValueError('E8D80 binding is not idempotent')
    found = module.body(text, module.NAME)
    changed = text.replace(found[0], found[0].replace('0x3cu', '0x3du', 1), 1)
    if changed == text or module.install(changed, changed)[1]:
        raise ValueError('E8D80 binding accepted a changed body')
    operand = sorted(module.OPERANDS)[0]
    remapped = text.replace(f'case 0x{operand:x}u:return 0x{module.OPERANDS[operand]:x}u;',
                            f'case 0x{operand:x}u:return 0x{module.OPERANDS[operand] + 4:x}u;', 1)
    if remapped == text or module.install(remapped, remapped)[1]:
        raise ValueError('E8D80 binding accepted a changed operand mapping')
    site = module.SITES[0]
    moved = text.replace(found[0], found[0].replace(f'0x{site:x}u,false)', f'0x{site + 1:x}u,false)', 1), 1)
    if moved == text or module.install(moved, moved)[1]:
        raise ValueError('E8D80 binding accepted a changed dispatch site')
    report = dict(passed=False, unit=str(UNIT.relative_to(ROOT)), pin=module.PIN)
    release = compile_harness(scratch, 'flags-release', [])
    result = run(release, args.cases)
    census = run(release, max(args.cases // 4, 1000), 'census')
    verify = run(release, max(args.cases // 4, 1000), 'verify')
    header = HEADER.read_text()
    # The bits after the earlier word-1 owners' in coverage word 1.
    offset = 64 + sum(len(enum_names(header, name)) - 1 for name in ('ClosestPointBranch', 'SweptPairBranch', 'ChildShapeBranch', 'AabbQueryBranch', 'ChildAgentBranch'))
    branches = [n for n in enum_names(header, 'FlagUpdateBranch') if n != 'flag_update_branch_end']
    coverage = (result['coverage'] | result['coverage_high'] << 64 | result['coverage_top'] << 128) | \
        (verify['coverage'] | verify['coverage_high'] << 64 | verify['coverage_top'] << 128)
    hit = {name for index, name in enumerate(branches) if coverage >> (offset + index) & 1}
    missing = sorted(set(branches) - hit)
    report.update(release=result, census=census, verify=verify, branches=branches, covered=sorted(hit), missing=missing)
    if missing:
        raise ValueError(f'Uncovered E8D80 branches: {missing}')
    if census['compared'] == 0 or verify['compared'] == 0 or result['child_calls'] == 0:
        raise ValueError('Census, verify or the children never ran')
    if not args.skip_sanitizers:
        sanitized = compile_harness(scratch, 'flags-sanitized', [], sanitizers=True)
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
    print(f"PASS {result['compared']} native E8D80 comparisons ({result['ordered_stores']} ordered stores, "
          f"{result['faults']} faults), census {census['compared']}, verify {verify['compared']}, "
          f"{len(mutants)} mutants rejected")
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
