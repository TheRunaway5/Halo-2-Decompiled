#!/usr/bin/env python3
"""Prove the native TU1.5 point/segment distance leaf (14DD70, scene device).

Run through tools/run_test.py. Extracts the unmodified translated body from
the checked-in scene device unit (pinned by the specializer's LEAVES entry),
in its production cursor form, compiles
tests/native_tu15_point_segment_distance_tests.cpp against it and compares
every seeded case (ordered store events with preimages, bank, registers,
flags, XMM, x87 status including C1/TOP/sticky flags, x87 lanes, control
word, MXCSR, cursor; faults by exception, cursor and store prefix) in
Release and ASan/UBSan builds and in the shared leaf verify mode. Requires
both exits and C1 in both states. Then screens deliberate defects: each mutant
is an out-of-tree copy of the leaf header placed first on the include path and
must be rejected. The production binding must install with its x87 admission,
be idempotent, and refuse a changed body or a nonidentity selected operand.
Source hashes are rechecked after the screen.
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
UNIT = ROOT / 'src/rendering/renderer/source_scene_device_constructor_tu15_generated.cpp'
HEADER = ROOT / 'src/physics/runtime/native_tu15_matrix_leaves.hpp'
TEST = ROOT / 'tests/native_tu15_point_segment_distance_tests.cpp'
SPECIALIZER = ROOT / 'tools/specialize_native_operands.py'
INSTRUCTION = re.compile(r'if\(s\.services\.instruction\)\{s\.services\.instruction\((0x[0-9a-f]+u)\);\}')
MUTANTS = [
    ('sub-esp', 'gp[4]=m.flags.minus(gp[4],8u,4);\n    x[2]=load(io.rd(0x14dd73u', 'gp[4]=m.flags.minus(gp[4],12u,4);\n    x[2]=load(io.rd(0x14dd73u'),
    ('first-difference', 'load(io.rd(0x14dd78u,gp[1]+0xcu,4))', 'load(io.rd(0x14dd78u,gp[1]+0x10u,4))'),
    ('row-shift', 'gp[0]=m.flags.shift(gp[0],4u,4);', 'gp[0]=m.flags.shift(gp[0],3u,4);'),
    ('projection-sum', 'x[3]=ordered_sum_ss(x[3],x[5]);\n    io.st(0x14de0du', 'x[3]=ordered_difference_ss(x[3],x[5]);\n    io.st(0x14de0du'),
    ('spill-t', 'io.st(0x14de0du,gp[4]+8u,4,low_bits(x[3]));', 'io.st(0x14de0du,gp[4]+4u,4,low_bits(x[3]));'),
    ('compare-operands', 'm.flags.scalar_compare(x[2],x[0],false);\n    gp[7]=pop(io,m,0x14de40u);',
     'm.flags.scalar_compare(x[0],x[2],false);\n    gp[7]=pop(io,m,0x14de40u);'),
    ('spill-d', 'io.st(0x14de41u,gp[4]+8u,4,low_bits(x[0]));', 'io.st(0x14de41u,gp[4]+8u,4,low_bits(x[2]));'),
    ('near-branch', 'if(!m.flags.cf) {\n        const auto t=io.rd(0x14de4au', 'if(m.flags.cf) {\n        const auto t=io.rd(0x14de4au'),
    ('fabs', '"flds %2\\n\\tfabs\\n\\tfstps %0', '"flds %2\\n\\tfchs\\n\\tfstps %0'),
    ('far-subtract', '"flds %3\\n\\tfsubr %%st(1),%%st', '"flds %3\\n\\tfsub %%st(1),%%st'),
    ('second-radius', 'r2=io.rd(0x14de68u,gp[1]+0x14u,4)', 'r2=io.rd(0x14de68u,gp[1]+0x10u,4)'),
    ('far-sqrt', 'faddp\\n\\tfsqrt\\n\\tfstps', 'faddp\\n\\tfstps'),
    ('far-square', '"flds %5\\n\\tflds %6\\n\\tfmulp\\n\\tfaddp', '"flds %5\\n\\tflds %6\\n\\tfaddp\\n\\tfaddp'),
    ('c1', 'm.clear_x87_c1=true;m.x87_c1=status&0x200u;', 'm.clear_x87_c1=true;m.x87_c1=0;'),
    ('exit-flags', 'gp[4]=m.flags.plus(gp[4],8u,4);\n    pop(io,m,near', 'm.flags.logic(gp[4],4);gp[4]+=8u;\n    pop(io,m,near'),
    ('exit-cursor', 'pop(io,m,near?0x14de5au:0x14de82u);', 'pop(io,m,near?0x14de5au:0x14de7fu);'),
    ('push-register', 'push(io,m,0x14dd90u,gp[6]);', 'push(io,m,0x14dd90u,gp[7]);'),
    ('restore-edi', 'gp[7]=pop(io,m,0x14de40u);', 'gp[6]=pop(io,m,0x14de40u);'),
    ('difference-z', 'x[3]=ordered_difference_ss(x[3],x[6]);', 'x[3]=ordered_difference_ss(x[3],x[5]);'),
    ('room-depth', 'if(s.fp.count+depth>8u)return false;', 'if(s.fp.count+depth>9u)return false;'),
    ('room-mmx', 'if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;\n    if(s.fp.count+depth>8u)',
     'if(s.fp.count+depth>8u)'),
    ('room-control', 'return (control&0x3fu)==0x3fu;\n#else\n    return false;', 'return true;\n#else\n    return false;'),
]


def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def specializer():
    spec = importlib.util.spec_from_file_location('native_operands', SPECIALIZER)
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
    parser.add_argument('--cases', type=int, default=20000)
    parser.add_argument('--mutant-cases', type=int, default=4000)
    parser.add_argument('--jobs', type=int, default=4)
    parser.add_argument('--skip-sanitizers', action='store_true')
    args = parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):
        parser.error('Run through tools/run_test.py')
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    sources = [UNIT, HEADER, TEST, SPECIALIZER, Path(__file__).resolve()]
    before = {str(path.relative_to(ROOT)): digest(path) for path in sources}
    module = specializer()
    text = UNIT.read_text()
    found = module.leaf_body(text, '14dd70')
    pin = module.LEAVES['14dd70'][0]
    if module.digest(found[0].replace(found[1] + ' &s', 'STATE &s')) != pin:
        raise ValueError('Canonical 14DD70 body drifted from its pin')
    # Production binding through the real specializer.
    output, receipt = module.specialize(text)
    if '14dd70' not in receipt['native_leaves'] or 'halo2::native_tu15_leaves::x87_room(s,2u) && ' not in output:
        raise ValueError('14DD70 leaf binding did not install with its x87 admission')
    if output.count('// BEGIN native leaf14dd70') != 1 or module.specialize(output)[0] != output:
        raise ValueError('14DD70 leaf binding is not idempotent')
    changed = text.replace(found[0], found[0].replace('0x14u', '0x18u', 1), 1)
    if changed == text or '14dd70' in module.install_leaves(changed, changed)[1]:
        raise ValueError('14DD70 leaf binding accepted a changed body')
    operand = re.search(r'selected\((0x[0-9a-f]+)u,s\.image\)', found[0])[1]
    case = f'case {operand}u:return {operand}u;'
    remapped = text.replace(case, f'case {operand}u:return {hex(int(operand, 16) + 4)}u;', 1)
    if remapped == text or '14dd70' in module.install_leaves(remapped, remapped)[1]:
        raise ValueError('14DD70 leaf binding accepted a nonidentity selected operand')
    body = INSTRUCTION.sub(r's.instruction_literal(\1);', found[0])
    (scratch / 'point_segment_body.inc').write_text('static void run_14dd70(WorldState &);\n' + body + '\n')
    report = dict(passed=False, unit=str(UNIT.relative_to(ROOT)), pin=pin)
    release = compile_harness(scratch, 'segment-release', [])
    result = run(release, args.cases)
    verify = run(release, max(args.cases // 6, 1000), 'verify')
    report.update(release=result, verify=verify)
    if not (result['near'] and result['far'] and result['c1_set'] and result['faults'] and result['far'] + result['near'] > result['c1_set']):
        raise ValueError(f'Incomplete 14DD70 coverage: {result}')
    if not args.skip_sanitizers:
        sanitized = compile_harness(scratch, 'segment-sanitized', [], sanitizers=True)
        report['sanitized'] = run(sanitized, max(args.cases // 4, 2000))
        sanitized.unlink()
    release.unlink()
    header_text = HEADER.read_text()

    def screen(mutant):
        name, old, new = mutant
        if header_text.count(old) != 1:
            return dict(name=name, rejected=False, error='mutation anchor missing')
        directory = scratch / 'segment-mutants' / name / 'physics/runtime'
        directory.mkdir(parents=True)
        (directory / HEADER.name).write_text(header_text.replace(old, new, 1))
        try:
            binary = compile_harness(scratch, f'segment-mutant-{name}', [scratch / 'segment-mutants' / name])
        except subprocess.CalledProcessError:
            return dict(name=name, rejected=False, error='mutant failed to compile')
        outcome = subprocess.run([str(binary), str(args.mutant_cases)], capture_output=True, text=True)
        binary.unlink()
        return dict(name=name, rejected=outcome.returncode != 0, diagnostic=outcome.stderr.strip()[-200:])
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
    print(f"PASS {result['compared']} native 14DD70 comparisons ({result['near']} near, {result['far']} far, "
          f"{result['c1_set']} C1 set, {result['faults']} faults), verify {verify['compared']}, {len(mutants)} mutants rejected")
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
