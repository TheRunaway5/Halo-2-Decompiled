#!/usr/bin/env python3
"""Compare all native16EA60 copies against unchanged TU1.5 hardware execution."""
import argparse
import hashlib
import json
import os
import platform
from pathlib import Path
import random
import re
import shlex
import struct
import subprocess
import sys
from generated_routine_ownership import function_bodies
from native_effect_slot_scan_replacements import (canonical_body,install_native_effect_slot_scan,PREFIX)
from validate_native_tu15_prt_helpers import PIN,physical_original

ROOT=Path(__file__).resolve().parents[1]
REGIONS=((0x61000000,16384),(0x62000000,8192))
COPIES=('src/campaign/ai/source_ai_frame_tu15_generated.cpp',
        'src/gameplay/units/source_combat_action_tu15_generated.cpp',
        'src/rendering/effects/source_effect_light_view_tu15_generated.cpp',
        'src/rendering/effects/source_effect_particle_frame_tu15_generated.cpp')


def digest(path):return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--xbe',type=Path,required=True)
    p.add_argument('--report',type=Path,required=True);p.add_argument('--sanitizers',action='store_true')
    p.add_argument('--compile-commands',type=Path)
    p.add_argument('--require-binding',action='store_true');args=p.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):p.error('Run through tools/run_test.py')
    if sys.platform!='linux' or platform.machine().lower() not in ('x86_64','amd64','i386','i686'):
        print('SKIP original effect-slot hardware proof requires Linux x86 ptrace');return 77
    if not args.xbe.is_file():print('SKIP missing original TU1.5 image');return 77
    if digest(args.xbe)!=PIN:raise ValueError('Wrong original TU1.5 image')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);bound=[];rejected=0
    for name,path in zip(('frame','combat','light','particle'),COPIES):
        source=(ROOT/path).read_text();body=next(body for match,body in function_bodies(source)if match[1]=='16ea60')
        original=canonical_body(body);raw=source.replace(body,original,1);native=install_native_effect_slot_scan(raw)
        if args.require_binding and native!=source:raise ValueError('Native effect slot scan production prefix missing: '+path)
        if install_native_effect_slot_scan(native)!=native:raise ValueError('Effect slot scan binding is not idempotent')
        selected=next((line for line in source.splitlines()if line.startswith('std::uint32_t selected(')),'')
        mini=install_native_effect_slot_scan('#include \"physics/runtime/physics_world_runtime_internal.hpp\"\n'+selected+'\n'+original)
        bound.append('namespace '+name+' {\n'+mini+'\n}\n')
        if name=='frame':bound.append('namespace reference {\n'+selected+'\n'+original+'\n}\n')
        literal=re.sub(r'selected\((0x[0-9a-f]+u),s\.image\)',r'\1',original)
        for shape in (original,literal):
            candidate=raw.replace(original,shape,1);install_native_effect_slot_scan(candidate)
            for victim in (candidate,install_native_effect_slot_scan(candidate)):
                target=re.search(r'static void run_16ea60\(WorldState &s\) \{.*?\n\}',victim,re.S)[0]
                changed_body=target.replace('s.gp[1]),4)', 's.gp[1]),2)',1)
                if changed_body==target:raise ValueError('Effect slot scan mutation witness missing')
                changed=victim.replace(target,changed_body,1)
                try:install_native_effect_slot_scan(changed)
                except ValueError:rejected+=1
                else:raise ValueError('Effect slot scan accepted changed complete body')
        if 'selected('in original:
            changed=raw.replace('{if(image==SourcePhysicsContactImage::MapPack11)',
                '{if(p==0)return 1u;if(image==SourcePhysicsContactImage::MapPack11)',1)
            try:install_native_effect_slot_scan(changed)
            except ValueError:rejected+=1
            else:raise ValueError('Effect slot scan accepted altered selected resolver')
    (scratch/'effect_slot_scan_bodies.inc').write_text('\n'.join(bound))
    import native_effect_slot_scan_replacements as binding
    import native_effect_birth_color_binding as birth
    world_path=ROOT/'src/physics/physics_live_world_tu15.cpp';world=world_path.read_text()
    if birth.install_birth_color_collector(world)!=world:raise ValueError('Production birth-color typed collector/borrow missing')
    for before,after in (('birth_colors{event.presentation,o}','birth_colors{event.presentation,{}}'),('birth_colors.borrow()','birth_colors.other_borrow()')):
        changed=world.replace(before,after,1)
        if changed==world:raise ValueError('Missing birth producer mutation witness')
        try:birth.install_birth_color_collector(changed)
        except ValueError:rejected+=1
        else:raise ValueError('Accepted changed birth collector producer/lease')
    (scratch/'birth_color_original_collector.inc').write_text(birth.ORIGINAL)
    byte_source=(ROOT/'src/assets/cache.cpp').read_text();byte_methods=[]
    for name,pin in (('void Bytes::require','473fb33334f7f36929e033076c3411bb4a52fde5a7877f70ff077b4da7c293f1'),
                     ('std::uint32_t Bytes::u32','a7145e2e5a660146f5f6460db7a99e81625f9d7bb4a706a3c3a0b3f2856e19d8')):
        begin=byte_source.index(name);opening=byte_source.index('{',begin);end=opening+1;depth=1
        while depth:
            if byte_source[end]=='{':depth+=1
            elif byte_source[end]=='}':depth-=1
            end+=1
        method=byte_source[begin:end]
        if hashlib.sha256(method.encode()).hexdigest()!=pin:raise ValueError('Actual birth collector byte-reader changed')
        byte_methods.append(method)
    (scratch/'birth_color_bytes_reference.inc').write_text('namespace halo2 {\n'+'\n'.join(byte_methods)+'\n}\n')
    owner_path=ROOT/'src/gameplay/weapons/native_weapon_effect_frame_tu15_internal.hpp'
    dispatch_path=ROOT/'src/gameplay/weapons/native_weapon_effect_dispatch_tu15.cpp'
    owner=owner_path.read_text();dispatch=dispatch_path.read_text();native_owner=binding.install_owner(owner,dispatch)
    if args.require_binding and(native_owner!=owner or binding.OWNER_INCLUDE not in dispatch):raise ValueError('Native Effect slot owner prefix/include absent')
    if binding.install_owner(native_owner,dispatch)!=native_owner:raise ValueError('Effect slot owner binding not idempotent')
    for shape in (binding.canonical_body(binding.body(owner,'find_effect_slot'),binding.OWNER_PREFIX),binding.body(native_owner,'find_effect_slot')):
        changed=shape.replace('a.gp[1]+0x38','a.gp[1]+0x34',1)
        if changed==shape:raise ValueError('Missing Effect helper mutation witness')
        try:binding.install_owner(owner.replace(binding.body(owner,'find_effect_slot'),changed,1),dispatch)
        except ValueError:rejected+=1
        else:raise ValueError('Accepted semantic Effect helper drift')
    for name in binding.ROUTE_PINS:
        route=binding.body(dispatch,name);changed=route.replace('0x16ea60u','0x16ea61u',1)
        if changed==route:raise ValueError('Missing actual Effect entry route witness')
        try:binding.install_owner(owner,dispatch.replace(route,changed,1))
        except ValueError:rejected+=1
        else:raise ValueError('Accepted changed Effect entry route')
    (scratch/owner_path.name).write_text(native_owner)
    if binding.OWNER_INCLUDE not in dispatch:dispatch=binding.OWNER_INCLUDE+dispatch
    (scratch/'effect.cpp').write_text(dispatch)
    binary=scratch/'effect-slot-scan'
    command=['c++','-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math',
        '-ffunction-sections','-fdata-sections','-Wl,--gc-sections',
        '-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(ROOT/'src/gameplay/weapons'),'-I'+str(scratch),
        str(ROOT/'tests/native_tu15_effect_slot_scan_tests.cpp'),str(scratch/'effect.cpp'),'-o',str(binary)]
    if args.sanitizers:command[1:1]=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
    compiled=subprocess.run(command,capture_output=True,text=True)
    if compiled.returncode:raise ValueError('Effect slot test compilation failed: '+compiled.stderr[-4000:])
    refusal=subprocess.run([str(binary)],check=True,capture_output=True,text=True,
        env=dict(os.environ,HALO2_REFERENCE_EFFECT_SLOT_SCAN='1',HALO2_EFFECT_SLOT_REFUSAL_TEST='1'))
    reference_result=json.loads(refusal.stdout)
    if not reference_result['reference_admission_refused'] or reference_result['literal_callbacks']<=0 or reference_result['last_literal_pc']!=0x16ea98 or reference_result['returned_eax']!=0xffffffff:
        raise ValueError('Forced Effect slot reference did not provide actual refusal/callback/return evidence')
    rng=random.Random(0x16ea60);rows=[];requests=[];receipts=[]
    firsts=(0x80000000,0xffffffff,0,1,31,32,33,95,0x7ffffffe,0x7fffffff)
    modes=('match-first','match-last','exhaust','count-zero','count-negative','count-min','count-max',
        'unaligned','count-saved-di','pointer-saved-si','pointer-saved-bx','pointer-saved-di',
        'bitmap-saved-bx','bitmap-saved-si','bitmap-saved-di','bitmap-return','bitmap-descriptor')
    for first in firsts:
        for mode in modes:
            ordinal=len(rows);banks=[bytearray(rng.randbytes(size))for _,size in REGIONS]
            def put(at,value):
                for (base,size),bank in zip(REGIONS,banks):
                    if base<=at<=base+size-4:struct.pack_into('<I',bank,at-base,value&0xffffffff);return
                raise ValueError(f'Effect slot fixture escaped0x{at:x}')
            def get(at):
                for (base,size),bank in zip(REGIONS,banks):
                    if base<=at<=base+size-4:return struct.unpack_from('<I',bank,at-base)[0]
                raise ValueError(f'Effect slot fixture escaped0x{at:x}')
            stack=0x62001000+ordinal%4;pool=0x61000100;word_at=0x61002000;limit=min(0x7fffffff,first+4) if first<0x80000000 else 4
            gp=[rng.getrandbits(32)for _ in range(8)];gp[4]=stack;gp[2]=first
            if mode=='count-zero':limit=0
            if mode=='count-negative':limit=0xffffffff
            if mode=='count-min':limit=0x80000000
            if mode=='count-max':limit=0x7fffffff
            if mode=='unaligned':pool+=1;word_at+=3
            if mode=='count-saved-di':pool=stack-4-0x38;gp[7]=limit
            if mode=='pointer-saved-si':pool=stack-8-0x48
            if mode=='pointer-saved-bx':pool=stack-12-0x48
            if mode=='pointer-saved-di':pool=stack-4-0x48
            if mode.startswith('bitmap-saved-'):
                offset={'bitmap-saved-bx':12,'bitmap-saved-si':8,'bitmap-saved-di':4}[mode];word_at=stack-offset
                register={'bitmap-saved-bx':3,'bitmap-saved-si':6,'bitmap-saved-di':7}[mode];gp[register]=1<<(first&31)
            if mode=='bitmap-return':word_at=stack
            if mode=='bitmap-descriptor':word_at=pool+0x38
            bitmap=(word_at-(first>>5)*4)&0xffffffff
            put(pool+0x38,limit);put(pool+0x48,bitmap)
            if mode=='pointer-saved-si':gp[6]=bitmap
            if mode=='pointer-saved-di':gp[7]=bitmap
            for n in range(2):
                if mode.startswith('bitmap-saved-')or mode in ('bitmap-return','bitmap-descriptor'):break
                put(word_at+n*4,0)
            if first<0x80000000 and first<limit and mode not in ('exhaust','bitmap-return','bitmap-descriptor')and not mode.startswith('bitmap-saved-'):
                found=first+(min(3,limit-first-1)if mode=='match-last'else 0)
                at=(bitmap+(found>>5)*4)&0xffffffff;put(at,get(at)|(1<<(found&31)))
            gp[1]=pool;put(stack,0x63000000)
            flags=0x202|(rng.getrandbits(12)&0xcd5);mxcsr=0x1f80|((ordinal%4)<<13)|(0x8040 if ordinal&4 else 0)
            cw=(0x7f,0x27f,0x37f)[ordinal%3]|((ordinal%4)<<10)
            status=((ordinal%8)<<11)|((ordinal%8)<<8)|(0x4000 if ordinal&8 else 0)|((ordinal>>4)&63)
            live_values=('0000000000000080ff3f','00000000000000000080','35c26821a2da0fc90040','45230100000000c0ff7f',
                '00000000000000c00040','00000000000000a00140','0000000000000080fe3f','0000000000000080fd3f')
            live=list(live_values[:(0,1,4,6,8)[ordinal%5]]);xmm=[[rng.getrandbits(32)for _ in range(4)]for _ in range(8)]
            rows.append(dict(id=ordinal,name=f'{first:x}-{mode}',entry=0x16ea60,gp=gp,eflags=flags,matrix=get(stack+4),source=get(stack+8),max_steps=10000,
                xmm=xmm,before_fp=[cw,status,mxcsr],live_x87=live,banks=[dict(address=base,before=bank.hex())for (base,_),bank in zip(REGIONS,banks)]))
            requests.append((gp,flags,mxcsr,cw,status,live,xmm,banks));receipts.append(dict(case=rows[-1]['name'],first=first,mode=mode,live_depth=len(live)))
    sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'));from xbe import Xbe
    physical,proof=physical_original(Xbe(args.xbe),rows,scratch,entries=(0x16ea60,),regions=REGIONS)
    executed=set().union(*(case['executed_sites']for case in physical['cases']))
    if len(executed)!=30:raise ValueError('Effect slot original30-site coverage incomplete')
    inp,expected,actual=(scratch/name for name in ('record.in','record.expected','record.actual'))
    with inp.open('wb')as inputs,expected.open('wb')as outputs:
        for copy in range(7):
            for case,request in zip(physical['cases'],requests,strict=True):
                if not case['completed']or not case['live_x87_unchanged']:raise ValueError('Original effect slot scan completion/live-FP contract changed')
                gp,flags,mxcsr,cw,status,live,xmm,banks=request
                inputs.write(struct.pack('<I12I',copy,*gp,flags,mxcsr,cw,status));inputs.write(struct.pack('<I',len(live)))
                for value in live:inputs.write(bytes.fromhex(value))
                for lane in xmm:inputs.write(struct.pack('<4I',*lane))
                inputs.write(b''.join(banks))
                outputs.write(struct.pack('<12I',*case['boundary_gp'],case['boundary_eflags'],case['after_fp'][2],case['after_fp'][1],case['after_fp'][0]))
                for lane in case['boundary_xmm']:outputs.write(struct.pack('<4I',*lane))
                outputs.write(b''.join(bytes.fromhex(bank['after'])for bank in case['banks']))
                outputs.write(struct.pack('<I',len(case['writes'])))
                for event in case['writes']:
                    outputs.write(struct.pack('<III',event['instruction'],event['address'],len(bytes.fromhex(event['written']))))
                    outputs.write(bytes.fromhex(event['before']).ljust(4,b'\0'));outputs.write(bytes.fromhex(event['written']).ljust(4,b'\0'))
    with inp.open('rb')as inputs,actual.open('wb')as outputs:subprocess.run([str(binary)],stdin=inputs,stdout=outputs,check=True)
    if digest(actual)!=digest(expected):
        a,b=actual.read_bytes(),expected.read_bytes();first=next((n for n,(x,y)in enumerate(zip(a,b))if x!=y),min(len(a),len(b)))
        raise ValueError(f'Effect slot scan mismatch byte{first}: {a[first:first+16].hex()} vs {b[first:first+16].hex()}')
    producer_syntax=dict(run=False)
    if args.compile_commands and args.compile_commands.is_file():
        commands=json.loads(args.compile_commands.read_text())
        item=next(row for row in commands if Path(row['file']).resolve()==world_path.resolve())
        original=item.get('arguments')or shlex.split(item['command']);command=[];skip=False
        for value in original:
            if skip:skip=False;continue
            if value in ('-o','-MF','-MT','-MQ'):skip=True;continue
            if value in ('-c','-MD','-MMD','-MP'):continue
            command.append(value)
        command.append('-fsyntax-only');compiled=subprocess.run(command,cwd=item['directory'],capture_output=True,text=True)
        if compiled.returncode:raise ValueError('Actual birth producer CMake-command syntax failed: '+compiled.stderr[-4000:])
        producer_syntax=dict(run=True,passed=True,command=command,source_sha256=digest(world_path))
    paths=(*COPIES,'src/gameplay/weapons/native_tu15_effect_slot_scan.hpp','tools/native_effect_slot_scan_replacements.py',
        'include/halo2/native_tu15_bitmap_scan_kernel.hpp','include/halo2/native_tu15_data_array_iteration.hpp','src/core/native_tu15_data_array_iteration.cpp',
        'src/gameplay/weapons/native_weapon_effect_frame_tu15_internal.hpp','src/gameplay/weapons/native_weapon_effect_dispatch_tu15.cpp',
        'tests/native_tu15_effect_slot_scan_tests.cpp','tools/validate_native_effect_slot_scan_tu15.py','tools/native_selected_operand_binding.py',
        'tools/native_weapon_effect_state_bridge_tu15.py','cmake/NativeTu15EffectSlotScan.cmake',
        'include/halo2/native_weapon_effect_birth_color_observer.hpp','tools/native_effect_birth_color_binding.py',
        'src/physics/physics_live_world_tu15.cpp','src/assets/cache.cpp',
        'tools/validate_native_tu15_prt_helpers.py')
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(dict(passed=True,cases=receipts,scratch_owner=str(scratch),
        production_copies=4,actual_api_routes=2,birth_collector_route=1,comparisons=len(rows)*7,original_xbe_sha256=PIN,physical_original=proof,
        compared_bytes=actual.stat().st_size,comparison_sha256=digest(actual),native_instruction_callbacks=0,
        rejected_mutations=rejected,reference_admission_refused=reference_result['reference_admission_refused'],
        forced_reference_result=reference_result,source_sha256={path:digest(ROOT/path)for path in paths},
        supplemental_canonical_read_store_alias_cases=12,admission_refusal_cases=6,
        canonical_ordered_read_store_cases=len(rows)*7,canonical_final_instruction_cursor_verified=True,
        original_sites_executed=len(executed),
        default_observer_empty_service_cases=len(rows)*7,birth_color_collector_cases=90,birth_color_native_cases=len(rows),
        birth_collector_lease_identity_executor_lifetime_verified=True,
        live_outer_rebinding_revokes_direct_and_nested_admission=True,
        rebound_outer_retains_instruction_callbacks_and_backing_mutation=True,
        actual_producer_translation_syntax=producer_syntax,
        opaque_active_mmx_transport_verified=True,physical_mmx_execution_verified=False,
        normal_return_abi_verified=True,ordered_stores_verified=True,memory_fault_prefix_abi_verified=False,
        ordered_read_fault_semantics_verified=False,visual_parity_verified=False,native_production_closure_verified=False,
        retained_large_artifacts=[]),indent=2,sort_keys=True)+'\n')
    print(f'PASS {len(rows)*7} native effect-slot-scan comparisons against{len(rows)} unchanged i386 cases')
    return 0


if __name__=='__main__':raise SystemExit(main())
