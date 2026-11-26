#!/usr/bin/env python3
"""Verify 22AD60 and the original 250AC0 matrix edges on physical i386."""

import argparse
import copy
import json
import os
from pathlib import Path
import random
import struct
import subprocess
import sys

import native_particle_emitter_bound_replacements as emitter
import native_particle_transform_matrix_calls_replacements as matrix
from specialize_native_operands import specialize
from instrument_native_execution import instrument_translation
from validate_native_particle_transform_basis_tu15 import ROOT, UNIT, IMAGE, sha
from validate_native_tu15_prt_helpers import physical_original


REGIONS=((0x46a000,4096),(0x61000000,4096),(0x62000000,4096))
MACHINE='d3ffa48dc89194066282796e634c368601e0bef4a8ceebaa9b9d0231db43fe58'
BOUND=0x22ad60
TEST=ROOT/'tests/native_tu15_particle_emitter_bound_tests.cpp'
X87=('0000000000000080ff3f','00000000000000000080','35c26821a2da0fc90040',
     '45230100000000c0ff7f','00000000000000800040','00000000000000c00040','00000000000000a00140')


def input_rows(image):
    rng=random.Random(BOUND)
    emitters,matrices=[],[]
    finite=(0x3e99999a,0xbe99999a,0x3f000000,0xbf000000,0x41000000,0xbd7fffff,0x3fb7ae14)
    special=(0,0x80000000,1,0x807fffff,0x7f800000,0xff800000,0x7f800001,0xffc10abc)

    def start(n,name,entry,last):
        banks={base:bytearray(rng.randbytes(size)) for base,size in REGIONS}
        gp=[rng.getrandbits(32) for _ in range(8)]
        gp[4]=0x62000800+(n&3)
        row=dict(id=n,name=name,entry=entry,gp=gp,last_pc=last,matrix=rng.getrandbits(32),source=rng.getrandbits(32),
            eflags=0x202|(rng.getrandbits(12)&0xcd5),before_fp=[0x37f|((n&3)<<10),((n&7)<<11)|((n&3)<<8)|(n&0x3f),
            0x1f80|((n&3)<<13)|(n&0x3f)|((n&1)<<15)|((n&2)<<5)],
            live_x87=list(X87[:n%8]),xmm=[[rng.getrandbits(32) for _ in range(4)] for _ in range(8)],
            max_steps=160,stop_before_entries=[],strict_writes=True)
        def put(at,value,width=4):
            base=at &~4095
            banks[base][at-base:at-base+width]=value.to_bytes(width,'little')
        return row,banks,put

    for n in range(80):
        row,banks,put=start(n,f'emitter/{n}',BOUND,0x22ada4)
        sp=row['gp'][4]
        instance=sp-12 if n%5==1 else 0x61000200+n%4
        header=instance if n%5==1 else 0x61000400+n%4
        descriptor={2:sp-8,3:sp-12}.get(n%5,0x61000600+n%4)
        row['gp'][1]=instance
        put(instance+8,header)
        put(header+0x48,descriptor)
        payload=finite+special
        left=payload[n%len(payload)];right=payload[(n//2+4)%len(payload)]
        # Distinct winner, equality, both NaN categories and imported default.
        if n in (0,3): left,right=0x3e99999a,0x41000000
        if n in (8,16): left,right=0x41000000,0x3e99999a
        if n==24: left=right=0x3f800000
        put(descriptor+4,left);put(descriptor+8,right)
        metadata=0x20|(n&0xf) if n%4==3 else n&0xf
        put(descriptor+1,metadata,1)
        put(0x46a0d0,int.from_bytes(image.read(0x46a0d0,4),'little'))
        if n in (0,3,8,16,24,30,31):row['reopen']=True
        row['banks']=[dict(address=address,before=bank.hex()) for address,bank in banks.items()]
        emitters.append(row)

    for entry,successor,last in ((0x250c0c,0x250c11,0x1459a0),(0x250c17,0x250c1c,0x145b64)):
        for n in range(8):
            row,banks,put=start(n,f'matrix/{entry:x}/{n}',entry,last)
            sp=row['gp'][4]
            if entry==0x250c0c:
                row['gp'][0]=0x61000200 if n&1 else sp+0x24
                angles=(0x80000000,0,0) if n&1 else (0,0,0)
                row['return_pc'],row['matrix'],row['source']=angles
            else:
                out=0x61000200;left=out if n&1 else 0x61000400;right=out if n&2 else 0x61000600
                row['gp'][0],row['gp'][1],row['gp'][2]=out,right,left
                for block in {out,left,right}:
                    for at in range(0,0x34,4):put(block+at,rng.choice(finite))
            row['stop_before_entries']=[successor]
            row['reopen']=n in (0,1)
            row['banks']=[dict(address=address,before=bank.hex()) for address,bank in banks.items()]
            matrices.append(row)
    return emitters,matrices


def faults(rows,proof,all_accesses):
    generated=[]
    for row,observed in zip(rows,proof['cases'],strict=True):
        if not row.get('reopen'):continue
        for phase,accesses in (('before_read',observed['reads']),('before_write',observed['writes']),('after_write',observed['writes'])):
            ordinals=range(len(accesses)) if all_accesses else ((0,len(accesses)-1) if phase=='before_read' else (0,))
            for ordinal in dict.fromkeys(ordinals):
                item=copy.deepcopy(row)
                item.pop('reopen')
                item.update(name=f'{row["name"]}/{phase}/{ordinal}',fault=dict(phase=phase,ordinal=ordinal))
                generated.append(item)
    return generated


def native_helpers(source):
    result=[]
    for name in ('native_single_spill','native_fcos','native_fsin'):
        start=source.index(' '+name+'(')
        start=source.rfind('\n',0,start)+1
        stop=source.index('\n}',start)+2
        result.append(source[start:stop])
    return '\n'.join(result)


def prepare(scratch):
    canonical=UNIT.read_text()
    ordinary,receipt=specialize(canonical)
    if any(receipt['native_spatial_owners'].count(owner)!=1 for owner in ('22ad60','250c0c','250c17')):
        raise ValueError('Current emitter and matrix production binding is absent')
    world=emitter.world
    parent=world.body(ordinary,'250ac0')[0]
    for site,returned,_ in matrix.projection.CALLS:
        hook=f'L{site}:\n'
        if parent.count(hook)!=1 or parent.count(f'::call<0x{site}u>')!=1:
            raise ValueError(f'Missing prepared matrix call {site}')
        anchor=f'L{returned}:\n'
        if parent.count(anchor)!=1:raise ValueError('Missing original return label')
        parent=parent.replace(anchor,anchor+f'    if (fixture_return==0x{returned}u) return;\n',1)
    hook='    std::uint32_t projection_next{};'
    if parent.count(hook)!=1:raise ValueError('Missing prepared parent entry')
    parent=parent.replace(hook,hook+'\n    if (fixture_entry==0x250c0cu) goto L250c0c;\n'
        '    if (fixture_entry==0x250c17u) goto L250c17;',1)
    body=world.body(ordinary,'22ad60')[0]
    if body.count(emitter.HOOK)!=1 or body.count('s.instruction_literal(')!=23:raise ValueError('Missing complete emitter body')
    dependencies=native_helpers(ordinary)+'\n'+ '\n'.join(world.body(ordinary,callee)[0] for callee in matrix.CALLEE_PINS)
    (scratch/'particle_emitter_bound_owner.inc').write_text(dependencies+'\n'+parent+'\n'+body+'\n')
    counted,manifest,_=instrument_translation(ordinary,canonical,UNIT.relative_to(ROOT).as_posix(),ROOT)
    sites={row['symbol']:row['literal_sites'] for row in manifest['functions'] if row['symbol'] in ('22ad60','250ac0')}
    if sites!={'22ad60':23,'250ac0':114}:raise ValueError('Emitter/matrix accounting lost original PCs')
    return ordinary,counted,sites


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
    if sha(image.data)!=IMAGE or sha(image.read(BOUND,0x45))!=MACHINE:
        raise ValueError('Emitter original owner image drifted')
    for site,returned,target in matrix.projection.CALLS:
        pc=int(site,16);call=image.read(pc,5)
        if call[0]!=0xe8 or pc+5!=int(returned,16) or pc+5+struct.unpack('<i',call[1:])[0]!=int(target,16):
            raise ValueError('Original matrix CALL drifted')
    if image.read(0x1459a0,3)!=bytes.fromhex('c20c00') or image.read(0x145b64,1)!=b'\xc3':
        raise ValueError('Original matrix RET cleanup drifted')
    ordinary,counted,manifest=prepare(scratch)
    rows,observations,evidence=[],[],{}
    for name,inputs in zip(('emitter','matrix'),input_rows(image),strict=True):
        common=dict(entries=(BOUND,) if name=='emitter' else (0x250ac0,0x1458a0,0x1459b0),regions=REGIONS,
                    capture_boundary_x87=True,return_x87=name=='emitter')
        normal_dir=scratch/(name+'-normal');normal_dir.mkdir()
        original,proof=physical_original(image,inputs,normal_dir,**common)
        rejects=faults(inputs,original,name=='emitter')
        reject_dir=scratch/(name+'-faults');reject_dir.mkdir()
        faulted,fault_proof=physical_original(image,rejects,reject_dir,**common)
        sites=set().union(*(c['executed_sites'] for c in original['cases']))
        if name=='emitter' and len(sites)!=23:raise ValueError('Emitter did not observe all 23 original sites')
        evidence[name]=dict(normal=proof,faults=fault_proof,sites=sorted(sites))
        rows.extend(inputs+rejects);observations.extend(original['cases']+faulted['cases'])
    stores=sorted({event['instruction'] for case in observations for event in case['writes']})
    fixture=scratch/'emitter-and-matrix-oracle.json'
    fixture.write_text(json.dumps(dict(input=rows,expected=observations,store_sites=stores),separators=(',',':')))
    command=[args.compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math',
             f'-I{ROOT/"src"}',f'-I{ROOT/"include"}',f'-I{scratch}']
    env={k:v for k,v in os.environ.items() if k not in ('HALO2_REFERENCE_PARTICLE_EMITTER_BOUND','HALO2_REFERENCE_PARTICLE_MATRIX_CALLS',
        'HALO2_REFERENCE_PHYSICS_WORLD_LEAVES','HALO2_CENSUS_NATIVE_BRIDGES')}
    profiles={}
    for profile,flags in (('release',[]),('sanitized',['-O1','-fsanitize=address,undefined',
            '-fno-sanitize-recover=all','-fno-omit-frame-pointer','-fno-pie','-no-pie'])):
        binary=scratch/(profile+'-emitter-and-matrix')
        subprocess.run([*command,*flags,str(TEST),'-o',str(binary)],cwd=scratch,check=True)
        result=subprocess.run([binary,fixture],cwd=scratch,env=env,capture_output=True,text=True)
        if result.returncode:raise ValueError(f'{profile}: {result.stderr}')
        profiles[profile]=json.loads(result.stdout)
    args.report.parent.mkdir(parents=True,exist_ok=True)
    args.report.write_text(json.dumps(dict(passed=True,scratch=str(scratch),physical=evidence,profiles=profiles,
        compiled_alias_sha256=sha(ordinary),accounted_alias_sha256=sha(counted),manifest=manifest,
        genuine_calls=matrix.projection.CALLS,machine_sha256=MACHINE,retained_large_artifacts=[]),indent=2)+'\n')
    print(f'Native emitter and matrix shell physical proof passed: {profiles}')


if __name__=='__main__':
    main()
