#!/usr/bin/env python3
"""Compare the native 250B50 projection/return stages to physical TU1.5."""

import argparse
import copy
import json
import os
from pathlib import Path
import random
import struct
import subprocess
import sys

import native_particle_transform_projection_replacements as binding
from validate_native_particle_transform_basis_tu15 import IMAGE, ROOT, UNIT, sha
from validate_native_tu15_prt_helpers import physical_original
from specialize_native_operands import specialize
from instrument_native_execution import instrument_translation


FIRST, PROJECT, END = 0x250ac0, 0x250b50, 0x250c51
MACHINE = 'e021803da53f4f1424409045db67e9b24869f38c4cb5bd9ad3de8fd2d4a59328'
REGIONS = ((0x46a000,4096),(0x61000000,4096),(0x62000000,4096))
TEST = ROOT/'tests/native_tu15_particle_transform_projection_tests.cpp'
HEADER = ROOT/'src/rendering/effects/native_tu15_particle_transform_projection.hpp'


def inputs(image):
    rng=random.Random(PROJECT)
    rows=[]
    x87=('0000000000000080ff3f','00000000000000000080','35c26821a2da0fc90040',
         '45230100000000c0ff7f','00000000000000800040','00000000000000c00040')
    modes=dict(project=(PROJECT,0,0), first=(PROJECT,0,0x3f800000), second=(PROJECT,0x3f800000,0),
               unordered=(PROJECT,0,0x7f800001), before_second=(0x250c11,0,0),
               after_second=(0x250c1c,0,0), empty=(0x250c4d,0,0), final=(0x250c2e,0,0))
    finite=(0x3e99999a,0xbe99999a,0x3f000000,0xbf000000,0x41000000,0xbd7fffff,0x3fb7ae14)
    special=(0,0x80000000,1,0x807fffff,0x7f800000,0xff800000,0x7f800001,0xffc10abc)
    for mode,(entry,a,b) in modes.items():
        for n in range(8):
            index=len(rows)
            banks={base:bytearray(rng.randbytes(size)) for base,size in REGIONS}
            def put(address,value):
                base=address &~4095
                struct.pack_into('<I',banks[base],address-base,value)
            sp,out,obj=0x62000800+(n&3),0x61000400,0x61000200
            shift=sp+0xc if n&1 else obj+0x9c
            if n==7:out=sp-0xd
            gp=[rng.getrandbits(32) for _ in range(8)]
            gp[0],gp[1],gp[2],gp[4],gp[6],gp[7]=shift,obj,out+0x10,sp,out,out+0x34
            for at in (*(out+p for p in range(0x10,0x40,4)),*(shift+p for p in (0,4,8)),
                        *(sp+p for p in range(0xc,0x18,4))):
                pool=finite if n<4 else special+finite
                put(at,rng.choice(pool))
            put(obj+0xa8,a);put(obj+0xac,b)
            put(sp+(0x3c if entry==0x250c4d else 0x48),0x63000000)
            put(0x46a0ec,int.from_bytes(image.read(0x46a0ec,4),'little'))
            mxcsr=0x1f80|((n&3)<<13)|(n&0x3f)|((n&1)<<15)|((n&2)<<5)
            row=dict(id=index,name=f'{mode}/{n}',entry=entry,gp=gp,matrix=rng.getrandbits(32),source=rng.getrandbits(32),
                eflags=0x202|(rng.getrandbits(12)&0xcd5),before_fp=[0x37f|((n&3)<<10),((n&7)<<11)|((n&3)<<8)|(n&0x3f),mxcsr],
                live_x87=list(x87[:index%7]),xmm=[[rng.getrandbits(32) for _ in range(4)] for _ in range(8)],
                banks=[dict(address=address,before=bank.hex()) for address,bank in banks.items()],max_steps=120,
                stop_before_entries=[0x1458a0,0x1459b0],strict_writes=True)
            if n==0:row['reopen']=True
            rows.append(row)
    return rows


