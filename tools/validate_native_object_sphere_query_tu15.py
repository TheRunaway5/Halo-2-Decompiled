#!/usr/bin/env python3
"""Prove the native TU1.5 object sphere query closure (BD080/BA970/BAA40).

Run through tools/run_test.py. Extracts the unmodified translated bodies of
all three pinned BD080 variants (live support, combat action = scene device,
AI frame) and their BA970/BAA40 bodies from the checked-in units, compiles
tests/native_tu15_object_sphere_query_tests.cpp against them and compares
every seeded case (ordered store events with preimages, bank, registers,
flags, XMM, MXCSR, cursor; faults by exception, cursor and store prefix) in
Release and ASan/UBSan builds, plus census-admission and verify-mode runs.
Requires complete reachable branch coverage. Then screens deliberate defects:
each mutant is an out-of-tree copy of the native header placed first on the
include path and must be rejected. The production binding (specializer
prefix) must install with the unit's own 14D0E0 route, be idempotent, and
refuse a changed body or a nonidentity selected operand. Source hashes are
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
UNITS = {
    'live': ROOT / 'src/physics/physics_live_support_tu15_generated.cpp',
    'combat': ROOT / 'src/gameplay/units/source_combat_action_tu15_generated.cpp',
    'scene': ROOT / 'src/rendering/renderer/source_scene_device_constructor_tu15_generated.cpp',
    'ai': ROOT / 'src/campaign/ai/source_ai_frame_tu15_generated.cpp',
}
EXTRACT = {'live': ('bd080', 'ba970', 'baa40'), 'combat': ('bd080', 'baa40'), 'ai': ('bd080', 'ba970')}
ROUTES = {'live': 'direct', 'combat': 'dispatch', 'scene': 'dispatch', 'ai': 'direct'}
HEADER = ROOT / 'src/physics/runtime/native_tu15_object_sphere_query.hpp'
TEST = ROOT / 'tests/native_tu15_object_sphere_query_tests.cpp'
HELPER = ROOT / 'tools/native_object_sphere_query_replacements.py'
UNREACHABLE = set()
MUTANTS = [
    ('push-site', 'st(0xbd08eu,E-0x428u,4,ebp);', 'st(0xbd08du,E-0x428u,4,ebp);'),
    ('type-default', 'st(0xbd09bu,E+8u,4,0xffffffffu);', 'st(0xbd09bu,E+8u,4,0x7fffffffu);'),
    ('cluster-none', 'if((eax&0xffffu)==0xffffu)', 'if((eax&0xffffu)==0xfffeu)'),
    ('radius-compare', 'if(flags.cf || flags.zf) {', 'if(flags.cf) {'),
    ('flood-clamp', 'if(s16(eax)>0x200)', 'if(s16(eax)>=0x200)'),
    ('child-flags', 'ecx=flags.increment(ecx,4,false);', 'ecx=ecx+1u;'),
    ('child-xmm', 'for(unsigned i=0;i<8;++i)s.xmm[i]=x[i];\n        flood(s);', 'for(unsigned i=0;i<4;++i)s.xmm[i]=x[i];\n        flood(s);'),
    ('stamp', 'edx=rd(0xbd13au,0x4eabccu)+1u;', 'edx=rd(0xbd13au,0x4eabccu)+2u;'),
    ('iterator-slot', 'ebx=F+0x20u;', 'ebx=F+0x24u;'),
    ('second-table', 'table=second?0x4eaba4u:0x4eabb0u', 'table=second?0x4eabb0u:0x4eabb0u'),
    ('payload-offset', 'ecx=eax+0xcu;\n        st(0xba9afu+o', 'ecx=eax+0x8u;\n        st(0xba9afu+o'),
    ('type-shift', 'esi=1u<<(ecx&31u);', 'esi=1u<<(ecx&15u);'),
    ('dedupe', '==esi)mark(seen_reject,second);', '==esi+1u)mark(seen_reject,second);'),
    ('sum-order', 'x[5]=add_ss(x[5],x[3]);\n            x[1]=x[0];\n            x[5]=add_ss(x[5],x[2]);',
     'x[5]=add_ss(x[5],x[2]);\n            x[1]=x[0];\n            x[5]=add_ss(x[5],x[3]);'),
    ('compare-operands', 'return comiss_below(x[1],x[5]);', 'return comiss_below(x[5],x[1]);'),
    ('capacity-compare', 'if(s16(ecx)>=s16(rd(0xbd252u+o,E+0x1cu,2)))', 'if(s16(ecx)>s16(rd(0xbd252u+o,E+0x1cu,2)))'),
    ('output-index', 'st(0xbd26bu+o,ebp+esi*4u,4,eax);', 'st(0xbd26bu+o,ebp+esi*4u+4u,4,eax);'),
    ('next-slot', 'slot=second?F+0x1cu:F+0x20u', 'slot=second?F+0x20u:F+0x20u'),
    ('second-list-bit', 'if(!(rd(0xbd2d0u,E+4u,1)&2u))', 'if(!(rd(0xbd2d0u,E+4u,1)&1u))'),
    ('cluster-bound', 'if(!(s16(eax)<s16(limit)))', 'if(!(s16(eax)<=s16(limit)))'),
    ('exit-flags', 's.flags.plus(E-0x420u,0x420u,4);', 's.flags.plus(E-0x420u,0x421u,4);'),
    ('exit-stack', 's.gp={eax,ecx,edx,ebx,E+0x20u,ebp,esi,edi};', 's.gp={eax,ecx,edx,ebx,E+0x1cu,ebp,esi,edi};'),
    ('exit-cursor', 'rd(0xbd440u,E);final_pc=0xbd440u;', 'rd(0xbd440u,E);final_pc=0xbd43au;'),
    ('full-count', 'eax=(eax&0xffff0000u)|(ecx&0xffffu);', 'eax=(eax&0xffff0000u)|((ecx+1u)&0xffffu);'),
    ('single-width', 'st(0xbd177u,F+0x30u,2,eax&0xffffu);', 'st(0xbd177u,F+0x30u,4,eax);'),
    ('restore-ebx', 'ebx=rd(0xbd439u,F+12u);', 'ebx=rd(0xbd439u,F+8u);'),
    ('second-squares', 'x[3]=mul_ss(x[1],x[1]);\n            x[1]=mul_ss(x[2],x[2]);',
     'x[3]=mul_ss(x[2],x[2]);\n            x[1]=mul_ss(x[1],x[1]);'),
    ('center-read-pc', 'rd(0xbd204u,edi+8u)', 'rd(0xbd205u,edi+8u)'),
]


def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def helper():
    spec = importlib.util.spec_from_file_location('native_object_sphere_query_replacements', HELPER)
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
    parser.add_argument('--cases', type=int, default=6000)
    parser.add_argument('--mutant-cases', type=int, default=1500)
    parser.add_argument('--jobs', type=int, default=4)
    parser.add_argument('--skip-sanitizers', action='store_true')
    args = parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):
        parser.error('Run through tools/run_test.py')
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    sources = [*UNITS.values(), HEADER, TEST, HELPER, Path(__file__).resolve()]
    before = {str(path.relative_to(ROOT)): digest(path) for path in sources}
    module = helper()
    texts = {name: path.read_text() for name, path in UNITS.items()}
    # Production binding: each unit installs with its own 14D0E0 route,
    # idempotently, and refuses changed bodies and nonidentity operands.
    binding = {}
    for name, text in texts.items():
        if module.route(text) != ROUTES[name]:
            raise ValueError(f'{name}: BD080 binding route {module.route(text)!r} != {ROUTES[name]!r}')
        output, installed = module.install(text, text)
        if installed != ['bd080'] or module.prefix(ROUTES[name]) not in output or module.INCLUDE not in output:
            raise ValueError(f'{name}: BD080 binding did not install')
        if module.install(text, output)[1]:
            raise ValueError(f'{name}: BD080 binding is not idempotent')
        current = module.body(text, 'bd080')[0]
        changed = text.replace(current, current.replace('0x4eabccu', '0x4eabd0u', 1), 1)
        if changed == text or module.install(changed, changed)[1]:
            raise ValueError(f'{name}: BD080 binding accepted a changed body')
        for callee in ('ba970', 'baa40'):
            found = module.body(text, callee)
            if found:
                altered = text.replace(found[0], found[0].replace('0x44u', '0x48u', 1), 1)
                if module.install(altered, altered)[1]:
                    raise ValueError(f'{name}: BD080 binding accepted a changed {callee} body')
        if 'selected(0x420u,s.image)' in text:
            remapped = re.sub(r'case 0x420u:return 0x420u;', 'case 0x420u:return 0x424u;', text, count=1)
            if remapped == text or module.install(remapped, remapped)[1]:
                raise ValueError(f'{name}: BD080 binding accepted a nonidentity selected operand')
        binding[name] = ROUTES[name]
    for name, bodies in EXTRACT.items():
        text = texts[name]
        found = [module.body(text, body)[0] for body in bodies]
        declarations = ''.join(f'static void run_{body}(WorldState &);\n' for body in bodies)
        (scratch / f'sphere_{name}.inc').write_text(declarations + '\n'.join(found) + '\n')
    report = dict(passed=False, units={k: str(v.relative_to(ROOT)) for k, v in UNITS.items()}, binding=binding,
                  pins=dict(bd080=module.BD080_PINS, callees={k: sorted(v) for k, v in module.CALLEE_PINS.items()}))
    release = compile_harness(scratch, 'sphere-release', [])
    result = run(release, args.cases)
    census = run(release, max(args.cases // 4, 600), 'census')
    verify = run(release, max(args.cases // 6, 450), 'verify')
    names = [name for name in re.search(r'enum Branch : unsigned \{(.*?)\};', HEADER.read_text(), re.S)[1].replace('\n', ' ').replace(' ', '').split(',') if name]
    names = [name for name in names if name != 'branch_count']
    hit = {name for index, name in enumerate(names) if result['coverage'] >> index & 1}
    missing = sorted(set(names) - hit - UNREACHABLE)
    report.update(release=result, census=census, verify=verify, branches=names, covered=sorted(hit),
                  unreachable=sorted(UNREACHABLE), missing=missing)
    if missing:
        raise ValueError(f'Uncovered native object sphere query branches: {missing}')
    if census['compared'] < 100 or verify['compared'] < 100:
        raise ValueError('Census or verify mode compared too few cases')
    if not args.skip_sanitizers:
        sanitized = compile_harness(scratch, 'sphere-sanitized', [], sanitizers=True)
        report['sanitized'] = run(sanitized, max(args.cases // 4, 600))
        report['sanitized_verify'] = run(sanitized, 300, 'verify')
        sanitized.unlink()
    release.unlink()
    header_text = HEADER.read_text()

    def screen(mutant):
        name, old, new = mutant
        if header_text.count(old) != 1:
            return dict(name=name, rejected=False, error='mutation anchor missing')
        directory = scratch / 'sphere-mutants' / name / 'physics/runtime'
        directory.mkdir(parents=True)
        (directory / HEADER.name).write_text(header_text.replace(old, new, 1))
        try:
            binary = compile_harness(scratch, f'sphere-mutant-{name}', [scratch / 'sphere-mutants' / name])
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
    print(f"PASS {result['compared']} native object sphere query comparisons ({result['ordered_stores']} ordered stores, "
          f"{result['faults']} faults, {result['children']} genuine children), census {census['compared']}, "
          f"verify {verify['compared']}, {len(mutants)} mutants rejected")
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
