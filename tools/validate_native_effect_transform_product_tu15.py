#!/usr/bin/env python3
"""Prove Effect145560's shared native kernel against unchanged i386 instructions."""
import argparse
import copy
import hashlib
import json
import os
from pathlib import Path
import random
import struct
import subprocess
import sys
from native_effect_slot_scan_replacements import body
import native_effect_transform_product_replacements as binding
from validate_native_tu15_prt_helpers import PIN,physical_original

ROOT=Path(__file__).resolve().parents[1]
HEADER='src/gameplay/weapons/native_weapon_marker_math_tu15_internal.hpp'
DISPATCH='src/gameplay/weapons/native_weapon_effect_dispatch_tu15.cpp'
REGIONS=((0x61000000,8192),(0x62000000,8192),(0x81000000,8192))
def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def corpus():
    rng=random.Random(0x145560);rows=[]
    special=(0,0x80000000,1,0x80000001,0x7fffff,0x800000,0x3f800000,0xbf800000,
             0x7f800000,0xff800000,0x7fc01234,0x7fc05678,0x7f801234,0xff805678,0x7f7fffff,0x00800001)
    values=('0000000000000080ff3f','123456789abcde80fe3f','00000000000000800040','00000000000000c0ff7f',
            '01000000000000000000','00000000000000800100','abcdefabcdefab90ff3f','00000000000000000080')
    for n in range(192):
        banks=[bytearray(rng.randbytes(size))for _,size in REGIONS];stack=0x62001000+n%4
        a=0x61000100+(n%4);b=0x61000400+((n//4)%4);out=0x61000800+n%4
        mode=n%16
        if mode==1:out=a
        elif mode==2:out=b
        elif mode==3:b=a;out=a
        elif mode==4:out=a+4
        elif mode==5:out=b+8
        elif mode==6:b=a+4
        elif mode==7:out=a+28
        elif mode==8:a=stack+4
        elif mode==9:b=stack+4
        elif mode==10:out=stack+4
        elif mode==11:a=stack-48;out=stack+4
        elif mode==12:a=0x81000101;b=0x81000302;out=0x81000603
        elif mode==13:out=a-4
        elif mode==14:b=a+12;out=a+16
        elif mode==15:a=stack-20;b=stack+12
        def put(at,value):
            for (base,size),bank in zip(REGIONS,banks):
                if base<=at<=base+size-4:struct.pack_into('<I',bank,at-base,value);return
            raise ValueError('Product fixture escaped bank')
        def get(at):
            for (base,size),bank in zip(REGIONS,banks):
                if base<=at<=base+size-4:return struct.unpack_from('<I',bank,at-base)[0]
            raise ValueError('Product fixture lookup escaped bank')
        for at in (a,b):
            for lane in range(13):
                value=special[(n+lane*3)%len(special)]if n<128 else struct.unpack('<I',struct.pack('<f',rng.uniform(-8,8)))[0]
                put(at+lane*4,value)
        put(stack,0x63000000);put(stack+4,out)
        gp=[rng.getrandbits(32)for _ in range(8)];gp[1]=a;gp[2]=b;gp[4]=stack
        depth=(0,1,4,8)[n%4];cw=0x7f|(0,0x200,0x300)[n%3]|((n%4)<<10)
        mxcsr=0x1f80|((n%4)<<13)|(0x40 if n%8>=4 else 0)|(0x8000 if n%16>=8 else 0)|(n%64)
        rows.append(dict(id=n,name=f'product-{n}-alias{mode}',entry=0x145560,gp=gp,eflags=0x202|(rng.getrandbits(12)&0xcd5),
            matrix=out,source=get(stack+8),before_fp=[cw,((n%8)<<11)|(n%64),mxcsr],live_x87=list(values[:depth]),
            xmm=[[rng.getrandbits(32)for _ in range(4)]for _ in range(8)],max_steps=100,
            banks=[dict(address=base,before=bank.hex())for (base,_),bank in zip(REGIONS,banks)]))
    return rows


def sources(scratch,require_binding):
    header=(ROOT/HEADER).read_text();dispatch=(ROOT/DISPATCH).read_text();native=binding.install_owner(header,dispatch)
    if require_binding and (native!=header or binding.INCLUDE not in dispatch):raise ValueError('Effect product default binding missing')
    if binding.install_owner(native,dispatch)!=native:raise ValueError('Effect product binding not idempotent')
    rejected=0
    for shape in (header,native):
        current=body(shape,'marker_transform_product')
        for old,new in (('a.gp[2]+40','a.gp[2]+44'),('0x1455d2,a.gp[0]+44','0x1455d2,a.gp[0]+40')):
            changed=shape.replace(current,current.replace(old,new,1),1)
            if changed==shape:raise ValueError('Missing product body mutation witness')
            try:binding.install_owner(changed,dispatch)
            except ValueError:rejected+=1
            else:raise ValueError('Accepted complete product body mutation')
    for name in ('native_weapon_effect_dispatch_tu15_run','native_weapon_effect_dispatch_tu15_owns_entry'):
        current=body(dispatch,name);changed=dispatch.replace(current,current.replace('0x145560u','0x145561u',1),1)
        try:binding.install_owner(header,changed)
        except ValueError:rejected+=1
        else:raise ValueError('Accepted Effect product entry route mutation')
    (scratch/'effect_marker_math.inc').write_text(native)
    if binding.INCLUDE not in dispatch:dispatch=binding.INCLUDE+dispatch
    dispatch=dispatch.replace('#include "native_weapon_marker_math_tu15_internal.hpp"','#include "effect_marker_math.inc"')
    (scratch/'effect.cpp').write_text(dispatch)
    return rejected


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--xbe',type=Path,required=True);p.add_argument('--report',type=Path,required=True)
    p.add_argument('--sanitizers',action='store_true');p.add_argument('--require-binding',action='store_true');p.add_argument('--compiler',default='c++');args=p.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):p.error('Run through tools/run_test.py')
    if not args.xbe.is_file():print('SKIP missing original image');return 77
    if sha(args.xbe)!=PIN:raise ValueError('Wrong original TU1.5 image')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);report=dict(passed=False,production_bound=args.require_binding,scratch_owner=str(scratch),retained_large_artifacts=[])
    try:
        rejected=sources(scratch,args.require_binding)
        from oracle_smg_firing_1_1 import Xbe
        import oracle_native_campaign_screen_lifecycle_tu15 as common
        original_metadata=common.metadata
        def metadata(image):
            result=original_metadata(image);code=common._Code(image,'.text')
            for site in result['sites']:
                instruction=code.at(site['instruction'])
                if instruction.mnemonic=='ret':site['operands'].append(dict(base=4,index=-1,scale=1,displacement=0,bytes=4,read=True,write=False))
                # Capstone omits the access flag on these four MOVHPS stores.
                # Authenticate the genuine store opcode; no instructions change.
                if site['instruction']in (0x1455d2,0x145605,0x145638,0x145666):
                    if instruction.mnemonic!='movhps'or bytes(instruction.bytes)[:2]!=b'\x0f\x17'or len(site['operands'])!=1:
                        raise ValueError('Original145560 MOVHPS store metadata drift')
                    site['operands'][0].update(read=False,write=True,bytes=8)
            return result
        common.ENTRIES=(0x145560,);common.metadata=metadata;image=Xbe(args.xbe);site_metadata=metadata(image)
        sites=sorted(s['instruction']for s in site_metadata['sites']if s['operands'])
        read_sites={str(s['instruction']):True for s in site_metadata['sites']if any(o['read']for o in s['operands'])}
        rows=corpus();physical,proof=physical_original(image,rows,scratch,entries=(0x145560,),regions=REGIONS,
            trace_calls=[dict(entry=pc,arguments=0,gp=True,xmm=True,x87_delta=0)for pc in sites],trace_extended=True,preserve_x87_prefix=True)
        executed=set().union(*(case['executed_sites']for case in physical['cases']))
        if len(executed)!=69:raise ValueError('Product original site coverage incomplete')
        prefixes=[];by_id={row['id']:row for row in rows}
        for case in physical['cases'][:8]:
            for phase,events in (('before_read',case['reads']),('before_write',case['writes']),('after_write',case['writes'])):
                for ordinal in range(len(events)):
                    row=copy.deepcopy(by_id[case['id']]);row.update(id=len(rows)+len(prefixes),seed_id=case['id'],name=row['name']+'-'+phase+'-'+str(ordinal),fault=dict(phase=phase,ordinal=ordinal));prefixes.append(row)
        prefix_dir=scratch/'prefix';prefix_dir.mkdir();prefix_observed,prefix_proof=physical_original(image,prefixes,prefix_dir,entries=(0x145560,),regions=REGIONS,preserve_x87_prefix=True)
        inputs={row['id']:row for row in rows+prefixes};cases=[]
        for observed in physical['cases']+prefix_observed['cases']:
            row=copy.deepcopy(inputs[observed['id']]);row['completed']=observed['completed'];row['read_sites']=read_sites
            row['final']=dict(gp=observed['boundary_gp'],eflags=observed['boundary_eflags'],xmm=observed['boundary_xmm'],fp=observed['after_fp'])
            row['last_pc']=0x14566e if observed['completed']else observed['boundary']['instruction']
            row['reads']=observed['reads'];row['writes']=observed['writes']
            if not observed['live_x87_unchanged']:raise ValueError('Product physical live80 topology was not verified')
            if 'seed_id'not in row:
                row['io_states']={str(phase['entry']):dict(gp=phase['gp'],eflags=phase['eflags'],xmm=phase['xmm'],fp=phase['fp'])for phase in observed['calls']}
            for before,after in zip(row['banks'],observed['banks'],strict=True):before['after']=after['after']
            cases.append(row)
        fixture=scratch/'fixture.json';fixture.write_text(json.dumps(dict(cases=cases),separators=(',',':')))
        binary=scratch/'product-tests';command=[args.compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math',
            '-ffunction-sections','-fdata-sections','-Wl,--gc-sections','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(ROOT/'src/gameplay/weapons'),'-I'+str(scratch),
            str(ROOT/'tests/native_tu15_effect_transform_product_tests.cpp'),str(scratch/'effect.cpp'),'-o',str(binary)]
        if args.sanitizers:command[1:1]=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
        built=subprocess.run(command,capture_output=True,text=True,timeout=180)
        if built.returncode:raise ValueError('Product test compilation failed: '+built.stderr[:5000])
        run=subprocess.run([str(binary),str(fixture)],capture_output=True,text=True,timeout=120)
        if run.returncode:raise ValueError('Product native comparison failed: '+run.stdout[-1000:]+run.stderr[-4000:])
        counts=json.loads(run.stdout);forced=[]
        for control in ('HALO2_REFERENCE_EFFECT_TRANSFORM_PRODUCT','HALO2_REFERENCE_MATRIX_LEAVES','HALO2_VERIFY_MATRIX_LEAVES'):
            result=subprocess.run([str(binary),str(fixture)],capture_output=True,text=True,timeout=60,env=dict(os.environ,HALO2_EFFECT_PRODUCT_REFUSAL_TEST='1',**{control:'1'}))
            if result.returncode:raise ValueError('Product forced reference failed: '+control+' '+result.stderr[-2000:])
            forced.append(dict(control=control,**json.loads(result.stdout)))
        files=(HEADER,DISPATCH,'src/gameplay/weapons/native_tu15_effect_transform_product.hpp','src/physics/runtime/native_tu15_matrix_leaves.hpp',
            'tools/native_effect_transform_product_replacements.py','tools/validate_native_effect_transform_product_tu15.py','tests/native_tu15_effect_transform_product_tests.cpp')
        report.update(**counts,physical_original=proof,physical_prefix=prefix_proof,normal_cases=len(rows),prefix_cases=len(prefixes),original_sites=69,
            forced_reference=forced,authentication_rejections=rejected,admission_refusals=9,compiler_command=command,
            retained_reference_normal_comparisons=len(rows),native_normal_routes=4,native_prefix_routes=2,
            original_body=dict(bytes=0x111,sha256=hashlib.sha256(image.read(0x145560,0x111)).hexdigest()),
            source_sha256={path:sha(ROOT/path)for path in files},address_undefined_sanitizers=args.sanitizers,
            physical_implicit_ret_read=True,partial_instruction_write_faults_qualified=False,arbitrary_observer_mutation_qualified=False)
        print(run.stdout.strip())
    except BaseException as error:report['error']=str(error);raise
    finally:args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')
    return 0
if __name__=='__main__':raise SystemExit(main())
