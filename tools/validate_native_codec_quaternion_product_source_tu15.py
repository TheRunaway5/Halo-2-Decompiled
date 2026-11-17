#!/usr/bin/env python3
"""Qualify the private 1201E0 retained view against both genuine source copies."""
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

from validate_native_codec_inverse_transform_source_tu15 import ORDERED, exact
from validate_native_codec_quaternion_product_tu15 import ENTRY, FRAME, KERNEL, OWNERS, ROOT, cases, original_owners, sha
from oracle_smg_firing_1_1 import Xbe, _Code
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN

SOURCE='src/physics/runtime/native_tu15_codec_quaternion_product_source_frame.hpp'
HARNESS='tests/native_tu15_codec_inverse_transform_source_tests.cpp'
STORE_PCS=(0x1201e5,0x1201ea,0x1201f1,0x1201fb,0x1201ff,0x12020d,0x120214,0x12021e,
           0x120222,0x12025c,0x120292,0x1202c9,0x120300)
EXTRA_READS=(0x1201e8,0x1201ee,0x1201f5,0x1201f8,0x12020b,0x120211,0x120218,0x12021b,0x120305)


def retained_sources(code,scratch):
    blocks=[];contexts={}
    for index,(name,pin)in enumerate(OWNERS.items()):
        text=(ROOT/name).read_text()
        body=exact(text,r'static void run_1201e0\(WorldState &s\) \{.*?\n\}')
        if hashlib.sha256(body.encode()).hexdigest()!=pin:raise ValueError('Exact quaternion product body changed')
        state=exact(text,r'struct LiveState : physics_world_runtime_detail::WorldState \{.*?\n\};\nusing WorldState=LiveState;')
        resolver=exact(text,r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}')
        ordered=[];sites=[]
        for line in body.splitlines():
            operations=re.findall(ORDERED,line)
            if operations:
                pc=int(re.search(r's.services.instruction\(0x([0-9a-f]+)u\)',line)[1],16)
                if len(operations)!=1 or code.at(pc).mnemonic!=operations[0]+'ss':
                    raise ValueError('Original quaternion product ordered operand changed')
                sites.append(pc)
                line=re.sub(ORDERED,lambda match:'exact_'+match[1]+'(',line)
            ordered.append(line)
        if len(sites)!=28:raise ValueError('Complete quaternion-product ordered SSE set changed')
        helpers=''.join(f'static __m128 exact_{name}(__m128 left,__m128 right){{asm volatile("{name}ss %1,%0":"+x"(left):"x"(right):"memory");return left;}}\n'
                        for name in ('add','sub','mul'))
        blocks.append(f'namespace inverse_source{index}{{using namespace halo2;using namespace halo2::physics_world_runtime_detail;\n'
                      +state+'\n'+resolver+'\n'+helpers+body.replace('run_1201e0(','legacy(',1)+'\n'
                      +'\n'.join(ordered).replace('run_1201e0(','ordered(',1)+'\n}\n')
        contexts[name]=dict(body=pin,resolver=hashlib.sha256(resolver.encode()).hexdigest(),
                            state=hashlib.sha256(state.encode()).hexdigest(),ordered_sites=list(map(hex,sites)))
    generated=''.join(blocks)
    (scratch/'codec_inverse_transform_sources.inc').write_text(generated)
    return contexts,hashlib.sha256(generated.encode()).hexdigest()


def harness(scratch):
    # Share the full ABI/provider comparator used by 120740. Only the selected
    # source view and the genuine current routine's write-site set differ.
    text=(ROOT/HARNESS).read_text()
    for old,new in (
        ('physics/runtime/native_tu15_codec_inverse_transform_source_frame.hpp',SOURCE.removeprefix('src/')),
        ('halo2::native_codec_inverse_transform_detail','halo2::native_codec_quaternion_product_detail'),
    ):
        if text.count(old)!=1:raise ValueError('Shared retained codec harness seam changed')
        text=text.replace(old,new,1)
    old=exact(text,r'memory\.write_sites=\{.*?\};')
    text=text.replace(old,'memory.write_sites={'+','.join(f'0x{pc:x}u'for pc in STORE_PCS)+'};',1)
    cpp=scratch/'quaternion_product_source.cpp'
    cpp.write_text(text.replace('Inverse transform','Quaternion product'))
    return cpp


