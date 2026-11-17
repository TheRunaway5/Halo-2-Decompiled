#!/usr/bin/env python3
"""Replay 30E235 packed route against authentic original selector and jump tables."""
import argparse
import hashlib
import json
import os
import platform
import random
import struct
import subprocess
import types
from pathlib import Path

from validate_native_simplex_hull_seed_frame_tu15 import BODY_PIN, PIN, ROOT, Xbe, authored, sha
from validate_native_tu15_prt_helpers import physical_original

ENTRY=0x30e235
SELECTOR,BRANCHES=0x30e908,0x30e8dc
DESTINATIONS=(0x30e89b,0x30e6bf,0x30e3a7,0x30e362,0x30e7c7,0x30e2af,0x30e25f,0x30e504,0x30e27d,0x30e4d3,0x30e8b5)
TABLE=bytes.fromhex('000102030a0a0a0a0405060a0a0a0a0a07080a0a0a0a0a0a09')
REGION=(0x61000000,0x3000)
IO=(ENTRY,0x30e237,0x30e244,0x30e251,0x30e258)
SITES=set(IO)|{0x30e239,0x30e23c,0x30e23e,0x30e241,0x30e24b}
DRIVER=ROOT/'tests/native_tu15_simplex_hull_reduction_route_tests.cpp'
ORIGINAL_OWNER=0x30de60
TABLE_OPCODES=((0x30e251,'0fb68008e93000'),(0x30e258,'ff2485dce83000'))
REPEAT_ENTRY=0x30e5ec
REPEAT_BRANCH=(0x30e5fb,'0f8650fcffff')
REPEAT_IO=(0x30e5ec,0x30e5ee,0x30e251,0x30e258)
REPEAT_SITES=set(REPEAT_IO)|{0x30e5f0,0x30e5f3,0x30e5f5,0x30e5f8,0x30e5fb,0x30e601}
EXTRA_INPUTS=()


def inputs(image):
    if image.read(SELECTOR,25)!=TABLE or struct.unpack('<11I',image.read(BRANCHES,44))!=DESTINATIONS:
        raise ValueError('Actual original hull selector or branch table drift')
    base,size=REGION;context=base+0x1000;sp=base+0x2000;rng=random.Random(ENTRY);rows=[]
    pairs=[(key>>3,key&7)for key in range(9,34)]+[(0,0),(0xffffffff,1),(2,0xffffffff),(0x80000000,0),(5,4)]
    live=('0000000000000080ff3f','35c26821a2da0fc90040','00000000000000000080','45230100000000c0ff7f')
    for first,second in pairs:
        for variant in range(4):
            raw=bytearray(rng.randbytes(size));gp=[rng.getrandbits(32)for _ in range(8)]
            gp[4],gp[6],gp[7]=sp,context,context+(0x10 if variant==3 else 4)
            struct.pack_into('<I',raw,context-base,first);struct.pack_into('<I',raw,gp[7]-base,second)
            words=struct.unpack_from('<3I',raw,sp-base);i=len(rows)
            rows.append(dict(id=i,name=f'route-{first:x}-{second:x}-{variant}',entry=ENTRY,gp=gp,
                eflags=0x202|(rng.getrandbits(12)&0xcd5),xmm=[[rng.getrandbits(32)for _ in range(4)]for _ in range(8)],
                before_fp=[(0x7f,0x27f,0x37f)[i%3]|((i%4)<<10),0x4100|((i%8)<<11),0x1f80|(i%4)<<13],
                live_x87=[live[n%4]for n in range((0,1,4,7)[variant])],return_pc=words[0],matrix=words[1],source=words[2],
                stop_before_entries=list(DESTINATIONS),max_steps=40,selector_table=TABLE.hex(),
                branch_table=image.read(BRANCHES,44).hex(),banks=[dict(address=base,before=raw.hex())]))
    return rows


