#!/usr/bin/env python3
"""Compare actual 313E8E setup with real Tetrahedron, Seed Facets and Plane children."""
import argparse
import hashlib
import json
import os
import platform
import re
import struct
import subprocess
import types
from pathlib import Path

from research_mp_sandbox_update_functions import Xbe
from validate_native_solver_hull_seed_route_frame_tu15 import BODY_PIN, actual_body
from validate_native_solver_hull_tetrahedron_frame_tu15 import (
    BODY_PIN as TETRA_PIN, actual_body as tetra_body, child_bodies, inputs as tetra_inputs,
)
from validate_native_tu15_prt_helpers import PIN, physical_original

ROOT=Path(__file__).resolve().parents[1]
ENTRY,STOP=0x313e8e,0x313f63
BASE,FS=0x61000000,0x62000000
CHILDREN={0x313ab0:4,0x314d80:0,0x314a30:0}
REGIONS=((0x41d000,0x1000),(0x46a000,0x1000),(BASE,0xc000),(FS,0x1000))
FS_SITES=(0x313ab7,0x313abe,0x313b9c,0x313bbd,0x313bc4,0x314343,0x314468,0x314484,0x3146e9)
VECTORS={0x313edf,0x313eeb,0x313ef7,0x313f0a,0x313f2b,0x313f37,0x313f43,0x313f57}
DRIVER=ROOT/'tests/native_tu15_solver_hull_search_setup_frame_tests.cpp'


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def parent(pc):
    return ENTRY<=pc<STOP


def inputs(image,smoke):
    rows=tetra_inputs(image)
    if smoke: rows=rows[:6]
    for row in rows:
        n=row['id'];gp=row['gp'];context=gp[1];sp=BASE+0x8000
        raw=bytearray.fromhex(row['banks'][2]['before'])+bytearray(0x4000)
        def put(address,*words):struct.pack_into('<'+'I'*len(words),raw,address-BASE,*words)
        vertices,payload=BASE+0x4500,BASE+0x4800
        for i in range(4):
            source=BASE+0x4200+i*32
            raw[vertices-BASE+16*i:vertices-BASE+16*(i+1)]=raw[source-BASE:source-BASE+16]
        if n%6==2:payload=sp+0x4a0
        if n%6==4:payload=sp+0x130
        if n%6==5:payload=sp+0x120
        put(context+0x48,payload);put(context+0x50,vertices)
        gp[4],gp[6]=sp,context
        words=struct.unpack_from('<3I',raw,sp-BASE)
        row.update(entry=ENTRY,name=f'search-setup-{n}',return_pc=words[0],matrix=words[1],source=words[2],
                   stop_before_entries=[STOP],max_steps=2500)
        row['banks'][2]['before']=raw.hex()
    return rows


