#!/usr/bin/env python3
"""Compare native22830 probe rotation/clock update against unchanged TU1.5."""
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
REGIONS=((0x449000,4096),(0x46a000,4096),(0x492000,4096),(0x4c5000,4096),
         (0x4ec000,4096),(0x515000,4096),(0x61000000,32768),(0x62000000,8192))


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--xbe',type=Path,required=True)
    parser.add_argument('--native',type=Path)
    parser.add_argument('--report',type=Path,required=True)
    parser.add_argument('--sanitizers',action='store_true')
    args=parser.parse_args()
    if sys.platform!='linux' or platform.machine().lower() not in ('x86_64','amd64','i386','i686'):
        print('SKIP physical PRT update requires Linux x86 ptrace');return 77
    if not args.xbe.is_file():print('SKIP missing owned original TU1.5');return 77
    if hashlib.sha256(args.xbe.read_bytes()).hexdigest()!=PIN:raise ValueError('Wrong TU1.5 original')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH'])
    sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
    from xbe import Xbe
    image=Xbe(args.xbe);native=args.native or scratch/'native-prt-update'
    if not args.native:
        command=['c++','-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math',
                 '-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),
                 str(ROOT/'tests/native_tu15_prt_update_tests.cpp'),
                 str(ROOT/'src/rendering/models/native_tu15_model_view_slots.cpp'),'-o',str(native)]
        if args.sanitizers:command+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
        subprocess.run(command,check=True)
    rng=random.Random(0x22830146100)
    cases=[(0,0,0,label) for label in ('null','empty','no-bsp','no-group','checksum','no-probes')]
    cases += [(count,0,0,'nonpositive-probes') for count in (-2147483648,-1)]
    for count in (1,2,4):
        for angle in (0,0.125,-0.75):
            for duration in (0,1,-1):cases.append((count,angle,duration,'finite'))
    for count in (1,2,4):
        for label in ('negative-phase','large-phase','clock-alias','probe-alias','nan-angle',
                      'infinite-angle','denormal-angle','nan-duration','large-angle','nan-axis','nonunit-axis'):
            cases.append((count,0.25,2,label))
    rows,receipts=[],[]
    request,expected,actual=(scratch/name for name in ('update.in','update.expected','update.actual'))
    with request.open('wb') as inputs:
        for ordinal,(count,angle,duration,label) in enumerate(cases):
            banks=[bytearray(rng.randbytes(size)) for _,size in REGIONS]
            for index in (0,1):banks[index][:]=image.read(REGIONS[index][0],REGIONS[index][1])
            def put(at,value,width=4):
                for (base,size),bank in zip(REGIONS,banks):
                    if base<=at<=base+size-width:
                        bank[at-base:at-base+width]=(value&((1<<(width*8))-1)).to_bytes(width,'little');return
                raise ValueError('Update fixture escaped declared banks')
            def fp(at,value):put(at,struct.unpack('<I',struct.pack('<f',value))[0])
            stack=0x62001000;gp=[rng.getrandbits(32) for _ in range(8)];gp[4]=stack
            flags=0x202|(rng.getrandbits(12)&0x8d5)
            control=0x1f80|((ordinal%4)<<13)|(0x8040 if ordinal&4 else 0)
            cw=(0x7f,0x27f,0x37f)[ordinal%3]
            xmm=[[rng.getrandbits(32) for _ in range(4)] for _ in range(8)]
            lightmap,bsp,group,probes=0x61000100,0x61000400,0x61000800,0x61001000
            if label=='clock-alias':probes=0x4920b4  # First red coefficients contain the real double clock.
            if label=='probe-alias':probes=0x4c5154  # First RGB coefficients alias the published SH cache.
            put(0x4ecc14,0 if label=='null' else lightmap)
            put(lightmap+0x80,0 if label=='empty' else 1)
            put(0x4ecc18,0 if label=='no-bsp' else bsp)
            put(lightmap+0x84,group);put(group+0x1c,-1 if label=='no-group' else 2)
            put(group+4,7);put(bsp+8,8 if label=='checksum' else 7)
            put(group+0x38,0 if label=='no-probes' else count);put(group+0x3c,probes)
            fp(lightmap+0x34,duration)
            if label=='nan-duration':put(lightmap+0x34,0x7f812345)
            fp(0x515ecc,2);fp(0x515ed0,0.35)
            if label=='negative-phase':fp(0x515ed0,-0.5)
            if label=='large-phase':fp(0x515ed0,17.75)
            for index in range(max(count,1)):
                row=probes+index*0xdc
                for offset in range(0,0xdc,4):fp(row+offset,rng.uniform(-2,2))
                for offset,value in ((0x94,0.6),(0x98,0.0),(0x9c,0.8),(0xa0,angle)):
                    fp(row+offset,value)
                if label=='nan-angle':put(row+0xa0,0x7f812345)
                if label=='infinite-angle':put(row+0xa0,0x7f800000)
                if label=='denormal-angle':put(row+0xa0,1)
                if label=='large-angle':fp(row+0xa0,1e30)
                if label=='nan-axis':put(row+0x94,0x7f812345)
                if label=='nonunit-axis':fp(row+0x94,2.25)
            # Genuine source time, never a supplied update outcome.
            clock=struct.unpack('<Q',struct.pack('<d',2.5))[0];put(0x4920c0,clock,8)
            put(stack,0x63000000);put(stack+4,0);put(stack+8,0)
            inputs.write(struct.pack('<I11I',0x22830,*gp,flags,control,cw))
            for lane in xmm:inputs.write(struct.pack('<4I',*lane))
            inputs.write(b''.join(banks))
            rows.append(dict(id=ordinal,name=f'{label}-{count}-{angle}-{duration}',entry=0x22830,gp=gp,
                eflags=flags,matrix=0,source=0,max_steps=100000,xmm=xmm,before_fp=[cw,0,control],live_x87=[],
                banks=[dict(address=base,before=bank.hex()) for (base,_),bank in zip(REGIONS,banks)]))
            receipts.append(dict(case=label,count=count,angle=angle,duration=duration))
    physical,proof=physical_original(image,rows,scratch,entries=(0x22830,0x146100,0x145d50,0x22730),regions=REGIONS)
    with expected.open('wb') as outputs:
        for receipt,original in zip(receipts,physical['cases'],strict=True):
            if not original['completed'] or not original['live_x87_unchanged']:raise ValueError('Original update did not complete')
            start=outputs.tell();outputs.write(struct.pack('<12I',*original['boundary_gp'],original['boundary_eflags'],
                original['after_fp'][2],original['after_fp'][1],original['after_fp'][0]))
            for lane in original['boundary_xmm']:outputs.write(struct.pack('<4I',*lane))
            outputs.write(b''.join(bytes.fromhex(bank['after']) for bank in original['banks']))
            outputs.write(struct.pack('<I',len(original['writes'])))
            for event in original['writes']:
                before,written=bytes.fromhex(event['before']),bytes.fromhex(event['written'])
                outputs.write(struct.pack('<III',event['instruction'],event['address'],len(written))+
                              before.ljust(4,b'\0')+written.ljust(4,b'\0'))
            receipt.update(stream_start=start,stream_bytes=outputs.tell()-start,stores=len(original['writes']))
    with request.open('rb') as inputs,actual.open('wb') as outputs:subprocess.run([str(native)],stdin=inputs,stdout=outputs,check=True)
    digest=lambda path:hashlib.sha256(path.read_bytes()).hexdigest()
    if digest(actual)!=digest(expected):
        left,right=actual.read_bytes(),expected.read_bytes()
        first=next((n for n,(a,b) in enumerate(zip(left,right)) if a!=b),min(len(left),len(right)))
        receipt=next((r for r in receipts if r['stream_start']<=first<r['stream_start']+r['stream_bytes']),{})
        raise ValueError(f'22830 mismatch{first} local{first-receipt.get("stream_start",0)} {receipt}: '
                         f'{left[first:first+16].hex()} vs{right[first:first+16].hex()}')
    from model_prt_update_native_replacements import replace_prt_update,PREFIX
    generated=(ROOT/'src/rendering/models/source_model_render_tu15_generated.cpp').read_text()
    if replace_prt_update(generated)!=generated:raise ValueError('22830 native default route absent')
    body=re.search(r'static void run_22830\(WorldState &s\) \{.*?\n\}',generated,re.S).group()
    rejected=0
    for candidate in (body,body.replace(PREFIX,'',1)):
        for old,new in (('0xa0u','0xa4u'),('run_146100(s)','run_145d50(s)'),('0x4920c0u','0x4920c4u')):
            changed=candidate.replace(old,new,1)
            if changed==candidate:raise ValueError('22830 mutation witness absent')
            try:replace_prt_update(generated.replace(body,changed,1))
            except ValueError:rejected+=1
            else:raise ValueError('22830 binder accepted changed complete body')
    # Replay the actual generator. Authenticate the complete original slot
    # reset before the prior narrower transform removes those operations.
    import generate_model_render_tu15 as generator
    original_binding=generator.replace_prt_update
    original_argv=sys.argv
    def inspect_original(source):
        nonlocal rejected
        raw=re.search(r'static void run_22830\(WorldState &s\) \{.*?\n\}',source,re.S).group()
        for old,new in (('0x4c51d4u','0x4c51d8u'),('0xffffffffu','0xfffffffeu')):
            changed=raw.replace(old,new,1)
            if changed==raw:raise ValueError('Retired22830 slot-prefix mutation witness absent')
            try:replace_prt_update(source.replace(raw,changed,1))
            except ValueError:rejected+=1
            else:raise ValueError('22830 binder retired a changed original slot operation')
        return original_binding(source)
    try:
        generator.replace_prt_update=inspect_original
        fresh=scratch/'regenerated-model.cpp'
        sys.argv=[str(ROOT/'tools/generate_model_render_tu15.py'),'--directory',str(scratch/'model-generation'),
                  '--output',str(fresh)]
        generator.main()
    finally:
        generator.replace_prt_update=original_binding;sys.argv=original_argv
    literal=re.sub(r'selected\((0x[0-9a-f]+u),s.image\)',r'\1',body)
    fresh_body=re.search(r'static void run_22830\(WorldState &s\) \{.*?\n\}',fresh.read_text(),re.S).group()
    if literal!=fresh_body:raise ValueError('22830 fresh generator body differs from admitted literal form')
    branch_sites=dict(rotations=0x22952,clock_advance=0x22ad4,phase_clamp=0x22b16,
                      cache_copy=0x22b33,cache_interpolation=0x22bb3,clock_disabled=0x22bfd)
    coverage={label:sum(site in original['executed_sites'] for original in physical['cases'])
              for label,site in branch_sites.items()}
    if not all(coverage.values()):raise ValueError('22830 cohort missed a material native branch')
    paths=('src/rendering/models/native_tu15_prt_update.hpp','tests/native_tu15_prt_update_tests.cpp',
           'tools/validate_native_tu15_prt_update.py','src/rendering/models/native_tu15_prt_rotation_bridge.hpp',
           'tools/model_prt_update_native_replacements.py','tools/model_prt_binding_common.py',
           'tools/generate_model_render_tu15.py','src/rendering/models/source_model_render_tu15_generated.cpp',
           'cmake/NativeTu15PrtSample.cmake')
    args.report.parent.mkdir(parents=True,exist_ok=True)
    args.report.write_text(json.dumps(dict(passed=True,cases=receipts,physical_original=proof,
        original_xbe_sha256=PIN,native_instruction_callbacks=0,forward_string_abi=True,
        branch_cases=coverage,rejected_semantic_mutations=rejected,fresh_generator_sha256=digest(fresh),
        read_fault_prefix_abi_qualified=False,compared_bytes=actual.stat().st_size,comparison_sha256=digest(actual),
        source_sha256={path:digest(ROOT/path) for path in paths}),indent=2,sort_keys=True)+'\n')
    print(f'PASS {len(cases)} physical22830 native update cases')
    return 0


if __name__=='__main__':raise SystemExit(main())
