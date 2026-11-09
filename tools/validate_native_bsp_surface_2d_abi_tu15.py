#!/usr/bin/env python3
"""Unchanged-i386 full ABI and intermediate-state proof for native2A0230."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import random
import re
import struct
import subprocess
import sys
from native_bsp_surface_2d_replacements import canonical_body,replace_bsp_surface_2d
from validate_native_tu15_prt_helpers import PIN,physical_original

ROOT=Path(__file__).resolve().parents[1]
REGIONS=((0x61000000,65536),(0x61080000,8192),(0x62000000,8192))
COPIES=('src/campaign/ai/source_ai_frame_tu15_generated.cpp','src/gameplay/units/death_cycle_runtime_tu15_generated.cpp')
SITES=(0x2a0237,0x2a0238,0x2a0248,0x2a024e,0x2a0253,0x2a0258,0x2a0262,0x2a0275,0x2a027e,0x2a0289,0x2a028d)
def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def emit_abi(out,gp,flags,fp,xmm):
    out.write(struct.pack('<12I',*gp,flags,fp[2],fp[1],fp[0]))
    for lane in xmm:out.write(struct.pack('<4I',*lane))

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--xbe',type=Path,required=True)
    p.add_argument('--report',type=Path,required=True);p.add_argument('--sanitizers',action='store_true');p.add_argument('--require-binding',action='store_true');p.add_argument('--compiler',default='c++');args=p.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):p.error('Run through tools/run_test.py')
    if not args.xbe.is_file():print('SKIP missing original TU1.5 image');return 77
    if sha(args.xbe)!=PIN:raise ValueError('Wrong original TU1.5 image')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);bound=[];rejected=0
    for name,path in zip(('ai','death'),COPIES):
        source=(ROOT/path).read_text();body=re.search(r'static void run_2a0230\(WorldState &s\) \{.*?\n\}',source,re.S)[0]
        original=canonical_body(body);raw=source.replace(body,original,1);native=replace_bsp_surface_2d(raw)
        if args.require_binding and native!=source:raise ValueError('BSP2D native default binding missing: '+path)
        if replace_bsp_surface_2d(native)!=native:raise ValueError('BSP2D binding is not idempotent')
        selected=next(line for line in source.splitlines()if line.startswith('std::uint32_t selected('))
        bound.append('namespace '+name+' {\n'+replace_bsp_surface_2d(selected+'\n'+original)+'\n}\n')
        literal=re.sub(r'selected\((0x[0-9a-f]+u),s\.image\)',r'\1',original)
        if name=='ai':
            bound.append('namespace reference {\n'+literal+'\n}\n')
            bound.append('namespace literal {\n'+replace_bsp_surface_2d(literal)+'\n}\n')
        literal_source=raw.replace(original,literal,1)
        for victim in (raw,native,literal_source,replace_bsp_surface_2d(literal_source)):
            target=re.search(r'static void run_2a0230\(WorldState &s\) \{.*?\n\}',victim,re.S)[0]
            changed=victim.replace(target,target.replace('s.gp[0]+s.gp[1]*2u','s.gp[0]+s.gp[1]*4u',1),1)
            if changed==victim:raise ValueError('BSP2D body mutation witness missing')
            try:replace_bsp_surface_2d(changed)
            except ValueError:rejected+=1
            else:raise ValueError('BSP2D accepted changed whole body')
        changed=raw.replace('{if(image==SourcePhysicsContactImage::MapPack11)',
            '{if(p==0)return 1u;if(image==SourcePhysicsContactImage::MapPack11)',1)
        try:replace_bsp_surface_2d(changed)
        except ValueError:rejected+=1
        else:raise ValueError('BSP2D accepted changed resolver')
    (scratch/'bsp_surface_2d_bodies.inc').write_text('\n'.join(bound));binary=scratch/'bsp-surface-2d'
    command=[args.compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(scratch),str(ROOT/'tests/native_tu15_bsp_surface_2d_abi_tests.cpp'),'-o',str(binary)]
    if args.sanitizers:command+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
    binding=None
    if args.require_binding:
        from specialize_native_operands import specialize,BSP_SURFACE_2D_ACTUAL_OWNER_IMAGES
        source=(ROOT/'src/physics/physics_live_support_tu15_generated.cpp').read_text();specialized,receipt=specialize(source)
        if receipt.get('native_bsp_surface_2d_actual_owner_entries')!=['2a0230']:raise ValueError('BSP2D actual owner binding missing')
        for entry,(length,pin)in BSP_SURFACE_2D_ACTUAL_OWNER_IMAGES.items():
            sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'));from xbe import Xbe
            if hashlib.sha256(Xbe(args.xbe).read(entry,length)).hexdigest()!=pin:raise ValueError('BSP2D whole original owner drift')
        binding=dict(source_sha256=hashlib.sha256(source.encode()).hexdigest(),specialized_sha256=hashlib.sha256(specialized.encode()).hexdigest())
        dispatch=re.search(r'static void dispatch\(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}',source)[0]
        for replacement in (dispatch.replace('{','{if(target==0)return;',1),dispatch.replace('s.required(site,target,tail)','s.required(site,target^1u,tail)',1)):
            if replacement==dispatch:raise ValueError('BSP2D dispatch drift witness missing')
            try:specialize(source.replace(dispatch,replacement))
            except ValueError as error:
                if str(error)!='Complete13FE50 dispatcher drift':raise
            else:raise ValueError('BSP2D accepted actual dispatcher drift')
        binding['dispatcher_mutations_rejected']=2
        command+=['-DHALO2_BSP_2D_OWNER_DISPATCH',*[str(ROOT/'src/physics'/name)for name in ('native_tu15_array_allocator.cpp','native_tu15_stack_arena.cpp','native_tu15_main_heap_release.cpp','native_tu15_main_heap_allocate.cpp','native_tu15_raw_allocator.cpp','native_tu15_physical_epoch.cpp')]]
    subprocess.run(command,check=True)
    forced=subprocess.run([str(binary)],capture_output=True,text=True,env=dict(os.environ,HALO2_REFERENCE_BSP_SURFACE_2D='1',HALO2_BSP_SURFACE_2D_REFERENCE_TEST='1'))
    if forced.returncode:raise ValueError('BSP2D forced-reference proof failed: '+forced.stderr[-4000:])
    rng=random.Random(0x2a0230);rows=[];requests=[]
    points=((0,0),(0x3f800000,0x3f800000),(0xbf800000,0),(0x80000000,0),(1,0x80000001),(0x7f800000,0x3f800000),(0xff800000,0x7f800000),(0x7fc12345,0x7f812345),(0x7f812345,0x7fc12345))
    planes=((0x3f800000,0,0),(0,0x3f800000,0),(0x3f800000,0x3f800000,0x3f000000),(0x7fc12345,0x3f800000,0),(0x7f812345,0x3f800000,0),(0x7f800000,0,0),(1,0x80000001,0),(0x7f7fffff,0x3f800000,0),(0xff800000,0x7f800000,0x7fc12345))
    live_values=('0000000000000080ff3f','00000000000000000080','35c26821a2da0fc90040','45230100000000c0ff7f','0100000000000080ff7f','0000000000000080ff7f','0000000000000080ffff','01000000000000000000')
    for n in range(128):
        banks=[bytearray(rng.randbytes(size))for _,size in REGIONS]
        def put(at,value,width=4):
            for (base,size),bank in zip(REGIONS,banks):
                if base<=at<=base+size-width:bank[at-base:at-base+width]=(value&((1<<(width*8))-1)).to_bytes(width,'little');return
            raise ValueError('BSP2D fixture escaped bank')
        node=(0,3,0x12340001,0x7fff,0x80000000,0xffff0000,0x8000,0xffff8001,0xffff,0xffffffff)[n%10]
        depth=(1,2,4,16,64)[n%5];mode=('ordinary','unaligned','point-stack','descriptor-stack','nodes-stack','alternate','overflow','point-row')[n%8]
        sp=0x62001000+n%16;descriptor=0x61000100;nodes=0x61001000;point=0x6100f000
        if mode=='unaligned':descriptor+=1;nodes+=3;point+=2
        if mode=='point-stack':point=sp-4
        if mode=='descriptor-stack':descriptor=sp-8
        if mode=='nodes-stack':nodes=sp-4;node=0;depth=1
        if mode=='point-row':point=nodes+4
        if(node&0x7fff)==0x7fff:depth=1
        start=node&0x7fff;normal=planes[n%len(planes)];point_values=points[(n//3)%len(points)]
        if mode=='nodes-stack':point_values=(0,0xbf800000)
        for k in range(depth):
            row=nodes+(start+k)*16
            for lane,value in enumerate(normal):put(row+lane*4,value)
            put(row+12,start+k+1 if k+1<depth else 0xffff,2);put(row+14,start+k+1 if k+1<depth else 0x8000|(n&0x7fff),2)
        put(descriptor+4,nodes);put(point,point_values[0]);put(point+4,point_values[1]);put(sp,0x63000000);put(sp+4,descriptor);put(sp+8,point)
        gp=[rng.getrandbits(32)for _ in range(8)];gp[0]=node;gp[1]=descriptor;gp[2]=point;gp[4]=sp
        if mode=='descriptor-stack':gp[6]=nodes
        if mode=='nodes-stack':gp[6]=0x3f800000
        flags=0x202|(rng.getrandbits(12)&0xcd5);mxcsr=0x1f80|((n%4)<<13)|(0,0x40,0x8000,0x8040)[(n//4)%4]|(n%64)
        cw=(0x7f,0x27f,0x37f)[n%3]|((n%4)<<10);status=((n%8)<<11)|(n%64)|((n*0x100)&0x4700)
        live=list(live_values[:(0,1,4,8)[n%4]]);xmm=[[rng.getrandbits(32)for _ in range(4)]for _ in range(8)]
        rows.append(dict(id=n,name=f'{mode}-{node:x}-{depth}-{n}',entry=0x2a0230,gp=gp,eflags=flags,matrix=descriptor,source=point,xmm=xmm,before_fp=[cw,status,mxcsr],live_x87=live,max_steps=10000,
            banks=[dict(address=base,before=bank.hex())for (base,_),bank in zip(REGIONS,banks)]))
        requests.append((gp,flags,mxcsr,cw,status,live,xmm,banks))
    sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'));from xbe import Xbe
    image=Xbe(args.xbe);body_pin=hashlib.sha256(image.read(0x2a0230,0x5e)).hexdigest()
    if body_pin!='c362b64b0f3c686f7f63e8e15b269d313d89019acf82bcb33f6c78661f2cfa07':raise ValueError('Whole original2A0230 bytes drift')
    physical,proof=physical_original(image,rows,scratch,entries=(0x2a0230,),regions=REGIONS,
        trace_calls=[dict(entry=pc,arguments=0,gp=True,xmm=True,x87_delta=0)for pc in SITES],trace_extended=True)
    inputs,expected,actual=(scratch/name for name in ('tree.in','tree.expected','tree.actual'));receipts=[];copies=tuple(range(7))if args.require_binding else(0,1,2,3,4,6)
    with inputs.open('wb')as inp,expected.open('wb')as out:
        for copy in copies:
            for original,case,request in zip(rows,physical['cases'],requests,strict=True):
                if not case['completed']or not case['live_x87_unchanged']:raise ValueError('Original2A0230 failed completion/live80 contract: '+original['name'])
                gp,flags,mxcsr,cw,status,live,xmm,banks=request;inp.write(struct.pack('<I12I',copy,*gp,flags,mxcsr,cw,status));inp.write(struct.pack('<I',len(live)))
                for value in live:inp.write(bytes.fromhex(value))
                for lane in xmm:inp.write(struct.pack('<4I',*lane))
                inp.write(b''.join(banks));receipt=dict(name=original['name'],copy=copy,offset=out.tell());emit_abi(out,case['boundary_gp'],case['boundary_eflags'],case['after_fp'],case['boundary_xmm'])
                out.write(b''.join(bytes.fromhex(bank['after'])for bank in case['banks']));writes=case['writes']if copy<3 or copy==6 else [];out.write(struct.pack('<I',len(writes)))
                for event in writes:out.write(struct.pack('<III',event['instruction'],event['address'],len(bytes.fromhex(event['written'])))+bytes.fromhex(event['before']).ljust(4,b'\0')+bytes.fromhex(event['written']).ljust(4,b'\0'))
                phases=case['calls']if copy==2 else [];out.write(struct.pack('<I',len(phases)))
                for phase in phases:
                    out.write(struct.pack('<I',phase['entry']));emit_abi(out,phase['gp'],phase['eflags'],phase['fp'],phase['xmm']);out.write(struct.pack('<I',len(phase['live_x87'])));out.write(b''.join(bytes.fromhex(v)for v in phase['live_x87']))
                receipt['bytes']=out.tell()-receipt['offset'];receipts.append(receipt)
    with inputs.open('rb')as inp,actual.open('wb')as out:
        run=subprocess.run([str(binary)],stdin=inp,stdout=out,stderr=subprocess.PIPE,text=True,timeout=120)
    if run.returncode:raise ValueError('Native BSP2D failed: '+run.stderr[-4000:])
    if sha(actual)!=sha(expected):
        a,b=actual.read_bytes(),expected.read_bytes();first=next((i for i,(x,y)in enumerate(zip(a,b))if x!=y),min(len(a),len(b)));case=next((v for v in receipts if v['offset']<=first<v['offset']+v['bytes']),None)
        raise ValueError(f'BSP2D ABI mismatch byte{first},case{case}: {a[first:first+16].hex()} vs{b[first:first+16].hex()}')
    independent_controls=[]
    if args.require_binding:
        for control in ('HALO2_REFERENCE_RAW_ALLOCATOR','HALO2_REFERENCE_TICK_CONVERSION','HALO2_REFERENCE_PHYSICAL_EPOCHS'):
            target=scratch/'independent.actual'
            with inputs.open('rb')as inp,target.open('wb')as out:run=subprocess.run([str(binary)],stdin=inp,stdout=out,stderr=subprocess.PIPE,text=True,env=dict(os.environ,**{control:'1'}),timeout=120)
            if run.returncode or sha(target)!=sha(expected):raise ValueError('BSP2D unrelated reference control changed native owner: '+control+' '+run.stderr[-1000:])
            independent_controls.append(control);target.unlink()
    with inputs.open('rb')as inp:read_run=subprocess.run([str(binary)],stdin=inp,capture_output=True,env=dict(os.environ,HALO2_BSP_SURFACE_2D_READ_TEST='1'),check=True,timeout=120)
    cursor=0;read_sites={event['instruction']for case in physical['cases']for event in case['reads']};read_count=0
    for copy in copies:
        for case in physical['cases']:
            count=struct.unpack_from('<I',read_run.stdout,cursor)[0];cursor+=4;events=[struct.unpack_from('<III',read_run.stdout,cursor+12*i)for i in range(count)];cursor+=12*count
            explicit=[e for e in events if e[0]in read_sites];wanted=[(e['instruction'],e['address'],e['bytes'])for e in case['reads']]
            if explicit!=wanted:raise ValueError('BSP2D physical explicit read stream differs')
            read_count+=len(explicit)
    if cursor!=len(read_run.stdout):raise ValueError('BSP2D missing/trailing read records')
    executed=sorted({site for case in physical['cases']for site in case['executed_sites']})
    if len(executed)!=proof['owners']['0x2a0230']['sites']:raise ValueError('BSP2D original site coverage incomplete')
    files=(*COPIES,'include/halo2/native_tu15_bsp_surface_2d_kernel.hpp','src/physics/runtime/native_tu15_bsp_surface_2d.hpp','tests/native_tu15_bsp_surface_2d_abi_tests.cpp','tools/native_bsp_surface_2d_replacements.py','tools/validate_native_bsp_surface_2d_abi_tu15.py')
    if args.require_binding:files+=('src/physics/physics_live_support_tu15_generated.cpp','src/physics/runtime/native_tu15_allocator_owner_dispatch.hpp','tools/specialize_native_operands.py')
    report=dict(passed=True,production_bound=args.require_binding,comparisons=len(rows)*len(copies),physical_original=proof,compared_bytes=actual.stat().st_size,comparison_sha256=sha(actual),executed_sites=[f'{p:x}'for p in executed],binding=binding,independent_reference_controls=independent_controls,
        intermediate_full_abi_packets=sum(len(c['calls'])for c in physical['cases']),physical_explicit_read_comparisons=read_count,physical_implicit_reads_claimed=False,canonical_mutation_aliases=10,binding_drift_rejections=rejected,admission_refusals=9,
        forced_reference=json.loads(forced.stdout),address_undefined_sanitizers=args.sanitizers,compiler_command=command,original_body=dict(bytes=0x5e,sha256=body_pin),scratch_owner=str(scratch),source_sha256={name:sha(ROOT/name)for name in files},
        complete_memory_exception_abi_qualified=False,retained_large_artifacts=[])
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n');print(f'PASS {report["comparisons"]} nativeBSP2D full hardware comparisons');return 0
if __name__=='__main__':raise SystemExit(main())