def observe_with_fs(image,rows,scratch,trace,*,children=CHILDREN,regions=REGIONS,pass_first_stop=None):
    from oracle_native_campaign_screen_lifecycle_tu15 import _Code
    decoder=_Code(image,'.text');run=subprocess.run
    def launch(command,*args,**kwargs):
        if command[0]=='as':
            directory=Path(command[2]).parent;assembly=directory/'reference.s';text=assembly.read_text()
            opening='_start:\nfninit\nint3\njmp _start\n'
            if text.count(opening)!=1 or text.count('.section .bridge,"ax"')!=1:
                raise ValueError('Private search setup FS entry bridge drift')
            entry=('_start:\nmov $243,%eax\nmov $setup_tls,%ebx\nint $0x80\n'
                   'test %eax,%eax\njns 1f\nmov $1,%eax\nmov $126,%ebx\nint $0x80\n'
                   '1: mov setup_tls,%eax\nshl $3,%eax\nor $3,%eax\nmov %ax,%fs\nfninit\nint3\njmp _start\n')
            text=text.replace(opening,entry,1).replace('.section .bridge,"ax"','.section .bridge,"awx"',1)
            assembly.write_text(text+'\n.balign 16\nsetup_tls: .long -1,'+str(FS)+',0xfffff,0x51\n')
            metadata=directory/'metadata.json';auth=json.loads(metadata.read_text());sites=[]
            for site in auth['sites']:
                original=decoder.at(site['instruction'])
                ops=[op for op in original.operands if op.type==3 and original.reg_name(op.mem.segment)=='fs']
                if not ops:continue
                if len(ops)!=1 or len(site['operands'])!=1:raise ValueError('Unexpected original search setup FS operand')
                site['operands'][0]['fs_base']=FS;sites.append(site['instruction'])
            if tuple(sorted(sites))!=FS_SITES:raise ValueError('Original search setup FS operand inventory drift: '+repr(sites))
            metadata.write_text(json.dumps(auth))
            observer=directory/'observer.cpp';source=observer.read_text();needle='std::uint32_t p=o.at("displacement");'
            if source.count(needle)!=1:raise ValueError('Exact physical FS metadata consumer drift')
            source=source.replace(needle,needle+'p+=o.value("fs_base",0u);',1)
            if pass_first_stop is not None:
                state='bool finished{};'
                frontier='if(pc==stop||std::find(stop_entries.begin(),stop_entries.end(),pc)!=stop_entries.end())'
                if source.count(state)!=1 or source.count(frontier)!=1:raise ValueError('Exact first-pass observer seam drift')
                source=source.replace(state,state+'bool passed_first{};',1).replace(frontier,
                    f'const bool first_pass=pc=={pass_first_stop}u&&!passed_first;passed_first|=first_pass;'
                    'if(!first_pass&&(pc==stop||std::find(stop_entries.begin(),stop_entries.end(),pc)!=stop_entries.end()))',1)
            observer.write_text(source)
        return run(command,*args,**kwargs)
    context=physical_original.__globals__.copy();context['subprocess']=types.SimpleNamespace(run=launch)
    original=types.FunctionType(physical_original.__code__,context,physical_original.__name__,
                                physical_original.__defaults__,physical_original.__closure__)
    original.__kwdefaults__=physical_original.__kwdefaults__.copy()
    return original(image,rows,scratch,entries=(0x313bb0,)+tuple(children),regions=regions,
                    trace_calls=trace,trace_extended=True,preserve_x87_prefix=True)


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--report',type=Path,required=True);p.add_argument('--xbe',type=Path,required=True)
    p.add_argument('--compiler',default='c++');p.add_argument('--sanitizers',action='store_true');p.add_argument('--smoke',action='store_true')
    a=p.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):p.error('Run through tools/run_test.py')
    if platform.system()!='Linux' or platform.machine().lower()not in('x86_64','amd64','i386','i686') or not a.xbe.is_file():return 77
    scratch=Path(os.environ['HALO2_TEST_SCRATCH'])
    paths={DRIVER,Path(__file__),ROOT/'tools/observe_native_campaign_ai_vector_math_tu15.cpp'};pending=list(paths)
    while pending:
        path=pending.pop();source=path.read_text();names=re.findall(r'^#include "([^"\n]+)"',source,re.M)
        if path.suffix=='.py':names+=['tools/'+m+'.py'for m in re.findall(r'^(?:from|import) (\w+)',source,re.M)]
        for name in names:
            found=next((v for v in(path.parent/name,ROOT/name,ROOT/'include'/name,ROOT/'src'/name,ROOT/'tests'/name)if v.is_file()),None)
            if found is not None and found not in paths:paths.add(found);pending.append(found)
    pins={str(path.relative_to(ROOT)):sha(path)for path in sorted(paths)}
    report=dict(passed=False,production_bound=False,scratch_owner=str(scratch),source_sha256=pins,
                complete_owner=False,retained_large_artifacts=[])
    try:
        if sha(a.xbe)!=PIN:raise ValueError('Wrong actual TU1.5 original image')
        body=actual_body()
        if hashlib.sha256(body.encode()).hexdigest()!=BODY_PIN or hashlib.sha256(tetra_body().encode()).hexdigest()!=TETRA_PIN:
            raise ValueError('Complete original owner body drift')
        child_bodies();image=Xbe(a.xbe);rows=inputs(image,a.smoke)
        data=scratch/'inputs.json';data.write_text(json.dumps(rows,separators=(',',':')));report['input_sha256']=sha(data)
        sites={int(m[1],16)for m in re.finditer(r'services.instruction\(0x([0-9a-f]+)u\)',body)if parent(int(m[1],16))}
        io={int(m[1],16)for line in body.splitlines()if (m:=re.search(r'services.instruction\(0x([0-9a-f]+)u\)',line))
            and parent(int(m[1],16))and re.search(r'\b(?:r|v|w|vw)\(s.memory|s.push\(',line)}
        trace=[dict(entry=pc,arguments=0,gp=True,xmm=True)for pc in sorted(io)]
        trace.extend(dict(entry=pc,arguments=n,gp=True,xmm=True)for pc,n in CHILDREN.items())
        command=[a.compiler,'-std=c++20','-Wno-ignored-attributes','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math',
                 '-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(ROOT/'tests'),str(DRIVER),'-o',str(scratch/'search_setup')]
        if a.sanitizers:command[1:1]=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
        report['command']=command;built=subprocess.run(command,capture_output=True,text=True)
        if built.returncode:raise ValueError('Private hull search setup compile failed: '+built.stderr[-10000:])
        out=scratch/'native.json'
        with data.open('rb') as inp,out.open('wb') as output:
            done=subprocess.run([command[-1]],stdin=inp,stdout=output,stderr=subprocess.PIPE)
        if done.returncode:raise ValueError('Private hull search setup native stopped: '+done.stderr.decode()[-6000:])
        actual=json.loads(out.read_text())
        observed,report['original']=observe_with_fs(image,rows,scratch,trace)
        seen=set();counts={name:0 for name in('packets','children','reads','writes')}
        for case,got in zip(observed['cases'],actual,strict=True):
            row=rows[case['id']]
            if case.get('stopped_at')!=STOP or not case['live_x87_unchanged']:raise ValueError('Original escaped the private setup scope')
            calls=[];stores=[];packets=[];children=[]
            for event in case['writes']:
                pc=event['instruction']
                if pc in (0x313ea5,0x313b90,0x314e27,0x314eb2):
                    returned=struct.unpack('<I',bytes.fromhex(event['written']))[0]
                    if returned!=pc+5 or image.read(pc,1)!=b'\xe8':raise ValueError('Original child return instruction drift')
                    calls.append(returned)
                if not parent(pc):continue
                if pc not in VECTORS:stores.append(event);continue
                if len(event['before'])!=32 or len(event['written'])!=32:raise ValueError('Original setup MOVAPS width drift')
                stores.extend(dict(event,address=event['address']+n,before=event['before'][n*2:n*2+16],written=event['written'][n*2:n*2+16])for n in(0,8))
            for event in case['calls']:
                if event['entry'] in CHILDREN:children.append(dict(event,return_word=calls[len(children)]))
                else:packets.append({k:v for k,v in event.items()if k!='arguments'})
            if len(children)!=len(calls) or len(calls)!=7:raise ValueError('Original did not execute the actual complete tetrahedron family')
            want=dict(abi=dict(entry=STOP,gp=case['boundary_gp'],eflags=case['boundary_eflags'],xmm=case['boundary_xmm'],
                               fp=case['after_fp'],live_x87=row['live_x87']),packets=packets,children=children,
                      banks=sorted(case['banks'],key=lambda bank:bank['address']),
                      reads=[event for event in case['reads']if parent(event['instruction'])],writes=stores)
            for field,expected in want.items():
                if expected==got[field]:continue
                received=got[field];where=None
                if isinstance(expected,list):
                    where=next((n for n,(x,y) in enumerate(zip(expected,received))if x!=y),min(len(expected),len(received)))
                    expected,received=expected[where:where+1],received[where:where+1]
                if field=='banks' and expected and received:
                    x,y=expected[0]['after'],received[0]['after'];at=next(n for n,(a,b)in enumerate(zip(x,y))if a!=b)//2
                    expected,received=dict(address=expected[0]['address']+at,bits=x[at*2:at*2+16]),dict(bits=y[at*2:at*2+16])
                report['first_difference']=dict(case=row['name'],field=field,ordinal=where,expected=expected,actual=received)
                raise ValueError('Actual search setup differs: '+row['name']+' '+field)
            for key in counts:counts[key]+=len(want[key])
            seen.update(case['executed_sites'])
        report.update(cases=len(rows),parent_sites=len(sites),executed_parent_sites=len(seen&sites),missing_sites=sorted(sites-seen),
                      counts=counts,hostile_provider_comparisons=len(rows),original_text_unmodified=True,
                      supplied_child_outcomes=False,continuation=STOP)
        if sites-seen:raise ValueError('Incomplete actual search setup site coverage')
        report.update(passed=True,scope='Actual search setup prefixes through complete real Tetrahedron, Seed Facets and Plane. No retained source, full owner or physical fault closure.')
    except Exception as error:report['error']=str(error)
    report['sources_unchanged']=all(sha(ROOT/name)==value for name,value in pins.items())
    if not report['sources_unchanged']:report.update(passed=False,error='Private search setup source drift')
    a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({key:report[key]for key in('passed','error','first_difference','missing_sites')if key in report}))
    return 0 if report['passed']else 1

if __name__=='__main__':raise SystemExit(main())
