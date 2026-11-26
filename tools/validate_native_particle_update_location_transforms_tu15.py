#!/usr/bin/env python3
"""Verify the native 251F70 particle location transform walk on physical i386."""

import argparse
import copy
import json
import os
from pathlib import Path
import random
import struct
import subprocess
import sys

import native_particle_update_location_transforms_replacements as binding
from specialize_native_operands import specialize
from instrument_native_execution import instrument_translation
from validate_native_particle_transform_basis_tu15 import ROOT, UNIT, IMAGE, sha
from validate_native_tu15_prt_helpers import physical_original


FIRST, END = 0x251f70, 0x251fcd
MACHINE = '752f414bdb6a9c09fee95b26c5dea323ffae47918f9ab866e3603e6d544c208c'
REGIONS = ((0x46a000,4096),(0x4f0000,4096),(0x52d000,4096),(0x61000000,4*4096),(0x62000000,4096))
TEST = ROOT/'tests/native_tu15_particle_update_location_transforms_tests.cpp'
HEADER = ROOT/'src/rendering/effects/native_tu15_particle_update_location_transforms.hpp'


def inputs(image):
    rng=random.Random(FIRST)
    rows=[]
    x87=('0000000000000080ff3f','00000000000000000080','35c26821a2da0fc90040')
    modes=('empty','unknown-empty','zero-count','negative-count','minimum-count','maximal-count','short-chain',
           'short-definition','full-transform','effect-tag','live-count','live-pool')
    for mode in modes:
        for trial in range(5):
            number=len(rows)
            banks={base:bytearray(rng.randbytes(size)) for base,size in REGIONS}
            def put(address,value,width=4):
                for base,size in REGIONS:
                    if base<=address<=base+size-width:
                        raw=struct.pack('<f',value) if isinstance(value,float) else value.to_bytes(width,'little')
                        banks[base][address-base:address-base+width]=raw
                        return
                raise ValueError(f'Fixture address outside its banks {address:x}')
            offset=trial&3
            location,context,table,tag=(0x61000200+offset,0x61000400+offset,0x61000600+offset,0x61000800+offset)
            definition,authored,pool,pool2=(0x61000a00+offset,0x61001000+offset,0x61001800+offset,0x61001900+offset)
            storage,storage2,matrix=(0x61002000+offset,0x61002400+offset,0x61002a00+offset)
            handles=(0xa51c0002,0x48270004,0xf5330001)
            first_slot=handles[0]&0xffff
            kinds={'unknown-empty':0xdeadbeef,'effect-tag':0x65666665}
            counts={'zero-count':0,'negative-count':0xffffffff,'minimum-count':0x80000000,
                    'maximal-count':0x7fffffff,'short-definition':1}
            if mode=='live-count':definition=storage+first_slot*0x4c-0x20
            if mode=='live-pool':storage=0x52d718-0x10-first_slot*0x4c
            put(location+4,0xffffffff if mode in ('empty','unknown-empty') else handles[0])
            put(0x4f0414,table);put(context+0x10,1);put(context+0xe,0,2)
            put(table+0x10,kinds.get(mode,0x62736474));put(table+0x18,tag)
            if mode=='effect-tag':
                raw_event=0x61000b00+offset
                put(context+0x14,1);put(tag+0x18,raw_event);put(raw_event+0x38+0x34,definition)
            else:put(tag+0x18,definition)
            put(definition+0x30,counts.get(mode,3));put(definition+0x34,authored)
            for index,handle in enumerate(handles):
                for storage_base in (storage,storage2):
                    datum=storage_base+(handle&0xffff)*0x4c
                    put(datum+8,handles[index+1] if index+1<len(handles) and mode!='short-chain' else 0xffffffff)
                for displacement in (0x9c,0xa0,0xa4,0xa8,0xac):
                    put(authored+0xb8*index+displacement,float(index+trial)/4.0 if displacement<0xa8 else 0.0)
            put(0x52d718,pool);put(pool+0x44,storage);put(pool2+0x44,storage2)
            put(0x46a0ec,int.from_bytes(image.read(0x46a0ec,4),'little'))
            for i in range(1,13):put(matrix+i*4,(i+trial)/8.0)
            if mode=='live-count':put(matrix+4,0)
            if mode=='live-pool':put(matrix+4,pool2)
            gp=[rng.getrandbits(32) for _ in range(8)]
            gp[0],gp[1],gp[4]=location,context,0x62000800+offset
            transform=matrix if mode in ('full-transform','effect-tag','live-count','live-pool') else 0
            rows.append(dict(name=f'{mode}/{trial}',id=number,entry=FIRST,gp=gp,
                matrix=transform,source=rng.getrandbits(32),eflags=0x202|(rng.getrandbits(12)&0xcd5),
                before_fp=[0x37f|((trial&3)<<10),((trial&7)<<11)|((trial&3)<<8)|(trial&0x3f),0x1f80|((trial&3)<<13)|(trial&0x3f)],
                live_x87=list(x87[:trial%4]),xmm=[[rng.getrandbits(32) for _ in range(4)] for _ in range(8)],
                banks=[dict(address=address,before=bank.hex()) for address,bank in banks.items()],max_steps=1500,
                strict_writes=True,stop_before_entries=[],reopen=trial==0 and mode in ('empty','maximal-count','full-transform','live-count','live-pool')))
    return rows


