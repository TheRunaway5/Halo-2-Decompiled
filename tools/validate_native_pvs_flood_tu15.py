#!/usr/bin/env python3
"""Prove the native TU1.5 PVS closure (1532A0/170C80/170FB0/44680).

Run through tools/run_test.py. Extracts the four unmodified translated bodies
from the checked-in live-support unit (pinned), compiles
tests/native_tu15_pvs_flood_tests.cpp against them, and compares every
seeded case (ordered store events, bank, registers, flags, cursor) in Release
and ASan/UBSan builds. Requires complete reachable branch coverage. Then
screens deliberate defects: each mutant is an out-of-tree copy of the native
header placed first on the include path and must be rejected. The production
binding (specializer prefix) must install, be idempotent, and refuse a
changed body. Source hashes are rechecked after the screen.
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
import sys

ROOT = Path(__file__).resolve().parents[1]
UNIT = ROOT / 'src/physics/physics_live_support_tu15_generated.cpp'
HEADER = ROOT / 'src/physics/runtime/native_tu15_pvs_flood.hpp'
TEST = ROOT / 'tests/native_tu15_pvs_flood_tests.cpp'
HELPER = ROOT / 'tools/native_pvs_flood_replacements.py'
# Unreachable by construction: scan_negative needs a 2^31-entry scan and
# copy_bytes a vector length that is not a multiple of four (all are W*4).
UNREACHABLE = {'scan_negative', 'copy_bytes'}
MUTANTS = [
    ('push-site', 'io.st(0x170c87u,entry-0xf8u,4,r.ebp);', 'io.st(0x170c86u,entry-0xf8u,4,r.ebp);'),
    ('portal-count-sign', 'if(si(portals)>0) {', 'if(si(portals)>=0) {'),
    ('iterator-clear-bit', 'const auto keep=~(1u<<(low&31u));', 'const auto keep=~(1u<<((low+1u)&31u));'),
    ('row-plane-bound', 'if(first*4u<size) {', 'if(first*4u<=size) {'),
    ('join-or', 'mine|io.rd(0x170eb7u', 'mine&io.rd(0x170eb7u'),
    ('limit-and', 'mine&io.rd(0x170edbu', 'mine|io.rd(0x170edbu'),
    ('visited-store', 'io.st(0x170f0fu,word,4,io.rd(0x170f0fu,word,4)|esi);', 'io.rd(0x170f0fu,word,4);'),
    ('fixed-point-compare', '{equal=false;break;}', '{break;}'),
    ('record-base', 'r.ecx=r.ecx*r.eax+r.edx;', 'r.ecx=r.ecx*r.eax;'),
    ('iterator-word', 'io.st(0x170d54u,frame+0x36u,2,0);', 'io.st(0x170d54u,frame+0x36u,2,1);'),
    ('or-word-count', '} while(--r.ecx);', '} while(--r.ecx && r.ecx!=1u);'),
    ('iterator-bound', 'if(std::int16_t(index)>=std::int16_t(count))break;', 'if(std::int16_t(index)>std::int16_t(count))break;'),
    ('require-portal-word', 'if((r.eax&0xffu) && io.rd(0x153311u', 'if(!(r.eax&0xffu) && io.rd(0x153311u'),
    ('exit-flags', 's.flags.plus(entry-0x40u,0x40u,4);\n    s.gp[4]', 's.flags.plus(entry-0x40u,0x41u,4);\n    s.gp[4]'),
    ('exit-stack', 's.gp[4]=entry+12u;', 's.gp[4]=entry+8u;'),
    ('exit-cursor', 'if(cursor)*cursor=0x1532f2u;', 'if(cursor)*cursor=0x1532eeu;'),
    ('visit-length', 'const auto visit_bytes=sar(io.rd(0x170d11u,bsp+0x5cu,4)+0x1fu,5)<<2;',
     'const auto visit_bytes=sar(io.rd(0x170d11u,bsp+0x5cu,4)+0x1eu,5)<<2;'),
    ('spill-order', 'io.st(0x170dc7u,frame+0x28u,4,mask);\n                            io.st(0x170dcbu,frame+0x1cu,4,word);',
     'io.st(0x170dcbu,frame+0x1cu,4,word);\n                            io.st(0x170dc7u,frame+0x28u,4,mask);'),
    ('second-row-select', 'if(again_plane*4u<size)', 'if(again_plane*4u<size+4u)'),
    ('saved-ebx', 'r.ebx=io.rd(0x1532eeu,frame+12u,4);', 'r.ebx=io.rd(0x1532eeu,frame+8u,4);'),
]


def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def helper():
    spec = importlib.util.spec_from_file_location('native_pvs_flood_replacements', HELPER)
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


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--report', type=Path, required=True)
    parser.add_argument('--cases', type=int, default=6000)
    parser.add_argument('--mutant-cases', type=int, default=1500)
    parser.add_argument('--jobs', type=int, default=4)
    parser.add_argument('--skip-sanitizers', action='store_true')
    args = parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):
        parser.error('Run through tools/run_test.py')
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    sources = [UNIT, HEADER, TEST, HELPER, Path(__file__).resolve()]
    before = {str(path.relative_to(ROOT)): digest(path) for path in sources}
    text = UNIT.read_text()
    module = helper()
    if not module.canonical_matches(text):
        raise ValueError('Canonical PVS closure bodies drifted from their pins')
    bodies = [module.body(text, name)[0] for name in ('1532a0', '170c80', '170fb0', '44680')]
    declarations = ''.join(f'static void run_{name}(WorldState &);\n' for name in ('1532a0', '170c80', '170fb0', '44680'))
    (scratch / 'pvs_flood_bodies.inc').write_text(declarations + '\n'.join(bodies) + '\n')
    # Production binding: installs, idempotent, refuses a changed body.
    output, installed = module.install(text, text)
    if installed != ['1532a0'] or module.PREFIX not in output:
        raise ValueError('PVS binding did not install')
    if module.install(text, output)[1]:
        raise ValueError('PVS binding is not idempotent')
    changed = text.replace(bodies[1], bodies[1].replace('0xb0u', '0xb4u', 1), 1)
    if changed == text or module.install(changed, changed)[1]:
        raise ValueError('PVS binding accepted a changed 170C80 body')
    report = dict(passed=False, unit=str(UNIT.relative_to(ROOT)), pins=module.PINS)
    release = compile_harness(scratch, 'pvs-release', [])
    result = json.loads(subprocess.run([str(release), str(args.cases)], check=True, capture_output=True, text=True).stdout)
    names = [name for name in re.search(r'enum Branch : unsigned \{(.*?)\};', HEADER.read_text(), re.S)[1].replace('\n', ' ').replace(' ', '').split(',') if name]
    names = [name for name in names if name != 'branch_count']
    hit = {name for index, name in enumerate(names) if result['coverage'] >> index & 1}
    missing = sorted(set(names) - hit - UNREACHABLE)
    report.update(release=result, branches=names, covered=sorted(hit), unreachable=sorted(UNREACHABLE), missing=missing)
    if missing:
        raise ValueError(f'Uncovered native PVS branches: {missing}')
    if not args.skip_sanitizers:
        sanitized = compile_harness(scratch, 'pvs-sanitized', [], sanitizers=True)
        report['sanitized'] = json.loads(subprocess.run([str(sanitized), str(max(args.cases // 4, 500))], check=True,
            capture_output=True, text=True).stdout)
    header_text = HEADER.read_text()

    def screen(mutant):
        name, old, new = mutant
        if header_text.count(old) != 1:
            return dict(name=name, rejected=False, error='mutation anchor missing')
        directory = scratch / 'mutants' / name / 'physics/runtime'
        directory.mkdir(parents=True)
        (directory / HEADER.name).write_text(header_text.replace(old, new, 1))
        try:
            binary = compile_harness(scratch, f'mutant-{name}', [scratch / 'mutants' / name])
        except subprocess.CalledProcessError:
            return dict(name=name, rejected=False, error='mutant failed to compile')
        run = subprocess.run([str(binary), str(args.mutant_cases)], capture_output=True, text=True)
        binary.unlink()
        return dict(name=name, rejected=run.returncode != 0, diagnostic=run.stderr.strip()[-200:])
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
    print(f"PASS {result['compared']} native PVS closure comparisons ({result['ordered_stores']} ordered stores), "
          f"{len(mutants)} mutants rejected")
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
