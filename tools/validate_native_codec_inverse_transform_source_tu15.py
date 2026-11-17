#!/usr/bin/env python3
"""Private 120740 retained publication and measured original-order SSE control."""
import argparse
import copy
import hashlib
import json
import os
import platform
import random
import re
import shlex
import subprocess
from pathlib import Path

from validate_native_codec_inverse_transform_tu15 import ENTRY, FRAME, KERNEL, RET, ROOT, SOURCE_PINS, cases, owner_bodies, sha
from oracle_smg_firing_1_1 import Xbe, _Code
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN

TEST = 'tests/native_tu15_codec_inverse_transform_source_tests.cpp'
SOURCE = 'src/physics/runtime/native_tu15_codec_inverse_transform_source_frame.hpp'
ORDERED = r'_mm_(add|sub|mul|div)_ss\('


def exact(text, pattern):
    values=re.findall(pattern,text,re.S)
    if len(values)!=1:
        raise ValueError('Missing complete inverse transform source context')
    return values[0]


def sources(code, scratch):
    receipts, generated = {}, []
    for index,(name,pin) in enumerate(SOURCE_PINS.items()):
        text=(ROOT/name).read_text()
        body=exact(text,r'static void run_120740\(WorldState &s\) \{.*?\n\}')
        state=exact(text,r'struct LiveState : physics_world_runtime_detail::WorldState \{.*?\n\};\nusing WorldState=LiveState;')
        resolver=exact(text,r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}')
        if hashlib.sha256(body.encode()).hexdigest()!=pin:
            raise ValueError('Exact inverse transform retained body changed')
        ordered=[]
        sites=[]
        for line in body.splitlines():
            op=re.findall(ORDERED,line)
            if op:
                pc=int(re.search(r's.services.instruction\(0x([0-9a-f]+)u\)',line)[1],16)
                if len(op)!=1 or code.at(pc).mnemonic!=op[0]+'ss':
                    raise ValueError('Original ordered inverse transform opcode differs')
                sites.append(pc)
                line=re.sub(ORDERED,lambda match:'exact_'+match[1]+'(',line)
            ordered.append(line)
        if len(sites)<30:
            raise ValueError('Incomplete ordered inverse transform control')
        helpers=''.join(f'static __m128 exact_{name}(__m128 left,__m128 right){{asm volatile("{name}ss %1,%0":"+x"(left):"x"(right):"memory");return left;}}\n'
                        for name in ('add','sub','mul','div'))
        generated.append(f'namespace inverse_source{index}{{using namespace halo2;using namespace halo2::physics_world_runtime_detail;\n'
                         +state+'\n'+resolver+'\n'+helpers+body.replace('run_120740(','legacy(',1)+'\n'
                         +'\n'.join(ordered).replace('run_120740(','ordered(',1)+'\n}\n')
        receipts[name]=dict(state_sha256=hashlib.sha256(state.encode()).hexdigest(),
                            resolver_sha256=hashlib.sha256(resolver.encode()).hexdigest(),ordered_sites=list(map(hex,sites)))
    output=''.join(generated)
    (scratch/'codec_inverse_transform_sources.inc').write_text(output)
    return receipts,hashlib.sha256(output.encode()).hexdigest()


