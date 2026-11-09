#!/usr/bin/env python3
"""Qualify the complete23670 PRT cache controller against physical TU1.5."""
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
from validate_native_tu15_prt_cache import CHILD_PIN

ROOT = Path(__file__).resolve().parent.parent
REGIONS = ((0x449000,4096),(0x46a000,4096),(0x475000,4096),(0x494000,0x28000),
           (0x4c5000,4096),(0x4ec000,4096),(0x4f0000,4096),(0x4f2000,8192),
           (0x515000,4096),(0x61000000,65536),(0x62000000,16384))


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--xbe',type=Path,required=True)
    parser.add_argument('--report',type=Path,required=True)
    parser.add_argument('--sanitizers',action='store_true')
    parser.add_argument('--require-binding',action='store_true')
    args=parser.parse_args()
    if sys.platform!='linux' or platform.machine().lower() not in ('x86_64','amd64','i386','i686'):
        print('SKIP physical PRT cache builder requires Linux x86 ptrace');return 77
    if not args.xbe.is_file():print('SKIP missing owned English TU1.5');return 77
    digest=lambda path:hashlib.sha256(path.read_bytes()).hexdigest()
    if digest(args.xbe)!=PIN:raise ValueError('Wrong original TU1.5')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH'])
    sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
    from xbe import Xbe
    image=Xbe(args.xbe)
    source=(ROOT/'src/physics/physics_live_support_tu15_generated.cpp').read_text()
    child=re.search(r'static void run_130980\(WorldState &s\) \{.*?\n\}',source,re.S).group()
    from native_geometry_request_replacements import canonical_body
    child=canonical_body(child)
    if hashlib.sha256(child.encode()).hexdigest()!=CHILD_PIN:raise ValueError('130980 reference drifted')
    stubs='\n'.join(f'static void run_{entry}(WorldState&) {{throw std::runtime_error("Unqualified resource child{entry}");}}'
                    for entry in ('130eb0','1236a0','2dbf74','128870','22a680'))
    reference=scratch/'cache-child-reference.hpp'
    reference.write_text('namespace cache_reference {\nusing namespace halo2::physics_world_runtime_detail;\n'
        'static std::uint32_t sign_extend(std::uint32_t x,unsigned w){return w==1?'
        'std::uint32_t(std::int32_t(std::bit_cast<std::int8_t>(std::uint8_t(x)))):'
        'std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(x))));}\n'+stubs+'\n'+child+'\n}\n')
    native=scratch/'native-prt-cache-build'
    command=['c++','-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math',
        '-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-DHALO2_PRT_CACHE_CHILD_REFERENCE="'+str(reference)+'"',
        str(ROOT/'tests/native_tu15_prt_cache_build_tests.cpp'),'-o',str(native)]
    if args.sanitizers:command+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
    subprocess.run(command,check=True)
    rng=random.Random(0x23670146100)
    cases=[('existing',count,1,3,1,1,0) for count in (1,2,24)]
    cases += [('full',24,1,3,1,1,0)]
    cases += [('empty',0,count,order,1,1,0) for count in (-1,0,1,3) for order in (1,3)]
    cases += [('capacity',0,1,3,1,1,1)]
    cases += [('negative-cache',-1,count,3,1,1,raw) for count,raw in ((0,0),(1,1))]
    for mode in ('default','live-none','resident','missing'):
        for count in (1,3):
            for coefficient_count in (1,2,7):
                for components in (0,1,2):
                    cases.append((mode,0,count,3,2,components,coefficient_count))
    for label in ('singular','nan-matrix','nan-coefficients','source-payload-alias','selector-five'):
        for coefficient_count in (1,2):cases.append((label,0,2,3,2,2,coefficient_count))
    for label in ('source-basis-alias','source-inverse-alias','descriptor-inverse-alias',
                  'cursor-sign','cursor-high','cursor-limit','resource-stride-alias',
                  'context-count','context-peer','context-none','context-mismatch'):
        for coefficient_count in (1,2):cases.append((label,0,2,3,2,1,coefficient_count))
    for mode in ('default','resident'):
        for order in (1,2,4):
            for raw in (1,2):cases.append((mode,0,1,order,2,1,raw))
    cases += [('live-none',0,count,3,2,2,0) for count in (0,1,3)]
    rows=[];receipts=[]
    inputs_path,expected,actual=(scratch/name for name in ('build.in','build.expected','build.actual'))
    with inputs_path.open('wb') as inputs:
        for ordinal,(mode,cache_count,count,order,channels,components,coefficient_count) in enumerate(cases):
            banks=[bytearray(rng.randbytes(size)) for _,size in REGIONS]
            for index in (0,1,2):banks[index][:]=image.read(*REGIONS[index])
            def put(at,word,width=4):
                for (base,size),bank in zip(REGIONS,banks):
                    if base<=at<=base+size-width:
                        bank[at-base:at-base+width]=(word&((1<<(width*8))-1)).to_bytes(width,'little');return
                raise ValueError(f'23670 fixture store escaped0x{at:x}')
            def fp(at,x):put(at,struct.unpack('<I',struct.pack('<f',x))[0])
            stack=0x62002000;descriptor=0x61001000;sections=0x61002000
            frame=stack-0x210
            if mode=='descriptor-inverse-alias':descriptor=frame+0x6c
            lookup=0x61003000;coefficients=0x61004000
            selector_table=0x61000600;object_id=0xabcd0003;tag=0x12340002
            gp=[rng.getrandbits(32) for _ in range(8)];gp[4]=stack
            flags=0x202|(rng.getrandbits(12)&0x8d5)|(0x400 if ordinal&1 else 0)
            control=0x1f80|((ordinal%4)<<13)|(0x8040 if ordinal&4 else 0)
            cw=(0x7f,0x27f,0x37f)[ordinal%3]
            xmm=[[rng.getrandbits(32) for _ in range(4)] for _ in range(8)]
            put(0x515ec8,cache_count)
            for slot in range(24):put(0x4c51d0+slot*104,object_id if mode=='existing' and slot==cache_count-1 else object_id+slot+1)
            cursor={'capacity':0x27000,'cursor-sign':0x80000000,
                    'cursor-high':0xf0000000,'cursor-limit':0x80027000}.get(mode,0)
            put(0x494338,cursor)
            live=mode in ('live-none','resident','missing','resource-stride-alias')
            put(0x4ecc14,0x61000100 if live or mode.startswith('context-') else 0)
            put(0x4ecc18,0 if mode=='context-peer' else 0x61000300)
            put(0x61000180,1);put(0x61000184,0x61000200)
            put(0x6100021c,1);put(0x61000204,7);put(0x61000308,7)
            put(0x61000238,0);put(0x515ed4,0,1)
            if mode=='context-count':put(0x61000180,0xffffffff)
            if mode=='context-none':put(0x6100021c,0xffffffff)
            if mode=='context-mismatch':put(0x61000308,8)
            if mode=='live-none':tag=0xffffffff
            put(descriptor,order,2);put(descriptor+2,channels,2);put(descriptor+4,components,2)
            put(descriptor+0x14,6);put(descriptor+0x18,lookup)
            if mode=='source-payload-alias':coefficients=0x49433c
            if mode=='source-basis-alias':coefficients=frame+0x110
            if mode=='source-inverse-alias':coefficients=frame+0x6c
            put(descriptor+0x20,coefficients)
            for variant in range(6):
                put(lookup+variant*12,0 if mode in ('source-basis-alias','source-inverse-alias') else variant*16)
            for word in range(256):fp(coefficients+word*4,rng.uniform(-1,1))
            for section in range(max(count,1)):
                put(sections+4+section*4,section,2)
                put(sections+6+section*4,coefficient_count,2)
                for secondary in range(2):
                    transform=sections+0x44+section*48+secondary*48
                    for component in range(12):
                        if mode=='descriptor-inverse-alias':
                            fp(transform+component*4,1.0 if component in (1,4,10) else 0)
                        else:fp(transform+component*4,1.0 if component in (0,5,10) else rng.uniform(-0.1,0.1))
                    if mode=='singular':
                        for component in range(12):fp(transform+component*4,0)
                    if mode=='nan-matrix':put(transform+4,0x7f812345)
            if mode=='nan-coefficients':put(coefficients+5*16*4,0x7fc12345)
            # Real235E0/130980 resident or missing resource path.
            put(0x4f0414,0x61007000);put(0x61007028,0x61007100);put(0x61007128,0x61007200)
            put(selector_table,0,1);put(selector_table+1,0,1);put(selector_table+2,0,1)
            section_row=0x61007200;header=section_row+0x38
            put(section_row+0x34,0x61007300);put(0x61007368,0x61007400)
            put(0x61007400,0,1);put(0x61007401,1,1)
            put(header+0x1e,0 if mode=='missing' else 1,1);put(header+8,0);put(header+0x20,2)
            put(0x4f2d6c,0x61007500);put(0x61007564,0x61007600)
            put(0x61007644,0x61007700);put(0x61007538,0x89abcdef)
            if mode=='resource-stride-alias':
                put(0x61007644,frame+0x5c-0x14-2*24);put(0x61007538,0)
            put(0x4ecbd0,0x61007800);put(0x61007844,0x61007900)
            put(0x6100792c,0x61007a00);put(0x61007b16,0,2)
            for index,word in enumerate((0x63000000,descriptor,5 if mode=='selector-five' else 0,
                                         object_id,sections,count,selector_table,tag)):put(stack+index*4,word)
            inputs.write(struct.pack('<I11I',0x23670,*gp,flags,control,cw))
            for lane in xmm:inputs.write(struct.pack('<4I',*lane))
            inputs.write(b''.join(banks))
            rows.append(dict(id=ordinal,name=f'{mode}-{count}-{order}-{components}-{coefficient_count}',entry=0x23670,
                gp=gp,eflags=flags,matrix=descriptor,source=5 if mode=='selector-five' else 0,
                max_steps=100000,xmm=xmm,before_fp=[cw,0,control],live_x87=[],
                banks=[dict(address=base,before=bank.hex()) for (base,_),bank in zip(REGIONS,banks)]))
            receipts.append(dict(mode=mode,sections=count,order=order,components=components,coefficients=coefficient_count))
    entries=(0x23670,0x23520,0x23580,0x235e0,0x23c80,0x1456f0,0x145d50,0x146100,
             0x145e60,0x148b20,0x22cd0,0x22c10,0x22730,0x130980)
    physical,proof=physical_original(image,rows,scratch,entries=entries,regions=REGIONS,
        trace_calls=[dict(entry=0x130980,arguments=0,gp=True,xmm=True)])
    with expected.open('wb') as outputs:
        for receipt,original in zip(receipts,physical['cases'],strict=True):
            if not original['completed'] or not original['live_x87_unchanged']:raise ValueError('23670 original incomplete')
            start=outputs.tell()
            outputs.write(struct.pack('<12I',*original['boundary_gp'],original['boundary_eflags'],
                original['after_fp'][2],original['after_fp'][1],original['after_fp'][0]))
            for lane in original['boundary_xmm']:outputs.write(struct.pack('<4I',*lane))
            outputs.write(b''.join(bytes.fromhex(bank['after']) for bank in original['banks']))
            outputs.write(struct.pack('<I',len(original['writes'])))
            for event in original['writes']:
                before,written=bytes.fromhex(event['before']),bytes.fromhex(event['written'])
                outputs.write(struct.pack('<III',event['instruction'],event['address'],len(written))+
                    before.ljust(4,b'\0')+written.ljust(4,b'\0'))
            outputs.write(struct.pack('<I',len(original['calls'])))
            for call in original['calls']:
                outputs.write(struct.pack('<I12I',call['entry'],*call['gp'],call['eflags'],
                    call['fp'][2],call['fp'][1],call['fp'][0]))
                for lane in call['xmm']:outputs.write(struct.pack('<4I',*lane))
            receipt.update(stream_start=start,stream_bytes=outputs.tell()-start,stores=len(original['writes']),
                           resource_owner_calls=len(original['calls']))
    with inputs_path.open('rb') as inputs,actual.open('wb') as outputs:
        subprocess.run([str(native)],stdin=inputs,stdout=outputs,check=True)
    if digest(actual)!=digest(expected):
        left,right=actual.read_bytes(),expected.read_bytes()
        first=next((i for i,(a,b) in enumerate(zip(left,right)) if a!=b),min(len(left),len(right)))
        receipt=next((r for r in receipts if r['stream_start']<=first<r['stream_start']+r['stream_bytes']),{})
        raise ValueError(f'23670 mismatch{first} local{first-receipt.get("stream_start",0)} {receipt}: '
            f'{left[first:first+16].hex()} vs {right[first:first+16].hex()}')
    rejected=0
    if args.require_binding:
        from model_prt_cache_build_native_replacements import replace_prt_cache_build,PREFIX
        generated=(ROOT/'src/rendering/models/source_model_render_tu15_generated.cpp').read_text()
        if replace_prt_cache_build(generated)!=generated:raise ValueError('23670 native default binding absent')
        body=re.search(r'static void run_23670\(WorldState &s\) \{.*?\n\}',generated,re.S).group()
        literal=re.sub(r'selected\((0x[0-9a-f]+u),s.image\)',r'\1',body)
        for shape in (body,literal):
            rewritten=generated.replace(body,shape,1)
            if replace_prt_cache_build(rewritten)!=rewritten:raise ValueError('23670 source shape changed binding')
            for candidate in (shape,shape.replace(PREFIX,'',1)):
                for old,new in (('0x515ec8u','0x515eccu'),('0x27000u','0x27004u'),('if(s.flags.zf||','if(!s.flags.zf||')):
                    changed=candidate.replace(old,new,1)
                    if changed==candidate:raise ValueError('23670 mutation witness missing')
                    try:replace_prt_cache_build(generated.replace(body,changed,1))
                    except ValueError:rejected+=1
                    else:raise ValueError('23670 binder accepted semantic source drift')
    branch_sites=dict(existing=0x236c0,full=0x236c9,default_basis=0x2377b,live=0x23725,
        zero_allocation=0x237d7,allocation_failure=0x23c68,primary=0x239e9,secondary=0x23aac,
        primary_resource=0x23946,secondary_resource=0x23b73,sample=0x22cd0,resource_owner=0x130980)
    coverage={label:sum(site in case['executed_sites'] for case in physical['cases'])
              for label,site in branch_sites.items()}
    if not all(coverage.values()):raise ValueError(f'23670 required branch was not reached: {coverage}')
    paths=('src/rendering/models/native_tu15_prt_cache_build.hpp','tests/native_tu15_prt_cache_build_tests.cpp',
           'tools/validate_native_tu15_prt_cache_build.py','tools/validate_native_tu15_prt_helpers.py',
           'tools/model_prt_cache_build_native_replacements.py','tools/model_prt_binding_common.py',
           'tools/native_geometry_request_replacements.py',
           'tools/generate_model_render_tu15.py','cmake/NativeTu15ModelViewSlots.cmake',
           'src/rendering/models/source_model_render_tu15_generated.cpp',
           'src/rendering/models/native_tu15_prt_helpers.hpp','src/rendering/models/native_tu15_prt_projection.hpp',
           'src/rendering/models/native_tu15_prt_cache.hpp','src/rendering/models/native_tu15_prt_sample.hpp',
           'src/rendering/models/native_tu15_prt_interpolation.hpp','src/rendering/models/native_tu15_prt_scaled_basis.hpp',
           'src/rendering/models/native_tu15_prt_basis.hpp','src/rendering/models/native_tu15_prt_matrix_inverse.hpp',
           'src/rendering/models/native_tu15_prt_rotation_bridge.hpp','src/rendering/models/native_tu15_prt_rotation_types.hpp',
           'src/rendering/models/native_tu15_prt_rotation_polynomial.hpp','src/rendering/models/native_tu15_prt_matrix_vector.hpp',
           'src/physics/runtime/physics_world_runtime_internal.hpp','src/physics/runtime/physics_contact_runtime_internal.hpp')
    args.report.parent.mkdir(parents=True,exist_ok=True)
    args.report.write_text(json.dumps(dict(passed=True,cases=receipts,physical_original=proof,
        original_xbe_sha256=PIN,resource_child_body_sha256=CHILD_PIN,
        compared_bytes=actual.stat().st_size,comparison_sha256=digest(actual),
        native_instruction_callbacks=0,rejected_semantic_mutations=rejected,branch_cases=coverage,
        resource_owner_entry_abi_verified=True,
        resource_owner_entry_calls=sum(receipt['resource_owner_calls'] for receipt in receipts),
        resource_request_children_qualified=False,memory_fault_prefix_abi_verified=False,
        ordered_read_fault_semantics_verified=False,validation_scope='complete normal-return state and ordered stores',
        source_sha256={path:digest(ROOT/path) for path in paths}),indent=2,sort_keys=True)+'\n')
    print(f'PASS {len(cases)} full23670 physical-original cache build cases')
    return 0


if __name__=='__main__':raise SystemExit(main())
