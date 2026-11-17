#!/usr/bin/env python3
"""Compare four actual 313BB0 seed transitions with real nested native child bodies."""
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

from research_mp_sandbox_update_functions import Xbe
from validate_native_solver_hull_basis_frame_tu15 import inputs as basis_inputs
from validate_native_solver_hull_edge_frame_tu15 import inputs as edge_inputs
from validate_native_solver_hull_initialize_frame_tu15 import inputs as initialize_inputs
from validate_native_solver_hull_line_frame_tu15 import inputs as line_inputs
from validate_native_solver_hull_seed_route_frame_tu15 import BODY_PIN, actual_body
from validate_native_tu15_prt_helpers import PIN, physical_original

ROOT=Path(__file__).resolve().parents[1]
BASE=0x61000000
ENTRIES=(0x313db2,0x313dcf,0x313de0,0x313df1)
EXITS=(0x313d69,0x313d14,0x313c89,0x313bfc,0x31445f,0x314353)
CHILDREN={0x313560:2,0x3136a0:0,0x313790:0,0x313470:3,0x313390:2,0x313290:1,
          0x1f6610:2,0x2e8d20:2,0x306790:2}
REGIONS=((BASE,0x10000),(0x44a000,0x1000),(0x461000,0x2000),(0x46a000,0x1000))
FIRST,LAST=ENTRIES[0],0x313e89
VECTOR_STORES={0x313e4a,0x313e61,0x313e7c}
DRIVER=ROOT/'tests/native_tu15_solver_hull_seed_transition_frame_tests.cpp'


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def parent(pc):
    return FIRST<=pc<=LAST


def inputs(image,smoke):
    collections=[initialize_inputs(image),line_inputs(image),basis_inputs(image),edge_inputs(image)]
    constants=dict(address=0x46a000,before=image.read(0x46a000,0x1000).hex())
    rows=[]
    for route,collection in enumerate(collections):
        selectors=list(range(3 if smoke else 24))
        if route==0: selectors.extend(range(len(collection)-4,len(collection)-1))
        if route==3: selectors.append(-1)
        for i in selectors:
            row=copy.deepcopy(collection[i]);raw=bytearray.fromhex(row['banks'][0]['before'])
            gp=row['gp'];child_sp=gp[4];context=gp[1];bp=BASE+0xf000
            sp=child_sp+(12,4,4,16)[route]
            def put(address,*words):struct.pack_into('<'+'I'*len(words),raw,address-BASE,*words)
            gp[4],gp[5],gp[6]=sp,bp,context
            if route==0:
                put(bp+8,row['source'])
            if route==3:
                sp=(sp+15)&~15;gp[4]=sp
                raw[sp+0x60-BASE:sp+0x70-BASE]=raw[row['matrix']-BASE:row['matrix']+16-BASE]
                put(context+0x48,BASE+0x7000);put(context+0x50,row['source']);put(BASE+0x7080,3)
                if i==-1:
                    for n in range(4):
                        put(context+16*n,*(0x3f800000 if n==j and n<3 else 0 for j in range(4)))
                    put(BASE+0x5000,0x4613a0);put(BASE+0x6000,0x4613a0)
                    put(row['source'],0,0,0,0)
            # The common observer reseeds only these exact existing outer
            # frame words. The child builds its own genuine CALL frame.
            words=struct.unpack_from('<3I',raw,sp-BASE)
            row.update(id=len(rows),entry=ENTRIES[route],name=f'transition-{route}-{row["name"]}',
                       return_pc=words[0],matrix=words[1],source=words[2],stop_before_entries=list(EXITS),
                       max_steps=14000,live_x87=row['live_x87'][:5])
            row['banks'][0]['before']=raw.hex()
            if not any(bank['address']==constants['address'] for bank in row['banks']):row['banks'].append(constants)
            rows.append(row)
    return rows