def retirements(code):
    frame=(ROOT/FRAME).read_text();source=(ROOT/SOURCE).read_text();pairs={}
    for name,singular in (('reads','read'),('stores','store')):
        positions=[int(pc,16)for pc in re.findall(r'Operand\{0x([0-9a-f]+)u,',exact(frame,rf'inline constexpr std::array {name}\{{.*?\n\}};'))]
        nexts=[int(pc,16)for pc in re.findall(r'0x([0-9a-f]+)u',exact(source,rf'inline constexpr std::array {singular}_successor\{{.*?\n\}};'))]
        pairs.update(zip(positions,nexts,strict=True))
    pairs.update({pc:pc+code.at(pc).size for pc in (0x120748,0x120749,0x12074c,0x120758,0x12075a,0x12075d,
                  0x120760,0x120763,0x120766,0x120769,0x12086f,0x120870)})
    if ENTRY not in pairs or RET in pairs or len(pairs)!=62 or any(code.at(pc).size+pc!=next for pc,next in pairs.items()):
        raise ValueError('Inverse transform exact completed-I/O successor set changed')
    return pairs


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--compiler',action='append')
    parser.add_argument('--smoke',action='store_true')
    parser.add_argument('--sanitizers',action='store_true')
    parser.add_argument('--mmx',action='store_true')
    args=parser.parse_args()
    if 'HALO2_TEST_SCRATCH' not in os.environ:
        parser.error('Use tools/run_test.py')
    if platform.system()!='Linux' or platform.machine().lower() not in ('x86_64','amd64'):
        return 77
    scratch=Path(os.environ['HALO2_TEST_SCRATCH'])
    args.output.mkdir(parents=True,exist_ok=False)
    before={path.resolve():sha(path)for tree in ('tools','include','src','tests')for path in (ROOT/tree).rglob('*')
            if path.is_file()and path.suffix in ('.py','.cpp','.hpp','.inc','.h')}
    used={Path(__file__).resolve(),ROOT/TEST,ROOT/SOURCE,ROOT/KERNEL,ROOT/FRAME,ROOT/'tools/validate_native_codec_inverse_transform_tu15.py'}
    report=dict(passed=False,private_unbound=True,owned_scratch=str(scratch),retained_large_artifacts=[],compilers=[],
                readonly_gp_xmm_fp_metadata_host_controls_status=True,scope='Authored retained I/O and exact ordered SSE control; unmodified compiler comparison recorded separately')
    try:
        if sha(NEW_DEFAULT)!=NEW_PIN:
            raise ValueError('Original TU1.5 XBE changed')
        before_owners=owner_bodies();image=Xbe(NEW_DEFAULT);code=_Code(image,'.text')
        contexts,generated=sources(code,scratch)
        report.update(source_contexts=contexts,source_copy_sha256=generated,successors=retirements(code))
        rows=cases(image,args.smoke)
        normal=len(rows)
        if args.mmx:
            rng=random.Random(0x120740f00)
            for mask in range(16 if args.smoke else 256):
                row=copy.deepcopy(rows[mask%normal]);row.update(name=row['name']+f'-opaque-{mask}',live_x87=[])
                row['before_fp'][1]&=~0x3800
                row['active_mmx']=dict(known_mask=mask,physical=[
                    (rng.getrandbits(64).to_bytes(8,'little')+(0xffff,0x3fff,0,0x7fff)[(mask+n)%4].to_bytes(2,'little')).hex()for n in range(8)])
                rows.append(row)
        report.update(dense_rows=normal,opaque_rows=len(rows)-normal)
        fixture=scratch/'inputs.json';fixture.write_text(json.dumps(dict(cases=rows,successors=report['successors']),separators=(',',':')))
        for compiler in args.compiler or ('g++','clang++'):
            binary=scratch/Path(compiler).name;dep=binary.with_suffix('.d')
            command=[compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math',
                     '-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(scratch),str(ROOT/TEST),'-o',str(binary),'-MD','-MF',str(dep)]
            if args.sanitizers:
                command.extend(('-fsanitize=address,undefined','-fno-omit-frame-pointer'))
            done=subprocess.run(command,capture_output=True,text=True,timeout=480)
            if done.returncode:
                raise ValueError('Inverse transform source compile failed: '+done.stderr[-10000:])
            for value in shlex.split(dep.read_text().replace('\\\n',' ').split(':',1)[1]):
                path=Path(value).resolve()
                if path in before:used.add(path)
            done=subprocess.run([str(binary),str(fixture)],capture_output=True,text=True,timeout=480)
            if done.returncode:
                raise ValueError('Inverse transform retained proof failed: '+(done.stdout+done.stderr)[-14000:])
            report['compilers'].append(dict(compiler=compiler,command=command,result=json.loads(done.stdout)))
        if owner_bodies()!=before_owners or sources(code,scratch)!=(contexts,generated):
            raise ValueError('Exact inverse transform source context drift')
        report['passed']=True
    except BaseException as error:
        report['error']=repr(error);raise
    finally:
        report['source_before']={str(path.relative_to(ROOT)):before[path]for path in sorted(used)}
        report['source_after']={name:sha(ROOT/name)for name in report['source_before']}
        if report['source_before']!=report['source_after']:report.update(passed=False,source_drift=True)
        (args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    return 0 if report['passed'] else 1


if __name__=='__main__':
    raise SystemExit(main())
