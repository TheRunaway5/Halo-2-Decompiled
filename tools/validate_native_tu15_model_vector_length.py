#!/usr/bin/env python3
"""Compare native length/distance and their extra ST0 against unchanged i386."""
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
COPIES=('src/rendering/models/source_model_render_tu15_generated.cpp',
        'src/campaign/ai/source_ai_frame_tu15_generated.cpp',
        'src/rendering/renderer/source_scene_device_constructor_tu15_generated.cpp')


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--xbe',type=Path,required=True)
    parser.add_argument('--native',type=Path)
    parser.add_argument('--report',type=Path,required=True)
    parser.add_argument('--require-binding',action='store_true')
    parser.add_argument('--sanitizers',action='store_true')
    args=parser.parse_args()
    if sys.platform!='linux' or platform.machine().lower() not in ('x86_64','amd64','i386','i686'):
        print('SKIP original length/distance requires Linux x86 ptrace');return 77
    if not args.xbe.is_file():print('SKIP missing original TU1.5 image');return 77
    digest=lambda path:hashlib.sha256(path.read_bytes()).hexdigest()
    if digest(args.xbe)!=PIN:raise ValueError('Wrong original TU1.5 image')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH'])
    sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
    from xbe import Xbe
    image=Xbe(args.xbe);rng=random.Random(0x245703ea20)
    native=args.native or scratch/'native-vector-length'
    if not args.native:
        command=['c++','-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math',
            '-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),str(ROOT/'tests/native_tu15_model_vector_length_tests.cpp'),
            '-o',str(native)]
        if args.sanitizers:command[1:1]=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
        subprocess.run(command,check=True)
    modes=('ordinary','zero','negative-zero','snan-x','snan-y','snan-z','qnan-x','qnan-y','qnan-z',
           'infinity','denormal','large','tiny','unaligned','same','forward-overlap','backward-overlap',
           'stack-vector','opposite-infinity','dual-nan')
    cases=[(entry,mode,precision,rounding) for entry in (0x24570,0x3ea20)
        for mode in modes for precision in range(3) for rounding in range(4)]
    cases += [(entry,'random',i%3,i%4) for entry in (0x24570,0x3ea20) for i in range(64)]
    inputs_path,expected,actual=(scratch/name for name in ('length.in','length.expected','length.actual'))
    rows=[];receipts=[]
    with inputs_path.open('wb') as inputs:
        for ordinal,(entry,mode,precision,rounding) in enumerate(cases):
            banks=[bytearray(rng.randbytes(size)) for _,size in REGIONS]
            def put(at,value):
                for (base,size),bank in zip(REGIONS,banks):
                    if base<=at<=base+size-4:
                        struct.pack_into('<I',bank,at-base,value);return
                raise ValueError('Length fixture escaped declared banks')
            gp=[rng.getrandbits(32) for _ in range(8)];stack=0x62001000;gp[4]=stack
            first=stack+12 if mode=='stack-vector' else 0x61001000+(mode=='unaligned')
            second={'same':first,'forward-overlap':first+4,'backward-overlap':first-4}.get(mode,0x61002000)
            gp[0]=first;gp[1]=second
            left=[0x3e99999a,0xbef5c28f,0x3f47ae14];right=[0xbe4ccccd,0x3f147ae1,0x3e3851ec]
            if mode=='zero':left=[0]*3;right=[0]*3
            if mode=='negative-zero':left=[0x80000000]*3;right=[0]*3
            if mode=='random':left=[rng.getrandbits(32) for _ in range(3)];right=[rng.getrandbits(32) for _ in range(3)]
            if mode.startswith(('snan','qnan')):
                left['xyz'.index(mode[-1])]=0xff812345 if mode.startswith('snan') else 0x7fcabcde
            if mode=='dual-nan':left=[0x7fc12345,0xffc56789,0x7f812345];right=[0xffcabcde,0x7fc00001,0x7fc11111]
            if mode in ('infinity','opposite-infinity'):left=[0x7f800000,0xff800000,0x7f800000]
            if mode=='opposite-infinity':right=[0x7f800000,0x7f800000,0xff800000]
            if mode=='denormal':left=[1,0x80000002,0x007fffff];right=[0x80000001,2,0x807fffff]
            if mode=='large':left=[0x7f7fffff,0xff7fffff,0x7f000001];right=[0xff7fffff,0x7f7fffff,0xff000001]
            if mode=='tiny':left=[0x00800001,0x80800002,0x01000003];right=[0x00800000,0x80800001,0x01000002]
            for i,value in enumerate(right):put(second+4*i,value)
            for i,value in enumerate(left):put(first+4*i,value)
            put(stack,0x63000000)
            flags=0x202|(rng.getrandbits(12)&0xcd5)
            mxcsr=0x1f80|((ordinal%4)<<13)|(0x8040 if ordinal&4 else 0)
            cw=(0x7f,0x27f,0x37f)[precision]|(rounding<<10)
            status=((ordinal%8)<<11)|((ordinal%8)<<8)|(0x4000 if ordinal&8 else 0)|((ordinal>>4)&63)
            live_values=('0000000000000080ff3f','00000000000000000080',
                         '35c26821a2da0fc90040','45230100000000c0ff7f')
            depths=(0,1,3 if entry==0x24570 else 4)
            live=list(live_values[:depths[ordinal%3]])
            xmm=[[rng.getrandbits(32) for _ in range(4)] for _ in range(8)]
            inputs.write(struct.pack('<I12I',entry,*gp,flags,mxcsr,cw,status))
            inputs.write(struct.pack('<I',len(live)))
            for value in live:inputs.write(bytes.fromhex(value))
            for lane in xmm:inputs.write(struct.pack('<4I',*lane))
            inputs.write(b''.join(banks))
            rows.append(dict(id=ordinal,name=f'{entry:x}-{mode}-{precision}-{rounding}',entry=entry,gp=gp,eflags=flags,
                matrix=struct.unpack_from('<I',banks[3],stack+4-REGIONS[3][0])[0],
                source=struct.unpack_from('<I',banks[3],stack+8-REGIONS[3][0])[0],max_steps=100,
                xmm=xmm,before_fp=[cw,status,mxcsr],live_x87=live,
                banks=[dict(address=base,before=bank.hex()) for (base,_),bank in zip(REGIONS,banks)]))
            receipts.append(dict(entry=entry,case=mode,precision=precision,rounding=rounding,live_depth=len(live)))
    physical,proof=physical_original(image,rows,scratch,entries=(0x24570,0x3ea20),regions=REGIONS,return_x87=True)
    with expected.open('wb') as outputs:
        for receipt,case in zip(receipts,physical['cases'],strict=True):
            if not case['completed'] or not case['live_x87_unchanged'] or case['writes']:
                raise ValueError('Original length/distance completion/store contract changed')
            start=outputs.tell();outputs.write(struct.pack('<12I',*case['boundary_gp'],case['boundary_eflags'],
                case['after_fp'][2],case['after_fp'][1],case['after_fp'][0]))
            outputs.write(bytes.fromhex(case['returned_extended']))
            for lane in case['boundary_xmm']:outputs.write(struct.pack('<4I',*lane))
            outputs.write(b''.join(bytes.fromhex(bank['after']) for bank in case['banks']))
            outputs.write(struct.pack('<I',0))
            receipt.update(stream_start=start,stream_bytes=outputs.tell()-start)
    with inputs_path.open('rb') as inputs,actual.open('wb') as outputs:
        subprocess.run([str(native)],stdin=inputs,stdout=outputs,check=True)
    if digest(actual)!=digest(expected):
        left,right=actual.read_bytes(),expected.read_bytes()
        first=next((i for i,(a,b) in enumerate(zip(left,right)) if a!=b),min(len(left),len(right)))
        receipt=next((r for r in receipts if r['stream_start']<=first<r['stream_start']+r['stream_bytes']),{})
        raise ValueError(f'Length mismatch byte{first} local{first-receipt.get("stream_start",0)} {receipt}: '
            f'{left[first:first+16].hex()} vs {right[first:first+16].hex()}')
    rejected=0
    if args.require_binding:
        import model_vector_length_native_replacements as binding
        for path in COPIES:
            generated=(ROOT/path).read_text();bind=binding.replace_model_vector_length
            if bind(generated)!=generated:raise ValueError('Length native production route absent')
            for entry in binding.PINS:
                match=re.search(rf'static void run_{entry:x}\(WorldState &s\) \{{.*?\n\}}',generated,re.S)
                if not match:continue
                body=match.group();literal=re.sub(r'selected\((0x[0-9a-f]+u),s.image\)',r'\1',body)
                for shape in (body,literal):
                    if bind(generated.replace(body,shape,1))!=generated.replace(body,shape,1):
                        raise ValueError('Length body shape not idempotent')
                    for candidate in (shape,shape.replace(binding.prefix(entry),'',1)):
                        changed=candidate.replace(',4)',',2)',1)
                        if changed==candidate:raise ValueError('Missing length mutation witness')
                        try:bind(generated.replace(body,changed,1))
                        except ValueError:rejected+=1
                        else:raise ValueError('Length binder accepted semantic body drift')
    paths=(*COPIES,'src/rendering/models/native_tu15_model_vector_length.hpp',
        'tools/model_vector_length_native_replacements.py','tests/native_tu15_model_vector_length_tests.cpp',
        'tools/validate_native_tu15_model_vector_length.py','tools/validate_native_tu15_prt_helpers.py',
        'src/physics/runtime/physics_contact_runtime_internal.hpp','src/physics/runtime/physics_step_internal.hpp',
        'src/physics/runtime/physics_solver_internal.hpp','src/rendering/models/native_tu15_prt_basis.hpp',
        'tools/native_selected_operand_binding.py','tools/generate_model_render_tu15.py',
        'tools/generate_ai_frame_tu15.py','tools/generate_scene_device_constructor_tu15.py',
        'cmake/NativeTu15ModelViewSlots.cmake')
    args.report.parent.mkdir(parents=True,exist_ok=True)
    args.report.write_text(json.dumps(dict(passed=True,cases=receipts,original_xbe_sha256=PIN,physical_original=proof,
        compared_bytes=actual.stat().st_size,comparison_sha256=digest(actual),native_instruction_callbacks=0,
        returned_extended_80_bits_verified=True,live_x87_inputs_verified=True,x87_top_values=list(range(8)),
        rejected_semantic_mutations=rejected,memory_fault_prefix_abi_verified=False,
        x87_stack_overflow_abi_verified=False,ordered_read_fault_semantics_verified=False,
        scratch_owner=str(scratch),
        source_sha256={path:digest(ROOT/path) for path in paths}),indent=2,sort_keys=True)+'\n')
    print(f'PASS {len(cases)} native length/distance cases including full extended return')
    return 0


if __name__=='__main__':raise SystemExit(main())
