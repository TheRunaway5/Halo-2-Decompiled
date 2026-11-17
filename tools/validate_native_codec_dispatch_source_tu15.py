#!/usr/bin/env python3
"""Compare the private selector view with the nine unchanged retained bodies."""
import argparse
import copy
import hashlib
import json
import os
import platform
import re
import shlex
import subprocess
from pathlib import Path

from validate_native_codec_dispatch_tu15 import ENTRIES, ROOT, RETURNS, SOURCE_PINS, cases, sha, source_owners

TEST = 'tests/native_tu15_codec_dispatch_source_tests.cpp'
FRAME = 'src/physics/runtime/native_tu15_codec_dispatch_source_frame.hpp'


def unique(text, pattern):
    matches = re.findall(pattern, text, re.S)
    if len(matches) != 1:
        raise ValueError('Complete retained selector source context is missing or ambiguous')
    return matches[0]


def source_text(scratch):
    units, retained, routes = {}, [], []
    for name, entries in SOURCE_PINS:
        source = (ROOT/name).read_text()
        resolver = unique(source, r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}')
        state = unique(source, r'struct LiveState : physics_world_runtime_detail::WorldState \{.*?\n\};\nusing WorldState=LiveState;')
        units[name] = dict(resolver=hashlib.sha256(resolver.encode()).hexdigest(), state=hashlib.sha256(state.encode()).hexdigest())
        for entry, pin in entries:
            body = unique(source,rf'static void run_{entry:x}\(WorldState &s\) \{{.*?\n\}}')
            if hashlib.sha256(body.encode()).hexdigest() != pin:
                raise ValueError('Exact retained selector body changed')
            route = len(routes)
            children = set(re.findall(r'\brun_([0-9a-f]+)\(s\);return;',body))
            diagnostic = 'throw std::runtime_error("Selector source child unexpectedly executed");'
            stubs = ''.join(f'[[maybe_unused]] static void run_{child}(WorldState&){{{diagnostic}}}\n' for child in sorted(children))
            stubs += f'[[maybe_unused]] static void dispatch(WorldState&,std::uint32_t,std::uint32_t,bool){{{diagnostic}}}\n'
            retained.append(f'namespace selector_source{route}{{using namespace halo2;using namespace halo2::physics_world_runtime_detail;\n'
                            + state+'\n'+resolver+'\n'+stubs+body.replace(f'run_{entry:x}(', 'entry(', 1)+'\n}\n')
            routes.append(f'X({route},0x{entry:x}u)')
    result=''.join(retained)+f'\n#define SELECTOR_SOURCE_COUNT {len(routes)}\n#define SELECTOR_SOURCE_ROUTES(X) '+ ' '.join(routes)+'\n'
    (scratch/'codec_dispatch_sources.inc').write_text(result)
    return units, hashlib.sha256(result.encode()).hexdigest()


def inputs(tails, smoke):
    result = []
    reads, checkpoints = set(), set()
    for row in cases(tails,smoke):
        if 'compose' in row:
            continue
        entry = row['entry']
        backing = bytes.fromhex(row['banks'][0]['before'])
        base = row['banks'][0]['address']
        def value(address):
            return int.from_bytes(backing[address-base:address-base+4],'little')
        outer = entry == ENTRIES[0]
        kind = value(0x510f34)
        row['returns'] = value(0x510f30)>4 if outer else kind>2
        row['variant_loads'] = not outer and kind<2
        result.append(row)
        accessed = [entry]
        stops = [entry+5]
        if row['variant_loads']:
            pc = entry + (0x38 if kind else 0x09)
            accessed += [pc,pc+7]
            stops += [pc+5,pc+12]
        if row['returns']:
            accessed.append(RETURNS[entry])
        for ordinal,pc in enumerate(accessed):
            if pc not in reads:
                prefix = copy.deepcopy(row)
                prefix.update(name=row['name']+f'-read-{pc:x}',fault=dict(phase='before_read',ordinal=ordinal))
                result.append(prefix)
                reads.add(pc)
        for pc in stops:
            if pc not in checkpoints:
                prefix = copy.deepcopy(row)
                prefix.update(name=row['name']+f'-retire-{pc:x}',stop=pc)
                result.append(prefix)
                checkpoints.add(pc)
    if len(checkpoints)!=26 or len(reads)!=32:
        raise ValueError('Retained selector source frontier corpus is incomplete')
    return dict(cases=result,transfers=tails)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--compiler',action='append')
    parser.add_argument('--smoke',action='store_true')
    parser.add_argument('--sanitizers',action='store_true')
    args = parser.parse_args()
    if 'HALO2_TEST_SCRATCH' not in os.environ:
        parser.error('Use tools/run_test.py')
    if platform.system() != 'Linux' or platform.machine().lower() not in ('x86_64','amd64'):
        return 77
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    args.output.mkdir(parents=True,exist_ok=False)
    before={path.resolve():sha(path) for tree in ('tools','include','src','tests') for path in (ROOT/tree).rglob('*')
            if path.is_file() and path.suffix in ('.py','.cpp','.hpp','.inc','.h')}
    used={Path(__file__).resolve(),ROOT/TEST,ROOT/FRAME,ROOT/'tools/validate_native_codec_dispatch_tu15.py'}
    report=dict(passed=False,private_unbound=True,owned_scratch=str(scratch),retained_large_artifacts=[],compilers=[],
                no_descendant_results=True,readonly_fp_metadata_host_controls_status=True)
    try:
        source_before, tails = source_owners()
        units, generated = source_text(scratch)
        report.update(source_contexts=units,source_copy_sha256=generated)
        fixture=scratch/'input.json'
        fixture.write_text(json.dumps(inputs(tails,args.smoke),separators=(',',':')))
        for compiler in args.compiler or ('g++','clang++'):
            binary=scratch/Path(compiler).name
            dep=binary.with_suffix('.d')
            command=[compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math',
                     '-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(scratch),str(ROOT/TEST),'-o',str(binary),'-MD','-MF',str(dep)]
            if args.sanitizers:
                command.extend(('-fsanitize=address,undefined','-fno-omit-frame-pointer'))
            done=subprocess.run(command,capture_output=True,text=True,timeout=480)
            if done.returncode:
                raise ValueError('Retained selector harness compile failed: '+done.stderr[-10000:])
            for value in shlex.split(dep.read_text().replace('\\\n',' ').split(':',1)[1]):
                path=Path(value).resolve()
                if path in before:
                    used.add(path)
            done=subprocess.run([str(binary),str(fixture)],capture_output=True,text=True,timeout=480)
            if done.returncode:
                raise ValueError('Retained selector harness failed: '+(done.stdout+done.stderr)[-12000:])
            report['compilers'].append(dict(compiler=compiler,command=command,result=json.loads(done.stdout)))
        if source_owners()!=(source_before,tails) or source_text(scratch)!=(units,generated):
            raise ValueError('Complete retained selector source context changed during proof')
        report['passed']=True
    except BaseException as error:
        report['error']=repr(error)
        raise
    finally:
        report['source_before']={str(path.relative_to(ROOT)):before[path] for path in sorted(used)}
        report['source_after']={name:sha(ROOT/name) for name in report['source_before']}
        if report['source_before']!=report['source_after']:
            report.update(passed=False,source_drift=True)
        (args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    return 0 if report['passed'] else 1


if __name__=='__main__':
    raise SystemExit(main())
