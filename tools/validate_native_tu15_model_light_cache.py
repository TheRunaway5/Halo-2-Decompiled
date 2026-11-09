#!/usr/bin/env python3
"""Compare structured3DDC0 and its real owner boundaries with physical TU1.5."""
import argparse
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
from validate_native_tu15_prt_helpers import PIN,physical_original

ROOT=Path(__file__).resolve().parent.parent
REGIONS=((0x44a000,4096),(0x46a000,4096),(0x4c6000,4096),(0x4ec000,4096),
         (0x4f0000,4096),(0x515000,4096),(0x61000000,65536),(0x62000000,16384))


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--xbe',type=Path,required=True);p.add_argument('--native',type=Path)
    p.add_argument('--report',type=Path,required=True);p.add_argument('--sanitizers',action='store_true')
    p.add_argument('--require-binding',action='store_true');args=p.parse_args()
    if sys.platform!='linux' or platform.machine().lower() not in ('x86_64','amd64','i386','i686'):
        print('SKIP physical original light-cache requires Linux x86 ptrace');return 77
    if not args.xbe.is_file():print('SKIP missing original TU1.5 image');return 77
    digest=lambda path:hashlib.sha256(path.read_bytes()).hexdigest()
    if digest(args.xbe)!=PIN:raise ValueError('Wrong original TU1.5 image')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
    from xbe import Xbe
    image=Xbe(args.xbe);rng=random.Random(0x3ddc016ea60)
    native=args.native or scratch/'native-light-cache'
    if not args.native:
        command=['c++','-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math',
            '-ffunction-sections','-fdata-sections','-Wl,--gc-sections','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),
            str(ROOT/'tests/native_tu15_model_light_cache_tests.cpp'),
            str(ROOT/'src/gameplay/weapons/native_weapon_effect_dispatch_tu15.cpp'),'-o',str(native)]
        if args.sanitizers:command[1:1]=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
        subprocess.run(command,check=True)
    cases=[(mode,size,backward) for mode in ('cached','stale','new','full','empty-full','future','tie','rollover')
           for size in (0,1,2,4,7) for backward in (False,True)]
    cases += [(mode,size,backward) for mode in ('unaligned','source-overlap','negative-offset','positive-offset',
                 'clock-alias','constant-alias','count-overwritten') for size in (0,4,7) for backward in (False,True)]
    rows=[];receipts=[];request,expected,actual=(scratch/name for name in ('cache.in','cache.expected','cache.actual'))
    with request.open('wb') as inputs:
        for ordinal,(mode,size,backward) in enumerate(cases):
            banks=[bytearray(rng.randbytes(length)) for _,length in REGIONS]
            for i,(base,length) in enumerate(REGIONS[:2]):banks[i][:]=image.read(base,length)
            def put(at,value,width=4):
                for (base,length),bank in zip(REGIONS,banks):
                    if base<=at<=base+length-width:
                        bank[at-base:at-base+width]=(value&((1<<(width*8))-1)).to_bytes(width,'little');return
                raise ValueError(f'Light-cache fixture escaped0x{at:x}')
            gp=[rng.getrandbits(32) for _ in range(8)];stack=0x62002000;gp[4]=stack
            object_pool,object_rows,object=0x61000100,0x61001000,0x61002000
            pool,bitmap,records=0x61000400,0x61000300,0x61008000
            if mode=='negative-offset':object=0x61009000
            if mode=='unaligned':records+=1
            if mode=='clock-alias':records=0x4c6800
            if mode=='constant-alias':records=0x46a050
            if mode=='count-overwritten':object=records-0xa4
            handle=0x12340003;salt=(1,0x7fff,0x8000,0xffff)[ordinal%4]
            put(0x4ecbd0,object_pool);put(object_pool+0x44,object_rows);put(object_rows+3*12+8,object)
            put(0x515f14,pool);put(pool+0x20,3);put(pool+0x24,256);put(pool+0x3c,3);put(pool+0x40,salt,2)
            put(pool+0x44,records);put(pool+0x48,bitmap)
            full=mode not in ('cached','stale','new')
            put(pool+0x34,3 if full else 0);put(pool+0x38,3 if mode!='new' else 0)
            put(bitmap,0 if mode=='empty-full' else (7 if full else (6 if mode=='stale' else 7)))
            existing=(salt<<16)|1 if mode in ('cached','stale') else 0xffffffff
            put(object+0xcc,existing);put(0x4c6854,100)
            ages={'future':(-1,-2,-3),'tie':(9,9,3),'rollover':(-0x80000000,0x7fffffff,-1),
                  'constant-alias':(30,5,1),'clock-alias':(30,5,1)}.get(mode,(5,13,2))
            for index,age in enumerate(ages):
                record=records+index*256;put(record,salt,2)
                put(record+4,handle if mode=='cached' and index==1 else 0x98760000+index)
                put(record+12,100-age)
            offset=-32768 if mode=='negative-offset' else (32767 if mode=='positive-offset' else 0x300)
            source=object+offset
            if mode=='source-overlap':source=records+256+0xc4;offset=source-object
            put(object+0x118,size*10,2);put(object+0x11a,offset,2)
            for component in range(8):put(source+component*4,0x12340003+component)
            put(object+0x124,96,2);put(object+0x126,0x500,2)
            for channel in range(8):
                for component,value in enumerate((0x3e800000,0x3f000000,0x3f400000)):
                    put(object+0x500+channel*12+component*4,value)
            put(stack,0x63000000);put(stack+4,handle);put(stack+8,rng.getrandbits(32))
            flags=0x202|(rng.getrandbits(12)&0xcd5)|(1024 if backward else 0)
            mxcsr=0x1f80|((ordinal%4)<<13)|(0x8040 if ordinal&4 else 0)
            cw=(0x7f,0x27f,0x37f)[ordinal%3]|((ordinal%4)<<10)
            status=((ordinal%8)<<11)|((ordinal%8)<<8)|(0x4000 if ordinal&8 else 0)|((ordinal>>4)&63)
            live=list(('0000000000000080ff3f','00000000000000000080',
                '35c26821a2da0fc90040','45230100000000c0ff7f')[:(0,1,4)[ordinal%3]])
            xmm=[[rng.getrandbits(32) for _ in range(4)] for _ in range(8)]
            inputs.write(struct.pack('<I12I',0x3ddc0,*gp,flags,mxcsr,cw,status));inputs.write(struct.pack('<I',len(live)))
            for value in live:inputs.write(bytes.fromhex(value))
            for lane in xmm:inputs.write(struct.pack('<4I',*lane))
            inputs.write(b''.join(banks))
            rows.append(dict(id=ordinal,name=f'{mode}-{size}-{backward}',entry=0x3ddc0,gp=gp,eflags=flags,
                matrix=handle,source=struct.unpack_from('<I',banks[-1],stack+8-REGIONS[-1][0])[0],max_steps=10000,
                xmm=xmm,before_fp=[cw,status,mxcsr],live_x87=live,
                banks=[dict(address=base,before=bank.hex()) for (base,_),bank in zip(REGIONS,banks)]))
            receipts.append(dict(mode=mode,copy_bytes=size,backward=backward))
    entries=(0x3ddc0,0x16e6a0,0x16ea60,0x2c390,0x3dd00,0xbcd80,0x1349e0)
    physical,proof=physical_original(image,rows,scratch,entries=entries,regions=REGIONS,
        trace_calls=[dict(entry=entry,arguments=0,gp=True,xmm=True) for entry in (0x16e6a0,0x16ea60,0x2c390,0x3dd00)])
    with expected.open('wb') as outputs:
        for receipt,case in zip(receipts,physical['cases'],strict=True):
            if not case['completed'] or not case['live_x87_unchanged']:raise ValueError('Original light-cache did not complete')
            start=outputs.tell();outputs.write(struct.pack('<12I',*case['boundary_gp'],case['boundary_eflags'],
                case['after_fp'][2],case['after_fp'][1],case['after_fp'][0]))
            for lane in case['boundary_xmm']:outputs.write(struct.pack('<4I',*lane))
            outputs.write(b''.join(bytes.fromhex(bank['after']) for bank in case['banks']))
            outputs.write(struct.pack('<I',len(case['writes'])))
            for event in case['writes']:
                before,written=bytes.fromhex(event['before']),bytes.fromhex(event['written'])
                outputs.write(struct.pack('<III',event['instruction'],event['address'],len(written))+before.ljust(4,b'\0')+written.ljust(4,b'\0'))
            outputs.write(struct.pack('<I',len(case['calls'])))
            for call in case['calls']:
                outputs.write(struct.pack('<I12I',call['entry'],*call['gp'],call['eflags'],call['fp'][2],call['fp'][1],call['fp'][0]))
                for lane in call['xmm']:outputs.write(struct.pack('<4I',*lane))
            receipt.update(stream_start=start,stream_bytes=outputs.tell()-start,calls=len(case['calls']))
    with request.open('rb') as inputs,actual.open('wb') as outputs:
        result=subprocess.run([str(native)],stdin=inputs,stdout=outputs,stderr=subprocess.PIPE)
    if result.returncode:raise ValueError('Native light-cache failed: '+result.stderr.decode()[-3000:])
    residuals=[int(value) for value in re.findall(rb'^residual (\d+)$',result.stderr,re.M)]
    slot_work=[tuple(map(int,row)) for row in re.findall(rb'^slot (\d+) (\d+)$',result.stderr,re.M)]
    datum_work=[tuple(map(int,row)) for row in re.findall(rb'^datum (\d+) (\d+)$',result.stderr,re.M)]
    if len(slot_work)!=len(cases)or not sum(row[0] for row in slot_work)or any(row[1] for row in slot_work):
        raise ValueError('Light-cache16EA60 child did not use the qualified native bitmap owner')
    if len(residuals)!=len(cases):raise ValueError('Missing explicit residual-owner accounting')
    if any(residuals)or len(datum_work)!=len(cases)or not sum(row[0]for row in datum_work)or any(row[1]for row in datum_work):
        raise ValueError('Light-cache16E6A0 child retained instruction work instead of the qualified native DATA owner')
    for receipt,count,slot,datum in zip(receipts,residuals,slot_work,datum_work,strict=True):
        receipt['residual_owner_instruction_callbacks']=count;receipt['native_slot_calls']=slot[0]
        receipt['native_datum_calls']=datum[0]
    if digest(actual)!=digest(expected):
        left,right=actual.read_bytes(),expected.read_bytes();first=next((i for i,(a,b) in enumerate(zip(left,right)) if a!=b),min(len(left),len(right)))
        receipt=next((r for r in receipts if r['stream_start']<=first<r['stream_start']+r['stream_bytes']),{})
        raise ValueError(f'Light-cache mismatch{first} local{first-receipt.get("stream_start",0)} {receipt}: '
            f'{left[first:first+24].hex()} vs{right[first:first+24].hex()}')
    rejected=0
    if args.require_binding:
        import model_light_cache_native_replacements as binding
        for path in binding.COPIES:
            source=(ROOT/path).read_text();bind=binding.replace_model_light_cache
            if bind(source)!=source:raise ValueError('Structured light-cache native binding absent')
            body=re.search(r'static void run_3ddc0\(WorldState &s\) \{.*?\n\}',source,re.S)[0]
            for shape in (body,re.sub(r'selected\((0x[0-9a-f]+u),s.image\)',r'\1',body)):
                rewritten=source.replace(body,shape,1)
                if bind(rewritten)!=rewritten:raise ValueError('Light-cache body shape not idempotent')
                for candidate in (shape,shape.replace(binding.PREFIX,'',1)):
                    changed=candidate.replace(',4)',',2)',1)
                    if changed==candidate:raise ValueError('Missing light-cache mutation witness')
                    try:bind(source.replace(body,changed,1))
                    except ValueError:rejected+=1
                    else:raise ValueError('Light-cache binder accepted body drift')
            changed=body.replace('s.fp.count<=4u','s.fp.count<=5u',1)
            try:bind(source.replace(body,changed,1))
            except ValueError:rejected+=1
            else:raise ValueError('Light-cache binder accepted depth-guard drift')
            if binding.VALIDATION in body:
                if not body[body.index('{')+1:].startswith(binding.VALIDATION+binding.PREFIX):
                    raise ValueError('Native light cache bypassed Model presented-pose validation')
                changed=body.replace('if(source_has_presented_model_pose_tu15','if(false&&source_has_presented_model_pose_tu15',1)
                try:bind(source.replace(body,changed,1))
                except ValueError:rejected+=1
                else:raise ValueError('Light-cache binder accepted presented-pose validation drift')
            if 'selected(' in body:
                resolver=re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}',source,re.S)[0]
                changed=resolver.replace('default:throw','default:if(p==0u)return 0u;throw',1)
                if changed==resolver:raise ValueError('Missing light-cache resolver mutation witness')
                try:bind(source.replace(resolver,changed,1))
                except ValueError:rejected+=1
                else:raise ValueError('Light-cache binder accepted resolver-code drift')
    paths=('src/rendering/models/native_tu15_model_light_cache.hpp','tests/native_tu15_model_light_cache_tests.cpp',
        'tools/validate_native_tu15_model_light_cache.py','tools/validate_native_tu15_prt_helpers.py',
        'src/gameplay/weapons/native_weapon_effect_dispatch_tu15.cpp','src/gameplay/weapons/native_weapon_effect_frame_tu15_internal.hpp',
        'src/gameplay/weapons/native_tu15_effect_slot_scan.hpp','include/halo2/native_tu15_bitmap_scan_kernel.hpp',
        'src/gameplay/weapons/native_tu15_effect_datum_allocate.hpp','include/halo2/native_tu15_datum_allocate_kernel.hpp',
        'tools/native_effect_datum_allocate_replacements.py','tools/native_weapon_effect_datum_bridge_tu15.py',
        'src/rendering/models/native_tu15_model_lighting_leaves.hpp','src/rendering/models/native_tu15_model_color_publish.hpp',
        'src/rendering/models/native_tu15_model_light_source.hpp','src/rendering/models/native_tu15_model_color.hpp',
        'src/physics/runtime/physics_contact_runtime_internal.hpp','tools/model_light_cache_native_replacements.py',
        'tools/native_selected_operand_binding.py','cmake/NativeTu15ModelViewSlots.cmake',
        'tools/generate_physics_live_support_tu15.py','tools/generate_model_render_tu15.py',
        'tools/generate_map_world_support_tu15.py','tools/generate_combat_action_tu15.py','tools/generate_shadow_frame_tu15.py',
        'src/rendering/models/source_model_render_tu15_generated.cpp','src/physics/physics_live_support_tu15_generated.cpp',
        'src/runtime/source_map_world_support_tu15_generated.cpp','src/gameplay/units/source_combat_action_tu15_generated.cpp',
        'src/rendering/shadows/source_shadow_frame_tu15_generated.cpp')
    sites=dict(cached=0x3de0f,allocation=0x3de1a,eviction=0x3de70,selected_older=0x3de9a,
        failed=0x3dfba,negative_age_fallback=0x3de8f,defaults=0x3df1a,publish=0x3dfa7)
    coverage={name:sum(site in case['executed_sites'] for case in physical['cases']) for name,site in sites.items()}
    for name,site in (('word_copy',0x3df7b),('byte_copy',0x3df82)):
        coverage[name]=sum(any(event['instruction']==site for event in case['writes']) for case in physical['cases'])
    if not all(coverage.values()):raise ValueError(f'Light-cache path not covered: {coverage}')
    args.report.parent.mkdir(parents=True,exist_ok=True)
    args.report.write_text(json.dumps(dict(passed=True,cases=receipts,physical_original=proof,original_xbe_sha256=PIN,
        compared_bytes=actual.stat().st_size,comparison_sha256=digest(actual),parent_instruction_callbacks=0,
        exact_child_entry_abi_snapshots=sum(case['calls'] for case in receipts),residual_owner_instruction_callbacks=sum(residuals),
        residual_owners=[],fully_native_subtree=True,
        native_slot_calls=sum(row[0] for row in slot_work),slot_instruction_callbacks=sum(row[1] for row in slot_work),
        native_datum_calls=sum(row[0]for row in datum_work),datum_instruction_callbacks=sum(row[1]for row in datum_work),
        rejected_semantic_mutations=rejected,branch_cases=coverage,
        legacy_state_only_copies_open=['src/gameplay/units/death_startup_runtime_generated.cpp:3DDC0'],
        memory_fault_prefix_abi_verified=False,ordered_read_fault_semantics_verified=False,scratch_owner=str(scratch),
        source_sha256={path:digest(ROOT/path) for path in paths}),indent=2,sort_keys=True)+'\n')
    print(f'PASS {len(cases)} structured native light-cache parent cases with native slot and datum owners')
    return 0


if __name__=='__main__':raise SystemExit(main())
