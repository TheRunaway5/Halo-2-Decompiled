#!/usr/bin/env python3
"""Compare the native World 3077F0 owner and its pinned 3037F0 source body."""

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys

import native_world_leaves_remap_sites as remaps
import native_world_leaves_replacements as world


ROOT = Path(__file__).resolve().parents[1]
UNIT = ROOT / 'src/physics/runtime/physics_world_runtime_generated.cpp'
HEADER = ROOT / 'src/physics/runtime/native_tu15_soa_support_world.hpp'
SYSTEMS = ROOT / 'src/physics/runtime/native_tu15_soa_support_world_systems.hpp'
TEST = ROOT / 'tests/native_tu15_soa_support_world_tests.cpp'
HARNESS = ROOT / 'tests/native_tu15_frame_misc_harness.hpp'
PIN = 'ade6eb5bf5a499943b696791f5e3d19243c05db5cd29208776278e87757dc850'
IMAGE_SHA256 = '21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'
TABLE_SHA256 = '533c8371259f8be444d6389dbff53386b5540d58f162e0c9c89c8d26f3739650'
X87_OPERAND = re.compile(r'ext\(f\((r\(s\.memory,[^,\n]*,4\))\)\)')
X87_ORDERED = r'halo2::native_soa_support_world_detail::load_operand(\1)'
MODES = ('', 'census', 'verify', *remaps.MODES)

MUTANTS = (
    (SYSTEMS, 'mask-thirteen', 'bool test_x=search==Search::without_y;',
     'bool test_x=search==Search::without_x;'),
    (SYSTEMS, 'dot-order', 'const auto dots=add(add(x_terms,y_terms),z_terms);',
     'const auto dots=add(add(z_terms,y_terms),x_terms);'),
    (SYSTEMS, 'feature-tag', 'feature_tag=0x3f000000u;', 'feature_tag=0x3f800000u;'),
    (SYSTEMS, 'scan-stride', 'group+=group_stride;', 'group+=axis_stride;'),
    (HEADER, 'x87-tie', '(accumulator>>8u)&0x41u,1u', '(accumulator>>8u)&0x01u,1u'),
    (HEADER, 'mask-fourteen', 'if(port.flags.cf||port.flags.zf)return true;',
     'if(port.flags.cf)return true;'),
    (HEADER, 'scan-negative', 'const bool scan=!flags.zf&&(flags.sf==flags.of);',
     'const bool scan=!flags.zf;'),
    (HEADER, 'visit-cursor', '0x307892u,0x3078a2u,', '0x307893u,0x3078a2u,'),
    (HEADER, 'restore-output', 'port.ret(0x307983u,8u);', 'port.ret(0x307983u,4u);'),
)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def prepare(scratch, xbe_path):
    sys.path.insert(0, str(ROOT / 'tools/reference/halo2-decompiled/tools'))
    from xbe import Xbe

    image = Xbe(xbe_path)
    tables = image.read(0x307988, 43)
    if sha(image.data) != IMAGE_SHA256 or sha(tables) != TABLE_SHA256:
        raise ValueError('TU1.5 XBE or support dispatch table drift')

    source = UNIT.read_text()
    found = world.body(source, '3037f0')
    if not found or world.digest(found) != PIN:
        raise ValueError('Unbound canonical World support body')
    mapping = world.mapping(source)
    if any(mapping.get(a) != b for a,b in ((0x303988,0x307988),(0x3039a4,0x3079a4))):
        raise ValueError('World support table relocation drift')
    body, count = X87_OPERAND.subn(X87_ORDERED, world.reference(found[0]))
    if count != 16:
        raise ValueError('World support x87 operand count drift')

    (scratch / 'body.inc').write_text(body + '\n')
    (scratch / 'resolvers.inc').write_text('\n'.join(
        re.search(r'^std::uint32_t ' + name + r'\(.*$',source,re.M)[0]
        for name in ('selected','canonical_branch')) + '\n')
    (scratch / 'tables.inc').write_text(','.join(f'0x{byte:02x}u' for byte in tables) + '\n')
    remaps.write(scratch)


