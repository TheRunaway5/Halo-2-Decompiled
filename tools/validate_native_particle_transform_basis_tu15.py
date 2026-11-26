#!/usr/bin/env python3
"""Verify the native 250AC0 basis span against the unchanged TU1.5 image."""

import argparse
import hashlib
import json
import os
from pathlib import Path
import random
import re
import struct
import subprocess
import sys

import native_particle_transform_basis_replacements as binding
from generated_routine_ownership import qualified_dispatch_entries
from instrument_native_execution import instrument_translation
from specialize_native_operands import specialize
from validate_native_tu15_prt_helpers import PIN as IMAGE, physical_original


ROOT = Path(__file__).resolve().parents[1]
UNIT = ROOT/'src/rendering/effects/source_effect_particle_frame_tu15_generated.cpp'
HEADER = ROOT/'src/rendering/effects/native_tu15_particle_transform_basis.hpp'
TEST = ROOT/'tests/native_tu15_particle_transform_basis_tests.cpp'
FIRST, TRANSFORM, LEAVE = 0x250ac0, 0x250b50, 0x250c4d
REGIONS = ((0x61000000,4096),(0x62000000,4096))
MACHINE = '81282abd4b616262055d1d330df23be3bf4819895c6320aa708b0988d841d946'


def sha(value):
    return hashlib.sha256(value.encode() if isinstance(value,str) else value).hexdigest()


def cases():
    rng = random.Random(FIRST)
    result = []
    x87_values = ('0000000000000080ff3f','00000000000000000080',
                  '35c26821a2da0fc90040','45230100000000c0ff7f')

    def row(mode, fault=None, reopen=False):
        ordinal = len(result)
        banks = {base:bytearray(rng.randbytes(size)) for base,size in REGIONS}
        sp = 0x62000800+(ordinal&3)
        source, output = {
            'null':(0,0), 'separate':(0x61000180,0x61000400), 'left':(0x61000402,0x610003f1),
            'right':(0x61000402,0x610003f9), 'inplace':(0x61000402,0x610003f6),
            'stack-source':(sp-0x60,0x61000400), 'stack-output':(0x61000400,sp-0x58),
            'local-translation':(0x61000180,0x61000400),
        }[mode]
        translation = sp-0x3c if mode=='local-translation' else 0x61000680
        gp = [rng.getrandbits(32) for _ in range(8)]
        gp[0], gp[1], gp[4], gp[6] = source,translation-0x9c,sp,output
        frame = dict(name=f'{mode}/{ordinal}',id=ordinal,entry=FIRST,gp=gp,matrix=rng.getrandbits(32),
            source=rng.getrandbits(32),eflags=0x202|(rng.getrandbits(12)&0xcd5),max_steps=100,
            before_fp=[0x37f|((ordinal&3)<<10),((ordinal&7)<<11)|((ordinal&3)<<8)|((ordinal>>2)&0x3f),
                       0x1f80|((ordinal&3)<<13)|(ordinal&0x3f)],
            live_x87=list(x87_values[:ordinal%5]),xmm=[[rng.getrandbits(32) for _ in range(4)] for _ in range(8)],
            banks=[dict(address=address,before=bank.hex()) for address,bank in banks.items()],
            stop_before_entries=[TRANSFORM,LEAVE],strict_writes=True)
        if fault: frame['fault']=dict(zip(('phase','ordinal'),fault,strict=True))
        if reopen: frame['reopen']=True
        result.append(frame)

    for mode in ('null','separate','left','right','inplace','stack-source','stack-output','local-translation'):
        for trial in range(6): row(mode,reopen=trial==0 and mode in ('separate','local-translation'))
    for mode,reads,writes in (('separate',12,15),('local-translation',15,18)):
        for phase,count in (('before_read',reads),('before_write',writes),('after_write',writes)):
            for at in range(count): row(mode,(phase,at))
    return result


