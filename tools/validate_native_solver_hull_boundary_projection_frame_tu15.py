#!/usr/bin/env python3
"""Replay genuine hull boundary projection with actual Barycentric and Facet Select."""
import argparse
import copy
import hashlib
import json
import os
import platform
import re
import struct
import subprocess
from pathlib import Path

from validate_native_solver_hull_search_iteration_frame_tu15 import (
    BASE, BODY_PIN, CHILDREN as ITERATION_CHILDREN, PIN, REGIONS as ITERATION_REGIONS,
    Xbe, actual_body, inputs as iteration_inputs, observe_with_fs, sha,
)

ROOT=Path(__file__).resolve().parents[1]
ENTRY,STOP=0x3144a0,0x3145ee
CHILDREN={0x3122f0:5,0x314b20:1,0x314870:0}
REGIONS=(*ITERATION_REGIONS,(0x48c000,0x1000))
VECTORS={0x3144a9,0x3144b8,0x3144c7,0x314554,0x314565,0x31458c,0x3145a1,0x3145c8,0x3145dd}
DRIVER=ROOT/'tests/native_tu15_solver_hull_boundary_projection_frame_tests.cpp'


def parent(pc):
    return ENTRY<=pc<STOP


def actual_before(image,scratch,smoke):
    seeds=iteration_inputs(image,smoke);owner=scratch/'actual-before';owner.mkdir()
    for k in range(12):
        row=copy.deepcopy(seeds[k]);raw=bytearray.fromhex(row['banks'][2]['before'])
        center=((1.,1.,1.),(2.,.5,3.),(-2.,-1.,.5),(.1,3.,-2.))[k%4]
        tetra=((-1.,-.2,-.5),(2.,0.,0.),(0.,.5,0.),(0.,0.,3.)) if k&4 else ((0.,0.,0.),(1.,0.,0.),(0.,1.,0.),(0.,0.,1.))
        for n,point in enumerate(tetra):
            vertex=tuple(point[i]+center[i] for i in range(3))
            struct.pack_into('<4f',raw,0x4500+n*16,*vertex,0.)
            struct.pack_into('<4f',raw,0x4800+n*16,*vertex,0.)
        row['banks'][2]['before']=raw.hex();row.update(id=len(seeds),name=f'off-origin-tetrahedron-{k}')
        seeds.append(row)
    # Keep every source case's original dense payload but leave the four
    # slots required by the actual Barycentric child free.
    for row in seeds:row['live_x87']=row['live_x87'][:4]
    observed,proof=observe_with_fs(image,seeds,owner,[],children=ITERATION_CHILDREN,
                       regions=ITERATION_REGIONS,pass_first_stop=0x313f63)
    rows=[];origin=dict(address=0x48c000,before=image.read(0x48c000,0x1000).hex())
    if origin['before'][0xa00*2:0xa10*2]!='00'*16:raise ValueError('Actual projection origin drift')
    for row,case in zip(seeds,observed['cases'],strict=True):
        if case.get('stopped_at')!=ENTRY:continue
        if not case['live_x87_unchanged']:raise ValueError('Original search did not preserve boundary x87')
        sp=case['boundary_gp'][4];banks=[dict(address=b['address'],before=b['after'])for b in case['banks']]
        main=next(b for b in banks if b['address']==BASE)
        words=struct.unpack_from('<3I',bytes.fromhex(main['before']),sp-BASE)
        row.update(id=len(rows),name='boundary-'+row['name'],entry=ENTRY,gp=case['boundary_gp'],
                   eflags=case['boundary_eflags'],xmm=case['boundary_xmm'],before_fp=case['after_fp'],
                   return_pc=words[0],matrix=words[1],source=words[2],stop_before_entries=[STOP],banks=banks+[origin])
        rows.append(row)
    if not rows:raise ValueError('Original search never reached boundary projection')
    return rows,proof


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
        path=pending.pop();text=path.read_text();names=re.findall(r'^#include "([^"\n]+)"',text,re.M)
        if path.suffix=='.py':names+=['tools/'+m+'.py'for m in re.findall(r'^(?:from|import) (\w+)',text,re.M)]
        for name in names:
            found=next((v for v in(path.parent/name,ROOT/name,ROOT/'include'/name,ROOT/'src'/name,ROOT/'tests'/name)if v.is_file()),None)
            if found is not None and found not in paths:paths.add(found);pending.append(found)
    pins={str(path.relative_to(ROOT)):sha(path)for path in sorted(paths)}
    report=dict(passed=False,production_bound=False,scratch_owner=str(scratch),source_sha256=pins,
                complete_owner=False,retained_large_artifacts=[])
    try:
        if sha(a.xbe)!=PIN:raise ValueError('Wrong actual TU1.5 original image')
        body=actual_body()
        if hashlib.sha256(body.encode()).hexdigest()!=BODY_PIN:raise ValueError('Complete original 313BB0 body drift')
        image=Xbe(a.xbe);rows,report['original_before']=actual_before(image,scratch,a.smoke)
        data=scratch/'inputs.json';data.write_text(json.dumps(rows,separators=(',',':')));report['input_sha256']=sha(data)
        sites={int(m[1],16)for m in re.finditer(r'services.instruction\(0x([0-9a-f]+)u\)',body)if parent(int(m[1],16))}
        io={int(m[1],16)for line in body.splitlines()if (m:=re.search(r'services.instruction\(0x([0-9a-f]+)u\)',line))
            and parent(int(m[1],16))and re.search(r'\b(?:r|v|w|vw)\(s.memory|s.push\(',line)}
        trace=[dict(entry=pc,arguments=0,gp=True,xmm=True,x87_delta=int(pc in(0x3144e9,0x3144fd,0x31450e)))for pc in sorted(io)]
        trace.extend(dict(entry=pc,arguments=n,gp=True,xmm=True)for pc,n in CHILDREN.items())
        command=[a.compiler,'-std=c++20','-Wno-ignored-attributes','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math',
                 '-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(ROOT/'tests'),str(DRIVER),'-o',str(scratch/'boundary_projection')]
        if a.sanitizers:command[1:1]=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
        report['command']=command;built=subprocess.run(command,capture_output=True,text=True)
        if built.returncode:raise ValueError('Private hull boundary projection compile failed: '+built.stderr[-10000:])
        out=scratch/'native.json'
        with data.open('rb') as inp,out.open('wb') as output:
            done=subprocess.run([command[-1]],stdin=inp,stdout=output,stderr=subprocess.PIPE)
        if done.returncode:raise ValueError('Native hull boundary projection stopped: '+done.stderr.decode()[-6000:])
        actual=json.loads(out.read_text())
        # No FS access is reached between these two actual parent boundaries.
        # The shared original observer sees complete backing, with no need
        # for the private FS bridge used by the authentic before state.
        from validate_native_tu15_prt_helpers import physical_original
        observed,report['original']=physical_original(image,rows,scratch,entries=(0x313bb0,)+tuple(CHILDREN),
                              regions=REGIONS,trace_calls=trace,trace_extended=True,preserve_x87_prefix=True)
        seen=set();counts={name:0 for name in('packets','children','reads','writes')};selected=[]
        for case,got in zip(observed['cases'],actual,strict=True):
            row=rows[case['id']]
            if case.get('stopped_at')!=STOP or not case['live_x87_unchanged']:raise ValueError('Original escaped private boundary projection scope')
            calls=[];stores=[];packets=[];children=[];advances=0
            for event in case['writes']:
                pc=event['instruction']
                if pc in(0x3144e0,0x314523,0x314b2c):
                    returned=struct.unpack('<I',bytes.fromhex(event['written']))[0]
                    if returned!=pc+5 or image.read(pc,1)!=b'\xe8':raise ValueError('Actual projection child return drift')
                    calls.append(returned);advances+=int(pc==0x314523)
                if not parent(pc):continue
                if pc not in VECTORS:stores.append(event);continue
                if len(event['before'])!=32 or len(event['written'])!=32:raise ValueError('Original boundary projection MOVAPS width drift')
                stores.extend(dict(event,address=event['address']+n,before=event['before'][n*2:n*2+16],written=event['written'][n*2:n*2+16])for n in(0,8))
            for event in case['calls']:
                if event['entry'] in CHILDREN:children.append(dict(event,return_word=calls[len(children)]))
                else:packets.append({k:v for k,v in event.items()if k!='arguments'})
            if len(children)!=len(calls):raise ValueError('Unobserved genuine projection descendant CALL')
            selected.append(advances)
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
                raise ValueError('Actual boundary projection differs: '+row['name']+' '+field)
            for key in counts:counts[key]+=len(want[key])
            seen.update(case['executed_sites'])
        report.update(cases=len(rows),parent_sites=len(sites),executed_parent_sites=len(seen&sites),missing_sites=sorted(sites-seen),
                      counts=counts,original_text_unmodified=True,supplied_child_outcomes=False,facet_advances=selected,
                      hostile_provider_comparisons=len(rows))
        if not a.smoke and sites-seen:raise ValueError('Incomplete actual boundary projection site coverage')
        report.update(passed=True,scope='Actual boundary projection starts from unchanged search continuation and executes genuine Barycentric, Select and Clear children. No retained source or complete owner claim.')
    except Exception as error:report['error']=str(error)
    report['sources_unchanged']=all(sha(ROOT/name)==value for name,value in pins.items())
    if not report['sources_unchanged']:report.update(passed=False,error='Private boundary projection source drift')
    a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({key:report[key]for key in('passed','error','first_difference','missing_sites','facet_advances')if key in report}))
    return 0 if report['passed']else 1

if __name__=='__main__':raise SystemExit(main())