def compile_harness(compiler, scratch, name, headers=None, sanitize=False):
    binary = scratch / name
    command = [compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math']
    if sanitize:
        command += ['-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-omit-frame-pointer']
    if headers is not None:
        command += [f'-I{headers}']
    command += [f'-I{ROOT / "include"}',f'-I{ROOT / "src"}',f'-I{scratch}',str(TEST),'-o',str(binary)]
    subprocess.run(command, check=True, cwd=scratch)
    return binary


def run(binary, cases, mode):
    result = subprocess.run([str(binary),str(cases),mode], text=True, capture_output=True)
    if result.returncode:
        raise RuntimeError(f'{binary.name} ({mode or "literal"}) failed:\n{result.stderr.strip()}')
    return json.loads(result.stdout)


def run_mutants(compiler, scratch, cases):
    rejected = []
    for path, name, old, new in MUTANTS:
        mutant = scratch / 'mutants' / name
        header = mutant / path.relative_to(ROOT / 'src')
        header.parent.mkdir(parents=True)
        original = path.read_text()
        if original.count(old) != 1:
            raise ValueError(f'Mutation anchor drift: {name}')
        header.write_text(original.replace(old,new,1))
        binary = compile_harness(compiler,scratch,name,headers=mutant)
        for mode in ('','verify'):
            result = subprocess.run([str(binary),str(cases),mode], text=True, capture_output=True)
            if result.returncode:
                rejected.append({'name':name,'mode':mode,'diagnostic':result.stderr.strip()[-400:]})
                break
        else:
            raise RuntimeError(f'Mutation survived: {name}')
        binary.unlink()
    return rejected


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--xbe',type=Path,required=True)
    parser.add_argument('--report',type=Path,required=True)
    parser.add_argument('--compiler',default='c++')
    parser.add_argument('--cases',type=int,default=8000)
    parser.add_argument('--mutant-cases',type=int,default=1000)
    parser.add_argument('--skip-sanitizers',action='store_true')
    parser.add_argument('--skip-mutants',action='store_true')
    args = parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):
        parser.error('Run through tools/run_test.py')
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    files = {UNIT,HEADER,SYSTEMS,TEST,HARNESS,*remaps.SOURCES,Path(world.__file__),Path(__file__).resolve()}
    hashes = {str(path.relative_to(ROOT)):sha(path.read_bytes()) for path in sorted(files)}
    prepare(scratch,args.xbe)
    release = compile_harness(args.compiler,scratch,'release')
    results = {mode or 'literal':run(release,args.cases,mode) for mode in MODES}
    coverage = 0
    for result in results.values():
        coverage |= result['coverage']
    branch_count = results['literal']['branches']
    if coverage != (1 << branch_count) - 1:
        raise ValueError(f'Missing World support branches: coverage {coverage:x}')

    sanitized = {}
    if not args.skip_sanitizers:
        binary = compile_harness(args.compiler,scratch,'sanitized',sanitize=True)
        sanitized = {mode or 'literal':run(binary,max(1000,args.cases//5),mode) for mode in ('','verify','remap-verify')}
    rejected = [] if args.skip_mutants else run_mutants(args.compiler,scratch,args.mutant_cases)
    if hashes != {str(path.relative_to(ROOT)):sha(path.read_bytes()) for path in sorted(files)}:
        raise ValueError('World support proof sources changed during validation')
    report = {'passed':True,'pin':PIN,'image_sha256':IMAGE_SHA256,'table_sha256':TABLE_SHA256,
              'source_sha256':hashes,'results':results,'sanitized':sanitized,'mutants':rejected}
    args.report.parent.mkdir(parents=True,exist_ok=True)
    args.report.write_text(json.dumps(report,indent=2,sort_keys=True) + '\n')
    print(f'PASS 3077F0: {sum(row["compared"] for row in results.values())} comparisons across six modes, '
          f'{branch_count}/{branch_count} branches, {len(rejected)} mutants rejected')


if __name__ == '__main__':
    main()