def prepare(scratch):
    canonical = UNIT.read_text()
    ordinary,receipt = specialize(canonical)
    entry = qualified_dispatch_entries(canonical,'tu15').get(binding.OWNER)
    if (not binding.admitted(canonical) or not entry or entry['entry']!=FIRST or
            receipt['native_spatial_owners'].count(binding.OWNER)!=1 or binding.install(canonical,ordinary)!=(ordinary,[])):
        raise ValueError('Missing authenticated production 250AC0 binding')
    owner = binding.world.body(ordinary,binding.OWNER)[0]
    if owner.count(binding.HOOK)!=1 or owner.count('s.instruction_literal(')!=114:
        raise ValueError('Prepared 250AC0 owner or successors drifted')
    (scratch/'particle_transform_basis_owner.inc').write_text(owner)
    accounted,manifest,_ = instrument_translation(ordinary,canonical,UNIT.relative_to(ROOT).as_posix(),ROOT)
    metadata,=[row for row in manifest['functions'] if row['symbol']==binding.OWNER]
    if metadata['literal_sites']!=114:
        raise ValueError('Prepared 250AC0 execution accounting lost original sites')
    return ordinary,accounted,metadata


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
    if sha(image.data)!=IMAGE or sha(image.read(FIRST,TRANSFORM-FIRST))!=MACHINE:
        raise ValueError('Original 250AC0 basis image drifted')
    calls=((0x251fb2,FIRST),(0x250c0c,0x1458a0),(0x250c17,0x1459b0))
    for site,target in calls:
        word=image.read(site,5)
        if word[0]!=0xe8 or site+5+struct.unpack('<i',word[1:])[0]!=target:
            raise ValueError(f'Original 250AC0 call identity drift at {site:x}')
    ordinary,accounted,metadata=prepare(scratch)
    rows=cases()
    physical,evidence=physical_original(image,rows,scratch,entries=(FIRST,),regions=REGIONS,preserve_x87_prefix=True)
    sites=set().union(*(row['executed_sites'] for row in physical['cases']))
    if len(sites)!=51 or not all(row['live_x87_unchanged'] for row in physical['cases']):
        raise ValueError('Incomplete original 250AC0 basis or live x87 observations')
    comparison=scratch/'basis-oracle.json'
    comparison.write_text(json.dumps(dict(input=rows,expected=physical['cases']),separators=(',',':')))
    env={k:v for k,v in os.environ.items() if k not in ('HALO2_REFERENCE_PARTICLE_TRANSFORM_BASIS',
        'HALO2_REFERENCE_PHYSICS_WORLD_LEAVES','HALO2_CENSUS_NATIVE_BRIDGES')}
    command=[args.compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math',
             f'-I{ROOT/"src"}',f'-I{ROOT/"include"}',f'-I{scratch}']
    profiles={}
    for profile,flags in (('release',[]),('sanitized',['-O1','-fsanitize=address,undefined',
            '-fno-sanitize-recover=all','-fno-omit-frame-pointer','-fno-pie','-no-pie'])):
        binary=scratch/('basis-'+profile)
        subprocess.run([*command,*flags,str(TEST),'-o',str(binary)],cwd=scratch,check=True)
        checked=subprocess.run([binary,comparison],cwd=scratch,env=env,capture_output=True,text=True)
        if checked.returncode: raise ValueError(f'{profile} 250AC0 differential failed: {checked.stderr}')
        profiles[profile]=json.loads(checked.stdout)
    args.report.parent.mkdir(parents=True,exist_ok=True)
    args.report.write_text(json.dumps(dict(passed=True,scratch=str(scratch),profiles=profiles,original=evidence,
        machine_sha256=MACHINE,original_sites=sorted(sites),genuine_calls=calls,manifest=metadata,
        compiled_alias_sha256=sha(ordinary),accounted_alias_sha256=sha(accounted),
        source_sha256={str(path.relative_to(ROOT)):sha(path.read_bytes()) for path in (UNIT,HEADER,TEST,Path(binding.__file__))},
        retained_large_artifacts=[]),indent=2)+'\n')
    print(f'Particle 250AC0 basis passed {profiles}')


if __name__=='__main__':
    main()
