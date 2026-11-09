#!/usr/bin/env python3
"""Physical-original qualification of model defaults and salted lookup leaves."""
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
from validate_native_tu15_prt_helpers import PIN, physical_original

ROOT=Path(__file__).resolve().parent.parent
REGIONS=((0x46a000,4096),(0x4ec000,4096),(0x61000000,32768),(0x62000000,8192))


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--xbe',type=Path,required=True)
    parser.add_argument('--native',type=Path)
    parser.add_argument('--report',type=Path,required=True)
    parser.add_argument('--require-binding',action='store_true')
    args=parser.parse_args()
    if sys.platform!='linux' or platform.machine().lower() not in ('x86_64','amd64','i386','i686'):
        print('SKIP model leaf original requires Linux x86 ptrace');return 77
    if not args.xbe.is_file():print('SKIP missing owned English TU1.5');return 77
    digest=lambda path:hashlib.sha256(path.read_bytes()).hexdigest()
    if digest(args.xbe)!=PIN:raise ValueError('Wrong original TU1.5 image')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH'])
    sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
    from xbe import Xbe
    image=Xbe(args.xbe);rng=random.Random(0x2c390bcdf0e7bc0)
    native=args.native or scratch/'native-model-leaves'
    if not args.native:
        subprocess.run(['c++','-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math',
            '-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),str(ROOT/'tests/native_tu15_model_lighting_leaves_tests.cpp'),
            '-o',str(native)],check=True)
    cases=[]
    for mode in ('ordinary','stack-before','stack-after','constant-alias','phi-alias','angle-alias',
                 'large-angle','nan-angle','infinite-phi','denormal-angle'):
        for precision in range(3):
            for rounding in range(4):cases.append((0x2c390,mode,precision,rounding))
    for index in (0,1,17,255):
        for salt in (0,0x1234,0xffff):cases.append((0xe7bc0,'flag',index,salt))
    for kind in ('none','empty','negative-count','out-of-range','zero-salt','bad-salt','matched','saved-row'):
        for salt in (1,0x7fff,0x8000,0xffff):
            for category in (0,1,31,32,255):cases.append((0xbcdf0,kind,salt,category))
    for category in (0,1,31,32,255):cases.append((0xbcdf0,'masked',0x8001,category))
    for entry in (0x1346c0,0x1349e0):
        for mode in ('color','zero','nan','infinite','denormal','scratch-alias','output-alias','factor-alias'):
            for precision in range(3):
                for rounding in range(4):cases.append((entry,mode,precision,rounding))
    for mode in ('tie','negative-tie'):
        for precision in range(3):
            for rounding in range(4):cases.append((0x1349e0,mode,precision,rounding))
    for mode in ('direction','z-high','z-negative','horizontal-zero','tiny','nan-vector','nan-angle',
                 'infinite-vector','large-angle','same-vector','forward-vector','backward-vector',
                 'argument-alias','saved-si-alias','bound-constant-alias','length-epsilon-alias',
                 'final-epsilon-alias','input-angle-alias','input-saved-alias'):
        for precision in range(3):
            for rounding in range(4):cases.append((0x11a360,mode,precision,rounding))
    for mode in ('step','zero-cap','negative-cap','nan-cap','infinite-cap','same-color','target-scratch',
                 'current-scratch','target-current','scale-nan','current-call-slot','target-call-slot'):
        for precision in range(3):
            for rounding in range(4):cases.append((0x3e4d0,mode,precision,rounding))
    inputs_path,expected,actual=(scratch/name for name in ('leaves.in','leaves.expected','leaves.actual'))
    rows=[];receipts=[]
    with inputs_path.open('wb') as inputs:
        for ordinal,(entry,mode,a,b) in enumerate(cases):
            banks=[bytearray(rng.randbytes(size)) for _,size in REGIONS];banks[0][:]=image.read(*REGIONS[0])
            def put(at,value,width=4):
                for (base,size),bank in zip(REGIONS,banks):
                    if base<=at<=base+size-width:
                        bank[at-base:at-base+width]=(value&((1<<(8*width))-1)).to_bytes(width,'little');return
                raise ValueError(f'Model leaf fixture escaped0x{at:x}')
            def double(at,value):
                raw=struct.unpack('<Q',struct.pack('<d',value))[0];put(at,raw,8)
            gp=[rng.getrandbits(32) for _ in range(8)];stack=0x62001000;gp[4]=stack
            flags=0x202|(rng.getrandbits(12)&0xcd5);mxcsr=0x1f80|((ordinal%4)<<13)|(0x8040 if ordinal&4 else 0)
            cw=(0x7f,0x27f,0x37f)[a%3]|((b%4)<<10)
            status=(ordinal&7)*0x100
            # C0/C1/C2 input bits exercise the specific x87 condition ownership.
            status = (status & 0x700)|((ordinal%8)<<11)|(0x4000 if ordinal&8 else 0)|((ordinal>>4)&63)
            live_values=('0000000000000080ff3f','00000000000000000080',
                         '35c26821a2da0fc90040','45230100000000c0ff7f',
                         '0000000000000080fe3f','00000000000000a00140')
            live=list(live_values[:(0,1,6 if entry==0x2c390 else 4)[ordinal%3]])
            xmm=[[rng.getrandbits(32) for _ in range(4)] for _ in range(8)]
            put(stack,0x63000000);put(stack+4,0x89abcdef)
            if entry==0x2c390:
                gp[0]={'stack-before':stack-0x78,'stack-after':stack+0x20,
                    'constant-alias':0x46a020,'phi-alias':0x46a0b4,'angle-alias':0x46a48c}.get(mode,0x61001000)
                if mode=='large-angle':double(0x46a4c8,2.0**64)
                if mode=='nan-angle':put(0x46a4c8,0x7ff0123456789abc,8)
                if mode=='infinite-phi':double(0x46a0f0,float('inf'))
                if mode=='denormal-angle':put(0x46a4c8,1,8)
            elif entry==0xe7bc0:
                gp[0]=(b<<16)|a;put(0x4ecbd0,0x61000100);put(0x61000144,0x61002000)
                put(0x61002000+a*12+8,0x61004000);put(0x610043dc,rng.getrandbits(8),1)
            elif entry==0xbcdf0:
                index=3;salt=a;gp[0]=0xffffffff if mode=='none' else (salt<<16)|index
                put(0x4ecbd0,0x61000100);put(0x61000138,{'empty':0,'negative-count':0xffffffff,'out-of-range':3}.get(mode,8))
                put(0x61000124,12);row=stack-8 if mode=='saved-row' else 0x61002000+index*12
                put(0x61000144,row-index*12)
                if mode=='saved-row':gp[7]=(b<<24)|salt
                else:
                    put(row,0 if mode=='zero-salt' else (salt^1 if mode=='bad-salt' else salt),2)
                    put(row+3,b,1);put(row+8,rng.getrandbits(32))
                put(stack+4,0 if mode=='masked' else 0xffffffff)
            elif entry==0x1346c0:
                gp[1]=0 if mode=='zero' else rng.getrandbits(32)
                gp[0]={'scratch-alias':stack-4,'output-alias':stack-8,'factor-alias':0x46a0c4}.get(mode,0x61001000)
                if mode=='nan':put(0x46a0c4,0x7f812345)
                if mode=='infinite':put(0x46a0c4,0x7f800000)
                if mode=='denormal':put(0x46a0c4,1)
                # Outputs overlapping the return word cannot return normally.
                if mode=='scratch-alias':gp[0]=stack-12
                if mode=='output-alias':gp[0]=stack-16
            elif entry==0x11a360:
                gp[1]={'input-angle-alias':stack+4,'input-saved-alias':stack-8}.get(mode,0x61002000)
                gp[0]={'same-vector':gp[1],'forward-vector':gp[1]+4,'backward-vector':gp[1]-4,
                       'argument-alias':stack+4,'saved-si-alias':stack-12,
                       'bound-constant-alias':0x46a0d0,'length-epsilon-alias':0x46a0e4,
                       'final-epsilon-alias':0x46a0e4}.get(mode,0x61001000)
                vector={'z-high':(0.3,0.4,0.9),'z-negative':(0.3,0.4,-0.9),
                        'horizontal-zero':(0,0,0.2),'tiny':(1e-30,-1e-30,0.2),
                        'length-epsilon-alias':(0.3,0.4,0.9),'final-epsilon-alias':(0.3,0.4,2),
                        'infinite-vector':(float('inf'),-float('inf'),float('inf'))}.get(mode,(0.3,0.4,0.5))
                for component,value in enumerate(vector):put(gp[1]+component*4,struct.unpack('<I',struct.pack('<f',value))[0])
                if mode=='nan-vector':put(gp[1],0x7f812345);put(gp[1]+4,0x7fcabcde)
                angle=2.0**70 if mode in ('large-angle','final-epsilon-alias') else 1.0
                put(stack+4,0x7f812345 if mode=='nan-angle' else struct.unpack('<I',struct.pack('<f',angle))[0])
            elif entry==0x3e4d0:
                gp[6]={'current-scratch':stack-0x18,'current-call-slot':stack-0x1c}.get(mode,0x61001000)
                target={'same-color':gp[6],'target-scratch':stack-0x18,'target-current':gp[6]+4,
                        'target-call-slot':stack-0x1c}.get(mode,0x61002000)
                put(gp[6],rng.getrandbits(32));put(target,rng.getrandbits(32));put(stack+4,target)
                cap={'zero-cap':0,'negative-cap':-0.25,'infinite-cap':float('inf')}.get(mode,0.1)
                xmm[2][0]=0x7f812345 if mode=='nan-cap' else struct.unpack('<I',struct.pack('<f',cap))[0]
                if mode=='scale-nan':put(0x46a0c4,0x7f812345)
            else:
                source={'scratch-alias':stack-8,'output-alias':stack-12,'factor-alias':0x46a148}.get(mode,0x61001000)
                put(stack+4,source)
                for component in range(3):
                    value=struct.unpack('<I',struct.pack('<f',rng.uniform(-2,2)))[0]
                    value={'zero':0,'nan':0x7f812345,'infinite':0xff800000,'denormal':1}.get(mode,value)
                    if mode in ('tie','negative-tie'):
                        put(0x46a148,0x43800000)
                        value=struct.unpack('<I',struct.pack('<f',(component*2+1)/512*(1 if mode=='tie' else -1)))[0]
                    put(source+component*4,value)
            put(stack,0x63000000)
            inputs.write(struct.pack('<I12I',entry,*gp,flags,mxcsr,cw,status))
            inputs.write(struct.pack('<I',len(live)))
            for value in live:inputs.write(bytes.fromhex(value))
            for lane in xmm:inputs.write(struct.pack('<4I',*lane))
            inputs.write(b''.join(banks))
            rows.append(dict(id=ordinal,name=f'{entry:x}-{mode}-{a}-{b}',entry=entry,gp=gp,eflags=flags,
                matrix=struct.unpack_from('<I',banks[3],stack+4-REGIONS[3][0])[0],
                source=struct.unpack_from('<I',banks[3],stack+8-REGIONS[3][0])[0],max_steps=1000,
                xmm=xmm,before_fp=[cw,status,mxcsr],live_x87=live,
                banks=[dict(address=base,before=bank.hex()) for (base,_),bank in zip(REGIONS,banks)]))
            receipts.append(dict(entry=entry,case=mode,first=a,second=b,live_x87_depth=len(live)))
    physical,proof=physical_original(image,rows,scratch,entries=(0x2c390,0xe7bc0,0xbcdf0,0x1346c0,0x1349e0,0x11a360,0x3e4d0),regions=REGIONS)
    with expected.open('wb') as outputs:
        for receipt,case in zip(receipts,physical['cases'],strict=True):
            if not case['completed'] or not case['live_x87_unchanged']:raise ValueError('Original model leaf did not complete')
            start=outputs.tell();outputs.write(struct.pack('<12I',*case['boundary_gp'],case['boundary_eflags'],
                case['after_fp'][2],case['after_fp'][1],case['after_fp'][0]))
            for lane in case['boundary_xmm']:outputs.write(struct.pack('<4I',*lane))
            outputs.write(b''.join(bytes.fromhex(bank['after']) for bank in case['banks']))
            outputs.write(struct.pack('<I',len(case['writes'])))
            for event in case['writes']:
                before,written=bytes.fromhex(event['before']),bytes.fromhex(event['written'])
                outputs.write(struct.pack('<III',event['instruction'],event['address'],len(written))+
                    before.ljust(4,b'\0')+written.ljust(4,b'\0'))
            receipt.update(stream_start=start,stream_bytes=outputs.tell()-start,stores=len(case['writes']))
    with inputs_path.open('rb') as inputs,actual.open('wb') as outputs:subprocess.run([str(native)],stdin=inputs,stdout=outputs,check=True)
    if digest(actual)!=digest(expected):
        left,right=actual.read_bytes(),expected.read_bytes();first=next((i for i,(a,b) in enumerate(zip(left,right)) if a!=b),min(len(left),len(right)))
        receipt=next((r for r in receipts if r['stream_start']<=first<r['stream_start']+r['stream_bytes']),{})
        raise ValueError(f'Model leaf mismatch {first} local{first-receipt.get("stream_start",0)} {receipt}: '
            f'{left[first:first+16].hex()} vs{right[first:first+16].hex()}')
    rejected=0
    if args.require_binding:
        import model_lighting_leaves_native_replacements as lighting
        import model_color_native_replacements as color
        import model_direction_native_replacements as direction
        import model_color_step_native_replacements as color_step
        generated=(ROOT/'src/rendering/models/source_model_render_tu15_generated.cpp').read_text()
        for binding,bind in ((lighting,lighting.replace_model_lighting_leaves),(color,color.replace_model_color),
                             (direction,direction.replace_model_direction),(color_step,color_step.replace_model_color_step)):
            if bind(generated)!=generated:raise ValueError('Model leaf native-default route absent')
            for entry in binding.PINS:
                body=re.search(rf'static void run_{entry:x}\(WorldState &s\) \{{.*?\n\}}',generated,re.S).group()
                literal=re.sub(r'selected\((0x[0-9a-f]+u),s.image\)',r'\1',body)
                for shape in (body,literal):
                    rewritten=generated.replace(body,shape,1)
                    if bind(rewritten)!=rewritten:raise ValueError('Model leaf shape not idempotent')
                    for candidate in (shape,shape.replace(binding.prefix(entry),'',1)):
                        changed=candidate.replace(',4)',',2)',1)
                        if changed==candidate:raise ValueError('Model leaf mutation witness missing')
                        try:bind(generated.replace(body,changed,1))
                        except ValueError:rejected+=1
                        else:raise ValueError('Model leaf binder accepted semantic body drift')
        import model_default_lighting_native_replacements as defaults
        for path in ('src/rendering/models/source_model_render_tu15_generated.cpp',
                     'src/physics/physics_live_support_tu15_generated.cpp',
                     'src/runtime/source_map_world_support_tu15_generated.cpp',
                     'src/gameplay/units/source_combat_action_tu15_generated.cpp',
                     'src/rendering/effects/source_effect_particle_draw_tu15_generated.cpp'):
            source=(ROOT/path).read_text();bind=defaults.replace_model_default_lighting
            if bind(source)!=source:raise ValueError('Default lighting native binding absent')
            body=re.search(r'static void run_2c390\(WorldState &s\) \{.*?\n\}',source,re.S)[0]
            for shape in (body,re.sub(r'selected\((0x[0-9a-f]+u),s.image\)',r'\1',body)):
                rewritten=source.replace(body,shape,1)
                if bind(rewritten)!=rewritten:raise ValueError('Default lighting body shape not idempotent')
                for candidate in (shape,shape.replace(defaults.PREFIX,'',1)):
                    changed=candidate.replace(',4)',',2)',1)
                    if changed==candidate:raise ValueError('Missing default-lighting mutation witness')
                    try:bind(source.replace(body,changed,1))
                    except ValueError:rejected+=1
                    else:raise ValueError('Default lighting binder accepted body drift')
    paths=('src/rendering/models/native_tu15_model_lighting_leaves.hpp','tests/native_tu15_model_lighting_leaves_tests.cpp',
           'src/rendering/models/native_tu15_model_color.hpp',
           'src/rendering/models/native_tu15_model_direction.hpp',
           'src/rendering/models/native_tu15_model_color_step.hpp',
           'tools/validate_native_tu15_model_lighting_leaves.py','tools/validate_native_tu15_prt_helpers.py',
           'tools/model_lighting_leaves_native_replacements.py','tools/model_prt_binding_common.py',
           'tools/model_color_native_replacements.py',
           'tools/model_direction_native_replacements.py',
           'tools/model_color_step_native_replacements.py',
           'tools/model_default_lighting_native_replacements.py','tools/native_selected_operand_binding.py',
           'tools/generate_physics_live_support_tu15.py','tools/generate_map_world_support_tu15.py',
           'tools/generate_combat_action_tu15.py','tools/generate_effect_particle_draw_tu15.py',
           'src/physics/physics_live_support_tu15_generated.cpp','src/runtime/source_map_world_support_tu15_generated.cpp',
           'src/gameplay/units/source_combat_action_tu15_generated.cpp',
           'src/rendering/effects/source_effect_particle_draw_tu15_generated.cpp',
           'tools/generate_model_render_tu15.py','cmake/NativeTu15ModelViewSlots.cmake',
           'src/rendering/models/native_tu15_prt_basis.hpp','src/rendering/models/native_tu15_prt_projection.hpp',
           'src/rendering/models/native_tu15_prt_helpers.hpp','src/physics/runtime/physics_step_internal.hpp',
           'src/physics/runtime/physics_solver_internal.hpp','src/rendering/models/source_model_render_tu15_generated.cpp',
           'src/physics/runtime/physics_contact_runtime_internal.hpp')
    args.report.parent.mkdir(parents=True,exist_ok=True)
    direction_cases=[case for case in physical['cases'] if 0x11a360 in case['executed_sites']]
    sites=dict(z_clamp=0x11a3ad,first_normalization=0x11a413,zero_length=0x11a427,
               fallback=0x11a43f,second_normalization=0x11a47d)
    direction_coverage={label:sum(site in case['executed_sites'] for case in direction_cases) for label,site in sites.items()}
    direction_coverage['second_normalization_skipped']=sum(0x11a47d not in case['executed_sites'] for case in direction_cases)
    if not all(direction_coverage.values()):raise ValueError(f'Direction branch not reached: {direction_coverage}')
    step_cases=[case for case in physical['cases'] if 0x3e4d0 in case['executed_sites']]
    step_sites=dict(red_lower_cap=0x3e507,red_upper_cap=0x3e511,green_lower_cap=0x3e532,
                    green_upper_cap=0x3e53c,blue_uncapped=0x3e566)
    step_coverage={label:sum(site in case['executed_sites'] for case in step_cases) for label,site in step_sites.items()}
    if not all(step_coverage.values()):raise ValueError(f'Color-step branch not reached: {step_coverage}')
    args.report.write_text(json.dumps(dict(passed=True,cases=receipts,original_xbe_sha256=PIN,physical_original=proof,
        compared_bytes=actual.stat().st_size,comparison_sha256=digest(actual),native_instruction_callbacks=0,
        live_x87_inputs_verified=True,live_x87_depths=[0,1,4,6],x87_top_values=list(range(8)),
        direction_branch_cases=direction_coverage,
        color_step_branch_cases=step_coverage,x87_c3_and_sticky_exception_inputs_verified=True,
        rejected_semantic_mutations=rejected,memory_fault_prefix_abi_verified=False,ordered_read_fault_semantics_verified=False,
        scratch_owner=str(scratch),
        source_sha256={path:digest(ROOT/path) for path in paths}),indent=2,sort_keys=True)+'\n')
    print(f'PASS {len(cases)} complete native model lighting/lookup leaf cases')
    return 0


if __name__=='__main__':raise SystemExit(main())