def prepare(scratch):
    canonical=UNIT.read_text()
    ordinary,receipt=specialize(canonical)
    if not all(receipt['native_spatial_owners'].count(owner)==1 for owner in ('250ac0','250b50')):
        raise ValueError('Missing current 250AC0 and 250B50 production aliases')
    owner=binding.basis.world.body(ordinary,'250ac0')[0]
    if owner.count(binding.JUMP+':')!=1 or owner.count('s.instruction_literal(')!=114:
        raise ValueError('Prepared projection owner lost original sites')
    anchor='    std::uint32_t projection_next{};'
    instrumented=anchor+'\n    switch(fixture_entry) {\n'+''.join(
        f'    case 0x{pc}u: goto L{pc};\n' for pc in binding.ROUTES)+'    }'
    if owner.count(anchor)!=1:
        raise ValueError('Prepared projection fixture dispatch anchor drifted')
    (scratch/'particle_transform_projection_owner.inc').write_text(owner.replace(anchor,instrumented,1))
    counted,manifest,_=instrument_translation(ordinary,canonical,UNIT.relative_to(ROOT).as_posix(),ROOT)
    metadata,=[row for row in manifest['functions'] if row['symbol']=='250ac0']
    if metadata['literal_sites']!=114:
        raise ValueError('Accounted projection lost an original label')
    return ordinary,counted,metadata,owner


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--xbe',type=Path,required=True)
    parser.add_argument('--compiler',default='c++')
    parser.add_argument('--report',type=Path,required=True)
    args=parser.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH'])
    sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
    from xbe import Xbe
    image=Xbe(args.xbe)
    if sha(image.data)!=IMAGE or sha(image.read(FIRST,END-FIRST))!=MACHINE:
        raise ValueError('Projection original owner image drifted')
    for site,returned,target in binding.CALLS:
        address=int(site,16)
        call=image.read(address,5)
        if call[0]!=0xe8 or address+5!=int(returned,16) or address+5+struct.unpack('<i',call[1:])[0]!=int(target,16):
            raise ValueError('Projection original call identity drifted')
    ordinary,counted,manifest,owner=prepare(scratch)
    common=dict(entries=(FIRST,),regions=REGIONS,capture_boundary_x87=True)
    rows=inputs(image)
    case_dir=scratch/'normal';case_dir.mkdir()
    normal,original=physical_original(image,rows,case_dir,**common)
    faults=[]
    for case,original_case in zip(rows,normal['cases'],strict=True):
        if not case.get('reopen'):continue
        for phase,count in (('before_read',len(original_case['reads'])),('before_write',len(original_case['writes'])),
                            ('after_write',len(original_case['writes']))):
            for at in range(count):
                copy_row=copy.deepcopy(case)
                copy_row.pop('reopen')
                copy_row.update(id=len(rows)+len(faults),name=f'{case["name"]}/{phase}/{at}',fault=dict(phase=phase,ordinal=at))
                faults.append(copy_row)
    fault_dir=scratch/'faults';fault_dir.mkdir()
    fault,original_fault=physical_original(image,faults,fault_dir,**common)
    observations=normal['cases']+fault['cases']
    sites=set().union(*(case['executed_sites'] for case in observations))
    if len(sites)!=63:
        raise ValueError(f'Incomplete original projection coverage {len(sites)} of 63')
    comparison=scratch/'projection-oracle.json'
    comparison.write_text(json.dumps(dict(input=rows+faults,expected=observations),separators=(',',':')))
    env={k:v for k,v in os.environ.items() if k not in ('HALO2_REFERENCE_PARTICLE_TRANSFORM_BASIS',
        'HALO2_REFERENCE_PARTICLE_TRANSFORM_PROJECTION','HALO2_REFERENCE_PHYSICS_WORLD_LEAVES','HALO2_CENSUS_NATIVE_BRIDGES')}
    command=[args.compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math',
        f'-I{ROOT/"src"}',f'-I{ROOT/"include"}',f'-I{scratch}']
    profiles={}
    for profile,flags in (('release',[]),('sanitized',['-O1','-fsanitize=address,undefined',
            '-fno-sanitize-recover=all','-fno-omit-frame-pointer','-fno-pie','-no-pie'])):
        binary=scratch/('projection-'+profile)
        subprocess.run([*command,*flags,str(TEST),'-o',str(binary)],cwd=scratch,check=True)
        checked=subprocess.run([binary,comparison],cwd=scratch,env=env,capture_output=True,text=True)
        if checked.returncode: raise ValueError(f'{profile} projection differential failed: {checked.stderr}')
        profiles[profile]=json.loads(checked.stdout)
    args.report.parent.mkdir(parents=True,exist_ok=True)
    args.report.write_text(json.dumps(dict(passed=True,scratch=str(scratch),profiles=profiles,original=original,
        original_faults=original_fault,machine_sha256=MACHINE,original_sites=sorted(sites),true_genuine_calls=binding.CALLS,
        manifest=manifest,compiled_alias_sha256=sha(ordinary),accounted_alias_sha256=sha(counted),bound_owner_sha256=sha(owner),
        source_sha256={str(path.relative_to(ROOT)):sha(path.read_bytes()) for path in (UNIT,HEADER,TEST,Path(binding.__file__))},
        retained_large_artifacts=[]),indent=2)+'\n')
    print(f'Particle 250B50 tail passed {profiles}')


if __name__=='__main__':
    main()
