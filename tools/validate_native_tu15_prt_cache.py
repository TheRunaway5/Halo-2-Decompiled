#!/usr/bin/env python3
"""Compare native235E0 with genuine130980 resident/missing child ownership."""
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
from native_geometry_request_replacements import canonical_body

ROOT = Path(__file__).resolve().parent.parent
REGIONS = ((0x475000,4096), (0x4ec000,4096), (0x4f0000,4096),
           (0x4f2000,8192), (0x61000000,32768), (0x62000000,8192))
CHILD_PIN = 'e0c21836231c2846c43ad71067a7841d92ad4533275f90a9377409d73008ba43'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--xbe', type=Path, required=True)
    parser.add_argument('--report', type=Path, required=True)
    parser.add_argument('--sanitizers', action='store_true')
    args = parser.parse_args()
    if sys.platform != 'linux' or platform.machine().lower() not in ('x86_64','amd64','i386','i686'):
        print('SKIP physical PRT cache requires Linux x86 ptrace'); return 77
    if not args.xbe.is_file(): print('SKIP missing English TU1.5 image'); return 77
    if hashlib.sha256(args.xbe.read_bytes()).hexdigest() != PIN: raise ValueError('Wrong original TU1.5')
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    sys.path.insert(0, str(ROOT / 'tools/reference/halo2-decompiled/tools'))
    from xbe import Xbe
    image = Xbe(args.xbe)
    source = (ROOT / 'src/physics/physics_live_support_tu15_generated.cpp').read_text()
    child = re.search(r'static void run_130980\(WorldState &s\) \{.*?\n\}',source,re.S).group()
    child = canonical_body(child)
    if hashlib.sha256(child.encode()).hexdigest() != CHILD_PIN:
        raise ValueError('Actual130980 child reference source drifted')
    stubs = '\n'.join(f'static void run_{entry}(WorldState&) {{ throw std::runtime_error("Unqualified genuine130980 child{entry} reached"); }}'
                      for entry in ('130eb0','1236a0','2dbf74','128870','22a680'))
    reference = scratch / 'cache-child-reference.hpp'
    reference.write_text('namespace cache_reference {\nusing namespace halo2::physics_world_runtime_detail;\n'
                         'static std::uint32_t sign_extend(std::uint32_t value,unsigned width) {'
                         'return width==1?std::uint32_t(std::int32_t(std::bit_cast<std::int8_t>(std::uint8_t(value)))):'
                         'std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(value))));}\n'
                         + stubs + '\n' + child + '\n}\n')
    native = scratch / 'native-prt-cache'
    command = ['c++','-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math',
               '-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),
               '-DHALO2_PRT_CACHE_CHILD_REFERENCE="'+str(reference)+'"',
               str(ROOT/'tests/native_tu15_prt_cache_tests.cpp'),'-o',str(native)]
    if args.sanitizers: command += ['-fsanitize=address,undefined','-fno-omit-frame-pointer']
    subprocess.run(command,check=True)
    rng = random.Random(0x235e0130980)
    cases = [('none',0,0)]
    for kind in ('resident','missing'):
        for selector in (0,1,7,255):
            for bias in (-32768,-1,0,1,32767): cases.append((kind,selector,bias))
    for kind in ('saved-si','saved-bx','saved-di','lod-slot'):
        for bias in (-1,0,32767): cases.append((kind,2,bias))
    rows, receipts = [], []
    inputs_path, expected, actual = (scratch/name for name in ('cache.in','cache.expected','cache.actual'))
    with inputs_path.open('wb') as inputs:
        for ordinal,(kind,selector,bias) in enumerate(cases):
            banks = [bytearray(rng.randbytes(size)) for _,size in REGIONS]
            def put(at,value,width=4):
                for (base,size),bank in zip(REGIONS,banks):
                    if base<=at<=base+size-width:
                        bank[at-base:at-base+width]=(value&((1<<(width*8))-1)).to_bytes(width,'little');return
                raise ValueError('Cache fixture escaped input banks')
            stack=0x62001000; gp=[rng.getrandbits(32) for _ in range(8)]
            gp[0]=0xffffffff if kind=='none' else 0x12340002
            gp[1]=0xabcd0003;gp[4]=stack
            flags=0x202|(rng.getrandbits(12)&0xcd5)
            xmm=[[rng.getrandbits(32) for _ in range(4)] for _ in range(8)]
            put(0x4f0414,0x61001000);put(0x61001028,0x61002000)
            put(0x61002028,0x61003000)
            selector_table=0x61000400; selector_index=3
            put(selector_table+selector_index,selector,1)
            # The byte selector spans256 section rows in a separate declared
            # region: fixture uses a low row while testing every raw selector.
            array=0x61003000-selector*0x5c
            put(0x61002028,array)
            section=0x61003000; header=section+0x38
            put(section+0x34,0x61004800);put(0x61004868,0x61005000)
            for index in range(16): put(0x61005000+index,(index*17+3)&255,1)
            put(header+0x1e,0 if kind=='missing' else 1,1)
            put(header+8,0);put(header+0x20,2)
            put(0x4f2d6c,0x61006000);put(0x61006064,0x61006100)
            age=0x61006200+0x14+2*24
            if kind.startswith('saved-'): age=stack-{'saved-si':12,'saved-di':8,'saved-bx':4}[kind]
            if kind=='lod-slot':age=stack+12
            put(0x61006144,age-0x14-2*24);put(0x61006038,3 if kind=='lod-slot' else 0x89abcdef)
            put(0x4ecbd0,0x61007000);put(0x61007044,0x61007100)
            put(0x61007100+3*12+8,0x61007400);put(0x61007516,bias,2)
            for index,value in enumerate((0x63000000,selector_table,selector_index,2)):put(stack+index*4,value)
            inputs.write(struct.pack('<I11I',0x235e0,*gp,flags,0x1f80,0x27f))
            for lane in xmm:inputs.write(struct.pack('<4I',*lane))
            inputs.write(b''.join(banks))
            rows.append(dict(id=ordinal,name=f'{kind}-{selector}-{bias}',entry=0x235e0,gp=gp,
                eflags=flags,matrix=selector_table,source=selector_index,max_steps=100000,xmm=xmm,
                before_fp=[0x27f,0,0x1f80],live_x87=[],
                banks=[dict(address=base,before=bank.hex()) for (base,_),bank in zip(REGIONS,banks)]))
            receipts.append(dict(case=kind,selector=selector,bias=bias))
    physical,proof=physical_original(image,rows,scratch,entries=(0x235e0,0x130980),regions=REGIONS)
    with expected.open('wb') as outputs:
        for receipt,original in zip(receipts,physical['cases'],strict=True):
            if not original['completed'] or not original['live_x87_unchanged']:
                raise ValueError('Original cache did not complete with x87 state preserved')
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
            receipt.update(stream_start=start,stream_bytes=outputs.tell()-start,stores=len(original['writes']))
    with inputs_path.open('rb') as inputs,actual.open('wb') as outputs:
        subprocess.run([str(native)],stdin=inputs,stdout=outputs,check=True)
    digest=lambda path:hashlib.sha256(path.read_bytes()).hexdigest()
    if digest(actual)!=digest(expected):
        left,right=actual.read_bytes(),expected.read_bytes()
        first=next((n for n,(a,b) in enumerate(zip(left,right)) if a!=b),min(len(left),len(right)))
        receipt=next((r for r in receipts if r['stream_start']<=first<r['stream_start']+r['stream_bytes']),{})
        raise ValueError(f'235E0 mismatch{first} local{first-receipt.get("stream_start",0)} {receipt}: '
                         f'{left[first:first+16].hex()} vs{right[first:first+16].hex()}')
    from model_prt_cache_native_replacements import replace_prt_cache,PREFIX
    generated=(ROOT/'src/rendering/models/source_model_render_tu15_generated.cpp').read_text()
    if replace_prt_cache(generated)!=generated:raise ValueError('235E0 production native route absent')
    body=re.search(r'static void run_235e0\(WorldState &s\) \{.*?\n\}',generated,re.S).group()
    rejected=0
    for candidate in (body,body.replace(PREFIX,'',1)):
        for old,new in (('0x4f0414u','0x4f0418u'),('0x130980u','0x130984u'),
                        ('if(s.flags.zf)','if(!s.flags.zf)')):
            changed=candidate.replace(old,new,1)
            if changed==candidate:raise ValueError('235E0 mutation witness missing')
            try:replace_prt_cache(generated.replace(body,changed,1))
            except ValueError:rejected+=1
            else:raise ValueError('235E0 binder accepted changed whole body')
    paths=('src/rendering/models/native_tu15_prt_cache.hpp','tests/native_tu15_prt_cache_tests.cpp',
           'tools/validate_native_tu15_prt_cache.py','tools/model_prt_cache_native_replacements.py',
           'tools/generate_model_render_tu15.py','src/rendering/models/source_model_render_tu15_generated.cpp',
           'cmake/NativeTu15PrtSample.cmake','tools/validate_native_tu15_prt_helpers.py')
    args.report.parent.mkdir(parents=True,exist_ok=True)
    args.report.write_text(json.dumps(dict(passed=True,cases=receipts,physical_original=proof,
        original_xbe_sha256=PIN,child_body_sha256=CHILD_PIN,native_parent_instruction_callbacks=0,
        child_owner='genuine retained130980 reference body, resident/missing paths',
        resource_request_children_qualified=False,read_fault_prefix_abi_qualified=False,
        rejected_semantic_mutations=rejected,
        compared_bytes=actual.stat().st_size,comparison_sha256=digest(actual),
        source_sha256={path:digest(ROOT/path) for path in paths}),indent=2,sort_keys=True)+'\n')
    print(f'PASS {len(cases)} physical235E0 cache cases with genuine130980 child')
    return 0


if __name__=='__main__':raise SystemExit(main())