def successors(code):
    frame=(ROOT/FRAME).read_text();source=(ROOT/SOURCE).read_text();pairs={}
    for name in ('read','store'):
        pcs=exact(frame,rf'inline constexpr std::array {name}s\{{.*?\}};') if name=='store'else exact(frame,r'inline constexpr std::array reads\{.*?\n\};')
        after=exact(source,rf'inline constexpr std::array {name}_successors\{{.*?\}};') if name=='store'else exact(source,r'inline constexpr std::array read_successors\{.*?\n\};')
        pairs.update(zip([int(pc,16)for pc in re.findall(r'0x([0-9a-f]+)u',pcs)],
                         [int(pc,16)for pc in re.findall(r'0x([0-9a-f]+)u',after)],strict=True))
    for pc in (*STORE_PCS,*EXTRA_READS):pairs.setdefault(pc,pc+code.at(pc).size)
    if len(pairs)!=54 or any(pc+code.at(pc).size!=after for pc,after in pairs.items()):
        raise ValueError('Exact quaternion-product retirements changed')
    return pairs


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--compiler',action='append')
    for arg in ('smoke','mmx','sanitizers'):p.add_argument('--'+arg,action='store_true')
    args=p.parse_args()
    if 'HALO2_TEST_SCRATCH'not in os.environ:p.error('Use tools/run_test.py')
    if platform.system()!='Linux'or platform.machine().lower()not in ('x86_64','amd64'):return 77
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);args.output.mkdir(parents=True,exist_ok=False)
    before={path.resolve():sha(path)for tree in ('tools','include','src','tests')for path in (ROOT/tree).rglob('*')
            if path.is_file()and path.suffix in ('.py','.cpp','.hpp','.inc','.h')}
    used={Path(__file__).resolve(),ROOT/FRAME,ROOT/SOURCE,ROOT/KERNEL,ROOT/HARNESS,
          ROOT/'tools/validate_native_codec_inverse_transform_source_tu15.py',ROOT/'tools/validate_native_codec_quaternion_product_tu15.py'}
    report=dict(passed=False,private_unbound=True,owned_scratch=str(scratch),retained_large_artifacts=[],compilers=[],
                readonly_gp_xmm_fp_metadata_host_controls_status=True)
    try:
        if sha(NEW_DEFAULT)!=NEW_PIN:raise ValueError('Original English TU1.5 XBE changed')
        code=_Code(Xbe(NEW_DEFAULT),'.text');original=original_owners();context,generated=retained_sources(code,scratch)
        cpp=harness(scratch);rows=cases(args.smoke);normal=len(rows)
        if args.mmx:
            rng=random.Random(ENTRY)
            for mask in range(16 if args.smoke else 256):
                row=copy.deepcopy(rows[mask%normal]);row.update(name=row['name']+f'-opaque-{mask}',live_x87=[])
                row['before_fp'][1]&=~0x3800
                row['active_mmx']=dict(known_mask=mask,physical=[
                    (rng.getrandbits(64).to_bytes(8,'little')+(0xffff,0x3fff,0,0x7fff)[(mask+n)%4].to_bytes(2,'little')).hex()for n in range(8)])
                rows.append(row)
        exact_pcs=successors(code)
        report.update(contexts=context,generated_source_sha256=generated,shared_harness_sha256=sha(cpp),
                      retirements=exact_pcs,normal_rows=normal,opaque_rows=len(rows)-normal)
        fixture=scratch/'inputs.json';fixture.write_text(json.dumps(dict(cases=rows,successors=exact_pcs),separators=(',',':')))
        for compiler in args.compiler or ('g++','clang++'):
            binary=scratch/Path(compiler).name;dep=binary.with_suffix('.d')
            command=[compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math',
                     '-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(ROOT/'tests'),'-I'+str(scratch),
                     str(cpp),'-o',str(binary),'-MD','-MF',str(dep)]
            if args.sanitizers:command.extend(('-fsanitize=address,undefined','-fno-omit-frame-pointer'))
            done=subprocess.run(command,capture_output=True,text=True,timeout=480)
            if done.returncode:raise ValueError('Quaternion-product retained compile failed: '+done.stderr[-10000:])
            for value in shlex.split(dep.read_text().replace('\\\n',' ').split(':',1)[1]):
                path=Path(value).resolve()
                if path in before:used.add(path)
            done=subprocess.run([str(binary),str(fixture)],capture_output=True,text=True,timeout=480)
            if done.returncode:raise ValueError('Quaternion-product retained comparison failed: '+(done.stdout+done.stderr)[-12000:])
            report['compilers'].append(dict(compiler=compiler,command=command,result=json.loads(done.stdout)))
        if original_owners()!=original or retained_sources(code,scratch)!=(context,generated):
            raise ValueError('Exact quaternion-product source context drift')
        report['passed']=True
    except BaseException as error:report['error']=repr(error);raise
    finally:
        report['source_before']={str(path.relative_to(ROOT)):before[path]for path in sorted(used)}
        report['source_after']={name:sha(ROOT/name)for name in report['source_before']}
        if report['source_before']!=report['source_after']:report.update(passed=False,source_drift=True)
        (args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    return 0 if report['passed']else 1


if __name__=='__main__':raise SystemExit(main())