def prepare(scratch):
    canonical=UNIT.read_text()
    ordinary,receipt=specialize(canonical)
    if receipt.get('native_spatial_owners',[]).count(binding.OWNER)!=1 or binding.install(canonical,ordinary)!=(ordinary,[]):
        raise ValueError('Expected one exact active 251F70 specialization')
    owner=binding.world.body(ordinary,binding.OWNER)[0]
    if owner.count(binding.HOOK)!=1 or owner.count('s.instruction_literal(')!=35:
        raise ValueError('Prepared active location transform owner drifted')
    (scratch/'particle_update_location_transforms_owner.inc').write_text(owner)
    counted,manifest,_=instrument_translation(ordinary,canonical,UNIT.relative_to(ROOT).as_posix(),ROOT)
    metadata,=[row for row in manifest['functions'] if row['symbol']==binding.OWNER]
    if metadata['literal_sites']!=35:
        raise ValueError('Location transform lost original counted sites')
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
    if sha(image.data)!=IMAGE or sha(image.read(FIRST,END-FIRST))!=MACHINE or image.read(0x251fca,3)!=b'\xc2\x08\x00':
        raise ValueError('Original 251F70 owner or RET 8 drifted')
    for site,returned,target in (*binding.CALLS,('179fe1','179fe6','251f70')):
        pc=int(site,16);call=image.read(pc,5)
        if call[0]!=0xe8 or pc+5!=int(returned,16) or pc+5+struct.unpack('<i',call[1:])[0]!=int(target,16):
            raise ValueError(f'Original emitter transform CALL drifted at {site}')
    ordinary,counted,manifest,owner=prepare(scratch)
    ports=(0x178070,0x251f7e,0x250ac0,0x251fb7)
    common=dict(entries=(FIRST,0x178070,0x250ac0),regions=REGIONS,capture_boundary_x87=True,trace_extended=True,
                trace_calls=[dict(entry=pc,arguments=0,gp=True,xmm=True) for pc in ports])
    rows=inputs(image)
    cases=scratch/'normal';cases.mkdir()
    normal,original=physical_original(image,rows,cases,**common)
    successful={row['name']:case for row,case in zip(rows,normal['cases'],strict=True)}
    child_counts={name:sum(event['entry']==0x250ac0 for event in case['calls']) for name,case in successful.items()}
    if (child_counts['full-transform/0'],child_counts['live-count/0'],child_counts['live-pool/0'])!=(3,1,3):
        raise ValueError('Original normal or provider-mutation control missed its executed path')
    rejected=[]
    for row,case in zip(rows,normal['cases'],strict=True):
        if not row.get('reopen'):continue
        for phase,accesses in (('before_read',case['reads']),('before_write',case['writes']),('after_write',case['writes'])):
            for at,event in enumerate(accesses):
                if not FIRST<=event['instruction']<END:continue
                fault=copy.deepcopy(row)
                fault.pop('reopen')
                fault.update(id=len(rows)+len(rejected),name=f'{row["name"]}/{phase}/{at}',fault=dict(phase=phase,ordinal=at))
                rejected.append(fault)
    cases=scratch/'faults';cases.mkdir()
    faults,fault_evidence=physical_original(image,rejected,cases,**common)
    observed=normal['cases']+faults['cases']
    sites=sorted({pc for case in observed for pc in case['executed_sites'] if FIRST<=pc<END})
    if len(sites)!=35:
        raise ValueError(f'Original location transform scope observed {len(sites)} of 35 sites')
    fixture=scratch/'location-transforms-oracle.json'
    fixture.write_text(json.dumps(dict(input=rows+rejected,expected=observed),separators=(',',':')))
    command=[args.compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math',
             f'-I{ROOT/"src"}',f'-I{ROOT/"include"}',f'-I{scratch}']
    env={k:v for k,v in os.environ.items() if k not in ('HALO2_REFERENCE_PARTICLE_LOCATION_TRANSFORMS',
        'HALO2_REFERENCE_PHYSICS_WORLD_LEAVES','HALO2_CENSUS_NATIVE_BRIDGES')}
    profiles={}
    for profile,flags in (('release',[]),('sanitized',['-O1','-fsanitize=address,undefined',
            '-fno-sanitize-recover=all','-fno-omit-frame-pointer','-fno-pie','-no-pie'])):
        executable=scratch/(profile+'-location-transforms')
        subprocess.run([*command,*flags,str(TEST),'-o',str(executable)],cwd=scratch,check=True)
        result=subprocess.run([executable,fixture],cwd=scratch,env=env,capture_output=True,text=True)
        if result.returncode:raise ValueError(f'{profile} 251F70 hardware comparison failed: {result.stderr}')
        profiles[profile]=json.loads(result.stdout)
    args.report.parent.mkdir(parents=True,exist_ok=True)
    args.report.write_text(json.dumps(dict(passed=True,scratch=str(scratch),profiles=profiles,
        physical=dict(normal=original,faults=fault_evidence),original_parent_sites=sites,original_machine_sha256=MACHINE,
        genuine_calls=binding.CALLS,authored_transform_visits=child_counts,manifest=manifest,
        alias_sha256=sha(ordinary),accounted_alias_sha256=sha(counted),prepared_owner_sha256=sha(owner),
        source_sha256={str(path.relative_to(ROOT)):sha(path.read_bytes()) for path in (UNIT,HEADER,TEST,Path(binding.__file__),Path(__file__))},
        retained_large_artifacts=[]),indent=2)+'\n')
    print(f'Native 251F70 location transform walk passed {profiles}')


if __name__=='__main__':
    main()
