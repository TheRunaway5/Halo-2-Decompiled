#!/usr/bin/env python3
"""Compare genuine 3140DD solver restarts, perturbation and allocator entry prefixes."""
import argparse
import copy
import hashlib
import json
import os
import platform
import re
import struct
import subprocess
import types
from pathlib import Path

from validate_native_solver_hull_edge_insert_release_tu15 import EXTRA_REGIONS, allocator
from validate_native_solver_hull_search_iteration_frame_tu15 import (
    BASE, BODY_PIN, CHILDREN as ITERATION_CHILDREN, EXITS as ITERATION_EXITS, PIN, REGIONS as ITERATION_REGIONS,
    Xbe, actual_body, inputs as iteration_inputs, observe_with_fs, sha,
)
from validate_native_solver_hull_search_setup_frame_tu15 import FS_SITES

ROOT=Path(__file__).resolve().parents[1]
ENTRY,LAST=0x3140dd,0x3141b0
CHILDREN={0x313300:4,0x3139f0:0}
ALLOCATOR=0x149e00
EXITS={0x313be0,0x314494,ALLOCATOR}
VECTORS={0x31412e,0x314133}
DYNAMIC={0x31414a,0x314150,0x314152,0x314157,0x31415a,0x31415c,0x31415d,0x314164,0x314165,
         0x31417b,0x314181,0x314183,0x314188,0x31418a,0x31418d,0x31418e,0x314195,0x314196}
DRIVER=ROOT/'tests/native_tu15_solver_hull_search_refresh_frame_tests.cpp'


def parent(pc):
    return ENTRY<=pc<=LAST