def original(image,rows,scratch,sites):
    run=subprocess.run
    def launch(command,*args,**kwargs):
        if command[0]=='as':
            observer=Path(command[2]).parent/'observer.cpp';source=observer.read_text()
            anchor='auto retained=[&](std::uint32_t p,unsigned n){'
            if source.count(anchor)!=1:raise ValueError('Exact original readonly table observation seam drift')
            readonly=(f'if((n==1u&&p>=0x{SELECTOR:x}u&&p<0x{SELECTOR+25:x}u)||'
                      f'(n==4u&&p>=0x{BRANCHES:x}u&&p<0x{BRANCHES+44:x}u))return true;')
            observer.write_text(source.replace(anchor,anchor+readonly,1))
        return run(command,*args,**kwargs)
    context=physical_original.__globals__.copy();context['subprocess']=types.SimpleNamespace(run=launch)
    scoped=types.FunctionType(physical_original.__code__,context,physical_original.__name__,
                              physical_original.__defaults__,physical_original.__closure__)
    scoped.__kwdefaults__=physical_original.__kwdefaults__.copy()
    trace=[dict(entry=pc,arguments=0,gp=True,xmm=True)for pc in sites]
    return scoped(image,rows,scratch,entries=(ORIGINAL_OWNER,),regions=(REGION,),trace_calls=trace,
                  trace_extended=True,preserve_x87_prefix=True)


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--report',type=Path,required=True);p.add_argument('--xbe',type=Path,required=True)
    p.add_argument('--compiler',default='c++');p.add_argument('--sanitizers',action='store_true')
    p.add_argument('--redispatch',action='store_true')
    a=p.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):p.error('Run through tools/run_test.py')
    if platform.system()!='Linux' or platform.machine().lower()not in('x86_64','amd64','i386','i686') or not a.xbe.is_file():return 77
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);paths=(DRIVER,Path(__file__),*EXTRA_INPUTS,ROOT/'src/physics/runtime/native_tu15_simplex_hull_reduction_route_frame.hpp',
          ROOT/'include/halo2/native_tu15_simplex_hull_reduction_route_kernel.hpp',ROOT/'tests/native_tu15_mopp_ray_frame_test_memory.hpp',
          ROOT/'tools/observe_native_campaign_ai_vector_math_tu15.cpp',ROOT/'tools/validate_native_tu15_prt_helpers.py',
          ROOT/'src/physics/runtime/native_tu15_raw_x87_scope.hpp',ROOT/'src/physics/runtime/native_tu15_solver_hull_seed_route_frame.hpp')
    pins={str(path.relative_to(ROOT)):sha(path)for path in paths}
    report=dict(passed=False,production_bound=False,scratch_owner=str(scratch),source_sha256=pins,retained_large_artifacts=[])
    try:
        if sha(a.xbe)!=PIN or hashlib.sha256(authored().encode()).hexdigest()!=BODY_PIN:
            raise ValueError('Actual original image or complete World body drift')
        image=Xbe(a.xbe)
        if any(image.read(pc,len(opcode)//2).hex()!=opcode for pc,opcode in TABLE_OPCODES):
            raise ValueError('Actual original table operand opcode drift')
        rows=inputs(image);io=IO;sites=SITES
        if a.redispatch:
            pc,opcode=REPEAT_BRANCH
            if image.read(pc,len(opcode)//2).hex()!=opcode:raise ValueError('Actual redispatch branch drift')
            for row in rows:row['entry']=REPEAT_ENTRY
            io=REPEAT_IO;sites=REPEAT_SITES
        data=scratch/'inputs.json';data.write_text(json.dumps(rows,separators=(',',':')));report['input_sha256']=sha(data)
        command=[a.compiler,'-std=c++20','-Wno-ignored-attributes','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math',
                 '-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(ROOT/'tests'),str(DRIVER),'-o',str(scratch/'reduction_route')]
        if a.sanitizers:command[1:1]=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
        report['command']=command;built=subprocess.run(command,capture_output=True,text=True)
        if built.returncode:raise ValueError('Private reduction route compile failed: '+built.stderr[-10000:])
        out=scratch/'native.json'
        with data.open('rb') as inp,out.open('wb') as output:
            done=subprocess.run([command[-1]],stdin=inp,stdout=output,stderr=subprocess.PIPE)
        if done.returncode:raise ValueError('Native reduction route stopped: '+done.stderr.decode()[-6000:])
        actual=json.loads(out.read_text());observed,report['original']=original(image,rows,scratch,io)
        seen=set();exits=set();counts={key:0 for key in ('packets','reads','writes')}
        for case,got in zip(observed['cases'],actual,strict=True):
            row=rows[case['id']];stop=case.get('stopped_at')
            if stop not in DESTINATIONS or not case['live_x87_unchanged']:raise ValueError('Original escaped private table scope')
            packets=[{k:v for k,v in event.items()if k!='arguments'}for event in case['calls']]
            want=dict(abi=dict(entry=stop,gp=case['boundary_gp'],eflags=case['boundary_eflags'],xmm=case['boundary_xmm'],
                 fp=case['after_fp'],live_x87=row['live_x87']),packets=packets,
                 banks=sorted(case['banks'],key=lambda bank:bank['address']),reads=case['reads'],writes=case['writes'])
            if want!=got:
                key=next(k for k in want if want[k]!=got[k]);report['first_difference']=dict(case=row['name'],field=key)
                raise ValueError('Actual original hull reduction route differs: '+row['name']+' '+key)
            for key in counts:counts[key]+=len(want[key])
            seen.update(case['executed_sites']);exits.add(stop)
        if seen!=sites or exits!=set(DESTINATIONS):raise ValueError('Incomplete original hull reduction route coverage')
        report.update(passed=True,cases=len(rows),executed_parent_sites=len(seen),counts=counts,continuations=sorted(exits),
                      original_text_unmodified=True,hostile_provider_comparisons=len(rows),redispatch=a.redispatch,
                      scope='Declared actual mid-owner frames and unmodified executable tables. Stops before the actual reduction branch.')
    except Exception as error:report['error']=str(error)
    report['sources_unchanged']=all(sha(ROOT/name)==value for name,value in pins.items())
    if not report['sources_unchanged']:report.update(passed=False,error='Private reduction route source drift')
    a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({key:report[key]for key in('passed','error','first_difference','executed_parent_sites')if key in report}))
    return 0 if report['passed']else 1

if __name__=='__main__':raise SystemExit(main())