def run_native(command,data,result):
    with data.open('rb') as inp,result.open('wb') as out:
        done=subprocess.run([command[-1]],stdin=inp,stdout=out,stderr=subprocess.PIPE)
    if done.returncode:raise ValueError('Native seed transition stopped: '+done.stderr.decode()[-6000:])
    return json.loads(result.read_text())


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--report',type=Path,required=True);p.add_argument('--xbe',type=Path,required=True)
    p.add_argument('--compiler',default='c++');p.add_argument('--sanitizers',action='store_true');p.add_argument('--smoke',action='store_true')
    p.add_argument('--mutations',action='store_true')
    a=p.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):p.error('Run through tools/run_test.py')
    if platform.system()!='Linux' or platform.machine().lower()not in('x86_64','amd64','i386','i686') or not a.xbe.is_file():return 77
    scratch=Path(os.environ['HALO2_TEST_SCRATCH'])
    paths={DRIVER,Path(__file__),ROOT/'tools/observe_native_campaign_ai_vector_math_tu15.cpp'}
    pending=list(paths)
    while pending:
        current=pending.pop()
        names=re.findall(r'^#include "([^"\n]+)"',current.read_text(),re.M)
        if current.suffix=='.py':names+=['tools/'+module+'.py' for module in re.findall(r'^from (\w+) import',current.read_text(),re.M)]
        for name in names:
            path=next((x for x in(current.parent/name,ROOT/name,ROOT/'include'/name,ROOT/'src'/name,ROOT/'tests'/name) if x.is_file()),None)
            if path is not None and path not in paths:paths.add(path);pending.append(path)
    pins={str(path.relative_to(ROOT)):sha(path)for path in sorted(paths)}
    report=dict(passed=False,production_bound=False,scratch_owner=str(scratch),source_sha256=pins,
                complete_owner=False,retained_large_artifacts=[])
    try:
        if sha(a.xbe)!=PIN:raise ValueError('Wrong TU1.5 original image')
        body=actual_body()
        if hashlib.sha256(body.encode()).hexdigest()!=BODY_PIN:raise ValueError('Complete actual 313BB0 body drift')
        image=Xbe(a.xbe);rows=inputs(image,a.smoke);data=scratch/'inputs.json';data.write_text(json.dumps(rows,separators=(',',':')))
        report['input_sha256']=sha(data)
        sites={int(m[1],16)for m in re.finditer(r'services.instruction\(0x([0-9a-f]+)u\)',body)if parent(int(m[1],16))}
        io={int(m[1],16)for line in body.splitlines()if (m:=re.search(r'services.instruction\(0x([0-9a-f]+)u\)',line))
            and parent(int(m[1],16))and re.search(r'\b(?:r|v|w|vw)\(s.memory|s.push\(',line)}
        trace=[dict(entry=pc,arguments=0,gp=True,xmm=True,x87_delta=int(pc==0x313e15)) for pc in sorted(io)]
        trace.extend(dict(entry=pc,arguments=n,gp=True,xmm=True)for pc,n in CHILDREN.items())
        command=[a.compiler,'-std=c++20','-Wno-ignored-attributes','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math',
                 '-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(ROOT/'tests'),str(DRIVER),'-o',str(scratch/'seed_transition')]
        if a.sanitizers:command[1:1]=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
        report['command']=command
        built=subprocess.run(command,capture_output=True,text=True)
        if built.returncode:raise ValueError('Private seed transition compile failed: '+built.stderr[-10000:])
        actual=run_native(command,data,scratch/'native.json')
        observed,report['original']=physical_original(image,rows,scratch,entries=(0x313bb0,)+tuple(CHILDREN),regions=REGIONS,
                        trace_calls=trace,trace_extended=True,preserve_x87_prefix=True)
        counts={name:0 for name in('packets','children','reads','writes')};seen=set();exits=set()
        for case,got in zip(observed['cases'],actual,strict=True):
            row=rows[case['id']];stop=case.get('stopped_at')
            if stop not in EXITS or not case['live_x87_unchanged']:raise ValueError('Original did not preserve transition caller scope')
            calls=[];stores=[];packets=[];children=[]
            for event in case['writes']:
                pc=event['instruction'];opcode=image.read(pc,1)[0]
                if opcode==0xe8 or pc in(0x3133ab,0x313425):
                    width=3 if pc in(0x3133ab,0x313425)else 5
                    returned=struct.unpack('<I',bytes.fromhex(event['written']))[0]
                    if returned!=pc+width:raise ValueError('Original child return store differs from actual instruction length')
                    calls.append(returned)
                if not parent(pc):continue
                if pc not in VECTOR_STORES:stores.append(event);continue
                if len(event['written'])!=32 or len(event['before'])!=32:raise ValueError('Expected original vector store width')
                stores.extend(dict(event,address=event['address']+n,before=event['before'][n*2:n*2+16],written=event['written'][n*2:n*2+16])for n in(0,8))
            for event in case['calls']:
                if event['entry'] in CHILDREN:
                    children.append(dict(event,return_word=calls[len(children)]))
                else:packets.append({k:v for k,v in event.items()if k!='arguments'})
            if len(children)!=len(calls):raise ValueError('Unobserved genuine descendant CALL')
            want=dict(abi=dict(entry=stop,gp=case['boundary_gp'],eflags=case['boundary_eflags'],xmm=case['boundary_xmm'],
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
                raise ValueError('Actual seed transition differs: '+row['name']+' '+field)
            for key in counts:counts[key]+=len(want[key])
            seen.update(case['executed_sites']);exits.add(stop)
        report.update(cases=len(rows),parent_sites=len(sites),executed_parent_sites=len(seen&sites),
                      missing_sites=sorted(sites-seen),continuations=sorted(exits),counts=counts,
                      original_text_unmodified=True,supplied_child_outcomes=False)
        if not a.smoke and (sites-seen or exits!=set(EXITS)):raise ValueError('Incomplete actual seed transition route coverage')
        if a.mutations:
            relative='physics/runtime/native_tu15_solver_hull_seed_transition_frame.hpp'
            text=(ROOT/'src'/relative).read_text();report['compiled_defects']={}
            controls=(
                ('wrong_initializer_status_byte','io_.byte(0x313dc2u,g[0])','io_.byte(0x313dc2u,g[0]+1u)'),
                ('wrong_projection_parity','return !s_.flags.pf;','return s_.flags.pf;'),
                ('wrong_triangle_count','flags.increment(Foundation::word(address),4u,false)','flags.increment(Foundation::word(address),4u,true)'),
            )
            for name,old,new in controls:
                if text.count(old)!=1:raise ValueError('Missing exact transition defect seam: '+name)
                owned=scratch/name;header=owned/relative;header.parent.mkdir(parents=True);header.write_text(text.replace(old,new,1))
                trial=command.copy();trial.insert(trial.index('-I'+str(ROOT/'include')),'-I'+str(owned));trial[-1]=str(owned/'seed_transition')
                built=subprocess.run(trial,capture_output=True,text=True,timeout=600)
                if built.returncode:raise ValueError('Compiled transition defect failed to build: '+name+' '+built.stderr[-5000:])
                tested=run_native(trial,data,owned/'result.json')
                first=next((n for n,(x,y)in enumerate(zip(actual,tested,strict=True))if x!=y),None)
                if first is None:raise ValueError('Unchanged original did not reject transition defect: '+name)
                field=next(k for k in actual[first]if actual[first][k]!=tested[first][k])
                report['compiled_defects'][name]=dict(rejected=True,first_case=rows[first]['name'],first_field=field)
        report.update(passed=True,scope='Actual seed-transition physical prefixes with complete child entry ABI and final backing. No retained source, whole owner, fault or default claim.')
    except Exception as error:report['error']=str(error)
    report['sources_unchanged']=all(sha(ROOT/name)==value for name,value in pins.items())
    if not report['sources_unchanged']:report.update(passed=False,error='Private seed transition source drift')
    a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({key:report[key]for key in('passed','error','first_difference','missing_sites')if key in report}))
    return 0 if report['passed']else 1

if __name__=='__main__':raise SystemExit(main())
