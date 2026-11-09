#!/usr/bin/env python3
"""Check whole1E5AF0 transfers against unchanged returns and real126200 entries."""
import argparse
import copy
import hashlib
import json
import os
from pathlib import Path
import platform
import random
import re
import struct
import subprocess
import sys
from native_node_resource_replacements import canonical_body,replace_node_resource
from validate_native_tu15_prt_helpers import PIN,physical_original

ROOT=Path(__file__).resolve().parents[1]
SOURCES=('src/physics/physics_live_support_tu15_generated.cpp','src/campaign/ai/source_ai_actor_tu15_generated.cpp',
         'src/campaign/ai/source_ai_frame_tu15_generated.cpp')
REGIONS=((0x61000000,65536),(0x62000000,8192))
def digest(path):return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--xbe',type=Path,required=True)
    parser.add_argument('--report',type=Path,required=True);parser.add_argument('--sanitizers',action='store_true');parser.add_argument('--require-binding',action='store_true');args=parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):parser.error('Run through tools/run_test.py')
    if sys.platform!='linux'or platform.machine().lower()not in ('x86_64','amd64','i386','i686'):
        print('SKIP unchanged original node-resource requires Linux x86 ptrace');return 77
    if not args.xbe.is_file():print('SKIP missing original TU1.5 image');return 77
    if digest(args.xbe)!=PIN:raise ValueError('Original TU1.5 image differs')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);bodies=[];rejected=0
    pattern=r'static void run_1e5af0\(WorldState &s\) \{.*?\n\}'
    for name in SOURCES:
        source=(ROOT/name).read_text();body=re.search(pattern,source,re.S)[0];original=canonical_body(body)
        raw=source.replace(body,original,1);native=replace_node_resource(raw)
        if args.require_binding and native!=source:raise ValueError('Production node-resource binding absent: '+name)
        if replace_node_resource(native)!=native:raise ValueError('Node-resource binding is not idempotent')
        for victim in (raw,native):
            target=re.search(pattern,victim,re.S)[0]
            for old,new in (('run_126200(s)','run_126201(s)'),('0x1e5b24u','0x1e5b25u')):
                altered=target.replace(old,new,1)
                if altered==target:raise ValueError('Body mutation witness absent')
                try:replace_node_resource(victim.replace(target,altered,1))
                except ValueError:rejected+=1
                else:raise ValueError('Node-resource accepted complete body drift')
        if 'selected('in original:
            for old,new in (('case 0x30u:return 0x30u;','case 0x30u:return 0x34u;'),('switch(p)','p^=1u;switch(p)')):
                altered=raw.replace(old,new,1)
                if altered==raw:raise ValueError('Resolver mutation witness absent')
                try:replace_node_resource(altered)
                except ValueError:rejected+=1
                else:raise ValueError('Node-resource accepted complete resolver drift')
        literal=lambda text:re.sub(r'selected\((0x[0-9a-f]+u),s.image\)',r'\1',text)
        bodies.append((literal(original),literal(re.search(pattern,native,re.S)[0])))
    if any(body[0]!=bodies[0][0]for body in bodies):raise ValueError('Authenticated node-resource copies diverged')
    text=''.join('namespace '+name+' {\n'+body[1]+'\n}\n'for name,body in zip(('primary','actor','frame'),bodies,strict=True))
    text+='namespace reference {\n'+bodies[0][0]+'\n}\n';(scratch/'node_resource_bodies.inc').write_text(text)
    binary=scratch/'node-resource';command=['c++','-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math',
        '-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(scratch),str(ROOT/'tests/native_tu15_node_resource_tests.cpp'),'-o',str(binary)]
    if args.sanitizers:command[1:1]=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
    compiled=subprocess.run(command,capture_output=True,text=True)
    if compiled.returncode:raise ValueError('Node-resource native fixture compile failed: '+compiled.stderr[-4000:])
    rng=random.Random(0x1e5af0);rows=[]
    modes=('ordinary','unaligned','shared-index-table','shared-node-enable','caller-return-flag','node-table-stack','resource-table-stack','shared-descriptor-flags')
    indices=(0,1,7,0xfffffffe,0x80000000,0x7fffffff,0x40000000)
    for mode in modes:
      for index in indices:
       for flag,enabled in ((0,0),(1,1),(0x10,0),(0xff,0x80),(0x10,0xff)):
        ordinal=len(rows);banks=[bytearray(rng.randbytes(size))for _,size in REGIONS]
        def put(at,value,width=4):
            for (base,size),bank in zip(REGIONS,banks):
                if base<=at<=base+size-width:bank[at-base:at-base+width]=(value&((1<<(8*width))-1)).to_bytes(width,'little');return
            raise ValueError('Node-resource fixture escaped declared bank: '+hex(at))
        descriptor=0x61000100;node=0x61001000;resource=0x61003000;stack=0x62001000+ordinal%16
        resource_index=(0,1,3,0xffffffff,0x80000000,0x7fffffff,0x40000000)[ordinal%7]
        if mode=='unaligned':descriptor+=3;node+=1;resource+=3
        if mode=='node-table-stack':descriptor=stack-0x2c
        if mode=='resource-table-stack':descriptor=stack-0xac
        if mode=='shared-index-table':node=descriptor+0x8c;resource_index=(resource*pow(21,-1,1<<32))&0xffffffff
        if mode=='shared-node-enable':resource=node+6;enabled=flag
        if mode=='caller-return-flag':node=stack-0x16;flag=0
        if mode=='shared-descriptor-flags':node=descriptor+0x9a;resource=0x61003010;resource_index=0;flag=0x10
        nodes=(node-index*0x6c)&0xffffffff;resources=(resource-resource_index*20)&0xffffffff
        put(descriptor+0x30,nodes);put(node+0x16,flag,1);put(node+0x24,resource_index);put(descriptor+0xb0,resources);put(resource+0x10,enabled,1)
        # Two aliases intentionally share a DWORD/byte; their natural shared
        # value, rather than contradictory fixture writes, owns both reads.
        if mode=='shared-index-table':put(node+0x24,resource_index)
        if mode=='shared-descriptor-flags':put(descriptor+0xb0,resources)
        put(stack,0x63000000);matrix=nodes if mode=='node-table-stack'else resources;source=resource_index
        put(stack+4,matrix);put(stack+8,source)
        gp=[rng.getrandbits(32)for _ in range(8)];gp[0]=index;gp[2]=descriptor;gp[4]=stack
        flags=0x202|(rng.getrandbits(12)&0xcd5);mxcsr=0x1f80|((ordinal%4)<<13)|(0x8040 if ordinal&4 else 0)
        cw=(0x7f,0x27f,0x37f)[ordinal%3]|((ordinal%4)<<10);status=((ordinal%8)<<11)|((ordinal%8)<<8)|((ordinal>>4)&63)
        live_values=('0000000000000080ff3f','00000000000000000080','35c26821a2da0fc90040','45230100000000c0ff7f')
        rows.append(dict(id=ordinal,name=f'{mode}-{index:x}-{flag:x}-{enabled:x}',entry=0x1e5af0,gp=gp,eflags=flags,matrix=matrix,source=source,
            max_steps=1000,stop_before_entries=[0x126200],xmm=[[rng.getrandbits(32)for _ in range(4)]for _ in range(8)],before_fp=[cw,status,mxcsr],
            live_x87=list(live_values[:(0,1,4)[ordinal%3]]),banks=[dict(address=base,before=bank.hex())for (base,_),bank in zip(REGIONS,banks)]))
    sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'));from xbe import Xbe
    normal=scratch/'normal';normal.mkdir();observed,proof=physical_original(Xbe(args.xbe),rows,normal,entries=(0x1e5af0,),regions=REGIONS)
    prefix_rows=[]
    witnesses=[n for n,case in enumerate(observed['cases'])if len(case['reads'])>=4][:12]+[0,1]
    for n in witnesses:
        for ordinal in range(len(observed['cases'][n]['reads'])):
            row=copy.deepcopy(rows[n]);row.update(id=len(rows)+len(prefix_rows),name=row['name']+f'-before-read-{ordinal}',fault=dict(phase='before_read',ordinal=ordinal));prefix_rows.append(row)
    prefixes=scratch/'prefixes';prefixes.mkdir();prefix_observed,prefix_proof=physical_original(Xbe(args.xbe),prefix_rows,prefixes,entries=(0x1e5af0,),regions=REGIONS)
    merged=[]
    for request,expected in zip(rows+prefix_rows,observed['cases']+prefix_observed['cases'],strict=True):
        item={**request,**expected};after={bank['address']:bank['after']for bank in expected['banks']}
        item['banks']=[dict(bank,after=after[bank['address']])for bank in request['banks']];merged.append(item)
    fixture=scratch/'node-resource.json';fixture.write_text(json.dumps(dict(cases=merged),separators=(',',':')))
    environment=dict(os.environ);environment.pop('HALO2_REFERENCE_NODE_RESOURCE',None)
    result=subprocess.run([str(binary),str(fixture)],capture_output=True,text=True,env=environment)
    if result.returncode:raise ValueError('Native node-resource comparison failed: '+result.stdout+result.stderr)
    native=json.loads(result.stdout)
    result=subprocess.run([str(binary),str(fixture)],capture_output=True,text=True,env=dict(environment,HALO2_REFERENCE_NODE_RESOURCE='1',HALO2_NODE_RESOURCE_REFERENCE_TEST='1'))
    if result.returncode:raise ValueError('Retained node-resource reference comparison failed: '+result.stdout+result.stderr)
    reference=json.loads(result.stdout)
    sites={int(pc,16)for pc in re.findall(r's\.services\.instruction\((0x[0-9a-f]+)u\)',bodies[0][0])}
    executed={pc for row in observed['cases']for pc in row['executed_sites']}
    if sites-executed:raise ValueError('Original node-resource source sites unexecuted: '+str(sorted(sites-executed)))
    files=(*SOURCES,'include/halo2/native_tu15_node_resource_kernel.hpp','src/physics/runtime/native_tu15_node_resource.hpp',
        'tools/native_node_resource_replacements.py','tests/native_tu15_node_resource_tests.cpp','tools/validate_native_tu15_node_resource.py','tools/validate_native_tu15_prt_helpers.py')
    report=dict(passed=True,production_copies=3,physical_cases=len(rows),normal_return_cases=sum(row['stopped_at']==0x63000000 for row in observed['cases']),
        genuine_126200_entry_cases=sum(row['stopped_at']==0x126200 for row in observed['cases']),before_read_cases=len(prefix_rows),native=native,forced_reference=reference,
        compared_bank_bytes=3*sum(sum(len(bank['before'])//2 for bank in row['banks'])for row in merged),fixture_sha256=digest(fixture),
        physical_original=proof,physical_read_prefixes=prefix_proof,rejected_mutations=rejected,source_sha256={name:digest(ROOT/name)for name in files},
        source_body_coverage=dict(total=len(sites),executed=len(sites&executed)),full_flags_and_child_entry_abi_verified=True,
        ordered_explicit_source_reads_verified=True,return_implicit_read_hardware_metadata_included=False,
        supplemental_canonical_alias_cases=12,sentinel_low_page_scope='Two canonical backed-zero-page cases only; physical normal cases require valid readable node fields.',
        supplied_child_outcomes=False,real_126200_lifecycle_closed=False,physical_page_faults_qualified=False,address_undefined_sanitizers=args.sanitizers,
        whole_engine_native_closure=False,scratch_directory=str(scratch),retained_large_artifacts=[])
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    print(f'PASS node-resource {native["comparisons"]} all-copy transfers including {native["genuine_child_entries"]} genuine child entries');return 0


if __name__=='__main__':
    try:raise SystemExit(main())
    except Exception as error:
        if '--report'in sys.argv:
            output=Path(sys.argv[sys.argv.index('--report')+1]);output.parent.mkdir(parents=True,exist_ok=True)
            output.write_text(json.dumps(dict(passed=False,error=repr(error),scratch_directory=os.environ.get('HALO2_TEST_SCRATCH'),retained_large_artifacts=[]),indent=2)+'\n')
        raise