def actual_before(image,scratch,with_allocator):
    # The source route naturally reaches 3140DD from actual masked-NaN
    # translation through a real Zero→Pair→Transpose→Clip chain.
    original=iteration_inputs(image,True)[-2:];out=scratch/'actual-before';out.mkdir()
    for n,row in enumerate(original):row['id']=n
    observed,proof=observe_with_fs(image,original,out,[],children=ITERATION_CHILDREN,
                       regions=ITERATION_REGIONS,pass_first_stop=0x313f63)
    pages=[];provenance=None
    if with_allocator:
        captures,provenance=allocator.reference_inputs(image)
        word=next(data[0x48c938-base:0x48c93c-base]for base,data in captures.items()
                  if base<=0x48c938 and 0x48c93c<=base+len(data))
        bank=bytearray(0x1000);bank[0x938:0x93c]=word
        for address,size in EXTRA_REGIONS:
            data=bytes(bank)if address==0x48c000 else image.read(address,size)
            pages.append(dict(address=address,before=data.hex()))
        backed={v['address']:bytes.fromhex(v['before'])for v in pages}
        if (struct.unpack('<I',word)[0],struct.unpack_from('<I',backed[0x486000],0x80)[0],
            struct.unpack_from('<I',backed[0x45f000],0x4e0)[0])!=(0x486080,0x45f4cc,ALLOCATOR):
            raise ValueError('Authentic allocator routing tuple drift')
    rows=[]
    for seed,case in zip(original,observed['cases'],strict=True):
        if case.get('stopped_at')!=ENTRY or not case['live_x87_unchanged']:
            raise ValueError('Actual chained solver did not reach refresh entry')
        sp=case['boundary_gp'][4];context=case['boundary_gp'][6]
        variants=[(attempt,'ordinary')for attempt in(0,1,2,18,19,20,0x7fffffff,0xffffffff)]
        variants.append((1,'configuration-aliases-attempt'))
        if with_allocator:variants.extend(((0,'payload-release'),(0,'vertex-release'),(1,'payload-release')))
        for attempt,kind in variants:
            row=copy.deepcopy(seed);banks=[dict(address=b['address'],before=b['after'])for b in case['banks']]
            main=next(bank for bank in banks if bank['address']==BASE);raw=bytearray.fromhex(main['before'])
            struct.pack_into('<I',raw,context+0x4c-BASE,attempt)
            if kind=='configuration-aliases-attempt':struct.pack_into('<I',raw,context+0x48-BASE,context+0x4c-0x80)
            if kind=='payload-release':struct.pack_into('<I',raw,sp+0x128-BASE,1)
            if kind=='vertex-release':struct.pack_into('<I',raw,sp+0x4a8-BASE,4)
            main['before']=raw.hex();words=struct.unpack_from('<3I',raw,sp-BASE)
            row.update(id=len(rows),name=f'refresh-{seed["id"]}-{attempt:x}-{kind}',entry=ENTRY,gp=case['boundary_gp'],
                       eflags=case['boundary_eflags'],xmm=case['boundary_xmm'],before_fp=case['after_fp'],
                       return_pc=words[0],matrix=words[1],source=words[2],stop_before_entries=sorted(EXITS),banks=banks+pages,
                       allocator_prefix=kind.endswith('-release'))
            rows.append(row)
    return rows,proof,provenance


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--report',type=Path,required=True);p.add_argument('--xbe',type=Path,required=True)
    p.add_argument('--compiler',default='c++');p.add_argument('--sanitizers',action='store_true');p.add_argument('--allocator-prefixes',action='store_true')
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
                complete_owner=False,allocator_returns_qualified=False,retained_large_artifacts=[])
    try:
        if sha(a.xbe)!=PIN:raise ValueError('Wrong actual TU1.5 original image')
        body=actual_body()
        if hashlib.sha256(body.encode()).hexdigest()!=BODY_PIN:raise ValueError('Complete original 313BB0 body drift')
        image=Xbe(a.xbe);rows,report['original_before'],report['allocator_provenance']=actual_before(image,scratch,a.allocator_prefixes)
        data=scratch/'inputs.json';data.write_text(json.dumps(rows,separators=(',',':')));report['input_sha256']=sha(data)
        sites={int(m[1],16)for m in re.finditer(r'services.instruction\(0x([0-9a-f]+)u\)',body)if parent(int(m[1],16))}
        io={int(m[1],16)for line in body.splitlines()if (m:=re.search(r'services.instruction\(0x([0-9a-f]+)u\)',line))
            and parent(int(m[1],16))and re.search(r'\b(?:r|v|w|vw)\(s.memory|s.push\(',line)}
        trace=[dict(entry=pc,arguments=0,gp=True,xmm=True)for pc in sorted(io)]
        trace.extend(dict(entry=pc,arguments=n,gp=True,xmm=True)for pc,n in CHILDREN.items())
        command=[a.compiler,'-std=c++20','-Wno-ignored-attributes','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math',
                 '-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(ROOT/'tests'),str(DRIVER),'-o',str(scratch/'search_refresh')]
        if a.sanitizers:command[1:1]=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
        report['command']=command;built=subprocess.run(command,capture_output=True,text=True)
        if built.returncode:raise ValueError('Private hull refresh compile failed: '+built.stderr[-10000:])
        out=scratch/'native.json'
        with data.open('rb') as inp,out.open('wb') as output:
            done=subprocess.run([command[-1]],stdin=inp,stdout=output,stderr=subprocess.PIPE)
        if done.returncode:raise ValueError('Native hull refresh stopped: '+done.stderr.decode()[-6000:])
        actual=json.loads(out.read_text())
        globals_=observe_with_fs.__globals__.copy()
        globals_['FS_SITES']=tuple(sorted((set(FS_SITES)-{0x313ab7,0x313abe,0x313b9c})|{0x3139f7,0x3139fe,0x313a9b}))
        scoped=types.FunctionType(observe_with_fs.__code__,globals_,observe_with_fs.__name__,
                                  observe_with_fs.__defaults__,observe_with_fs.__closure__)
        scoped.__kwdefaults__=observe_with_fs.__kwdefaults__.copy()
        regions=ITERATION_REGIONS+tuple(EXTRA_REGIONS if a.allocator_prefixes else ())
        observed,report['original']=scoped(image,rows,scratch,trace,children=CHILDREN,regions=regions)
        seen=set();exits={};counts={name:0 for name in('packets','children','reads','writes')};allocations=[]
        for case,got in zip(observed['cases'],actual,strict=True):
            row=rows[case['id']];stop=case.get('stopped_at')
            if stop not in EXITS or not case['live_x87_unchanged']:raise ValueError('Original escaped private refresh scope')
            calls=[];stores=[];packets=[];children=[]
            for event in case['writes']:
                pc=event['instruction']
                if pc in(0x31411d,0x3141ab,0x314165,0x314196):
                    returned=struct.unpack('<I',bytes.fromhex(event['written']))[0]
                    width=3 if pc in(0x314165,0x314196)else 5
                    if returned!=pc+width:raise ValueError('Original refresh child return instruction drift')
                    calls.append(returned)
                if not parent(pc):continue
                if pc not in VECTORS:stores.append(event);continue
                if len(event['before'])!=32 or len(event['written'])!=32:raise ValueError('Original refresh MOVAPS width drift')
                stores.extend(dict(event,address=event['address']+n,before=event['before'][n*2:n*2+16],written=event['written'][n*2:n*2+16])for n in(0,8))
            for event in case['calls']:
                if event['entry'] in CHILDREN:children.append(dict(event,return_word=calls[len(children)]))
                else:packets.append({k:v for k,v in event.items()if k!='arguments'})
            abi=dict(entry=stop,gp=case['boundary_gp'],eflags=case['boundary_eflags'],xmm=case['boundary_xmm'],
                     fp=case['after_fp'],live_x87=row['live_x87'])
            if stop==ALLOCATOR:
                sp=abi['gp'][4];main=next(v for v in case['banks']if v['address']==BASE)
                returned,*args=struct.unpack_from('<4I',bytes.fromhex(main['after']),sp-BASE)
                if returned!=calls[len(children)] or args[2]!=0x12:raise ValueError('Original allocator prefix ABI drift')
                children.append(dict(abi,arguments=args,return_word=returned))
                allocations.append(dict(case=row['name'],return_pc=returned,arguments=args))
            if len(children)!=len(calls):raise ValueError('Unobserved genuine refresh descendant CALL')
            want=dict(abi=abi,packets=packets,children=children,banks=sorted(case['banks'],key=lambda bank:bank['address']),
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
                raise ValueError('Actual refresh differs: '+row['name']+' '+field)
            for key in counts:counts[key]+=len(want[key])
            seen.update(case['executed_sites']);exits[stop]=exits.get(stop,0)+1
        missing=sites-seen
        report.update(cases=len(rows),parent_sites=len(sites),executed_parent_sites=len(seen&sites),missing_sites=sorted(missing),
                      counts=counts,original_text_unmodified=True,supplied_child_outcomes=False,continuations=exits,
                      allocator_entry_packets=allocations,hostile_provider_comparisons=len(rows))
        if missing!=(set()if a.allocator_prefixes else DYNAMIC):raise ValueError('Unexpected refresh site coverage')
        report.update(passed=True,scope='Actual refresh after genuine search, Sample/Cleanup returns, and optional authenticated allocator entry only. No full allocation, source, whole owner or fault closure.')
    except Exception as error:report['error']=str(error)
    report['sources_unchanged']=all(sha(ROOT/name)==value for name,value in pins.items())
    if not report['sources_unchanged']:report.update(passed=False,error='Private hull refresh source drift')
    a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({key:report[key]for key in('passed','error','first_difference','missing_sites','continuations')if key in report}))
    return 0 if report['passed']else 1

if __name__=='__main__':raise SystemExit(main())
