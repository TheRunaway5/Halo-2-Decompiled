#!/usr/bin/env python3
"""Observe genuine 1770C0 calls and returns through the indexed datum gate."""
import argparse
import hashlib
import json
import os
from pathlib import Path

import oracle_native_campaign_screen_lifecycle_tu15 as original
from oracle_smg_firing_1_1 import Xbe
from validate_native_indexed_datum_gate_frame_tu15 import BANKS, corpus as indexed_inputs
from validate_native_projectile_function_query_frame_tu15 import boundary_packet
from validate_native_tu15_prt_helpers import PIN, physical_original

PARENT,SCAN,REFERENCE,GATE=0x1770c0,0x16ea60,0x178070,0x3eb10
CALLS=((0x1770cf,SCAN,0x1770d4),(0x177118,REFERENCE,0x17711d),
       (0x177152,GATE,0x177157),(0x17727f,SCAN,0x177284))
REGIONS=tuple(sorted((*BANKS,(0x4f0000,4096),(0x51d000,4096),(0x557000,4096))))


def sha(path):
    with path.open('rb') as file:
        return hashlib.file_digest(file,'sha256').hexdigest()


def corpus(image):
    rows=indexed_inputs(image)[:64]
    for n,row in enumerate(rows):
        banks={item['address']:bytearray.fromhex(item['before']) for item in row['banks']}
        for base,size in REGIONS:
            banks.setdefault(base,bytearray(size))
        seeds=[]

        def put(address,value,width=4):
            for base,data in banks.items():
                if base<=address<=base+len(data)-width:
                    raw=(value&((1<<(8*width))-1)).to_bytes(width,'little')
                    pos=address-base;data[pos:pos+width]=raw
                    seeds.append(dict(address=address,bytes=raw.hex()))
                    return
            raise ValueError('ParticleDraw original fixture escaped declared banks')

        particles,bitmap,particle=0x61000100,0x61000180,0x61000400
        tags,definition,instances=0x61000900,0x61000b00,0x61000d00
        selected,header=0x61002600,0x61001400
        variant=n//8;scenario=n%8;index=(0,1,4,7)[variant%4]
        datum=(0,1,5,17,31,32,63,95)[variant%8]
        lookup=(0,1,5,-3,0x80000000,0x80000005,32,63)[variant%8]
        put(0x51df74,particles);put(particles+0x24,0x54)
        put(particles+0x38,index+1);put(particles+0x44,particle-index*0x54)
        put(particles+0x48,bitmap);put(bitmap,1<<index)
        put(particle,(0x8001+variant)&0xffff,2)
        put(particle+0xe,0,2);put(particle+0x10,0)
        put(particle+0x20,datum,2);put(particle+0x4c,0)
        put(0x4f0414,tags);put(tags,0x62736474);put(tags+8,definition)
        put(definition+0x18,instances);put(instances+0x16,0,2)
        put(0x557034+(datum//32)*4,0)
        put(0x4c6709,0 if scenario==0 else 1,1)
        put(0x4c670c,0xffffffff if scenario==1 else lookup)
        put(0x474c24,0xffff if scenario==2 else 0,2)
        put(0x4ecc18,header);put(header+0xac,0 if scenario==3 else -3 if scenario==4 else 4)
        put(header+0xb0,selected);put((selected+lookup*2)&0xffffffff,datum+(scenario==5),2)
        row.update(entry=PARENT,name=f'particle-draw-indexed-gate-{n}',max_steps=512,
                   matrix=0,source=0,seeds=seeds,wants_gate_result=int(scenario in (6,7)),
                   banks=[dict(address=base,before=banks[base].hex()) for base,_ in REGIONS])
    return rows


def observe(image,rows,scratch):
    code=original._Code(image,'.text');initial=original.metadata
    for pc,target,resume in CALLS:
        raw=image.read(pc,5)
        if raw[0]!=0xe8 or pc+5!=resume or resume+int.from_bytes(raw[1:],'little',signed=True)!=target:
            raise ValueError('Actual ParticleDraw direct call changed')
    def metadata(image):
        result=initial(image)
        for row in result['sites']:
            instruction=code.at(row['instruction'])
            if instruction.mnemonic in ('pop','ret'):
                row['operands'].append(dict(base=4,index=-1,scale=1,displacement=0,bytes=4,read=True,write=False))
            if instruction.mnemonic=='test':
                for operand in row['operands']:
                    if operand['write']:raise ValueError('Unexpected original TEST memory output')
                    operand['read']=True
        return result
    entries=(PARENT,SCAN,REFERENCE,GATE)
    original.metadata=metadata;original.ENTRIES=entries
    pcs=sorted({0x177159,0x17715f,0x177263,0x1772af,*(p for edge in CALLS for p in edge)})
    traces=[dict(entry=pc,arguments=0,gp=True,xmm=True) for pc in pcs]
    try:
        return physical_original(image,rows,scratch,entries=entries,regions=REGIONS,trace_calls=traces,
                                 trace_extended=True,preserve_x87_prefix=True)
    finally:
        original.metadata=initial


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--xbe',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):
        parser.error('Use tools/run_test.py')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH'])
    args.output.mkdir(parents=True,exist_ok=False)
    result=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[],production_bound=False)
    try:
        if sha(args.xbe)!=PIN:raise ValueError('Incorrect TU1.5 authority')
        image=Xbe(args.xbe);rows=corpus(image)
        outputs,result['authority']=observe(image,rows,scratch)
        covered=set();roots=set();joins={};outcomes={};edges={hex(pc):0 for pc,_,_ in CALLS}
        writes=0
        for row,case in zip(rows,outputs['cases'],strict=True):
            history=case['executed_sites'];covered.update(history);roots.update(pc for pc in history if PARENT<=pc<0x1772c0)
            traced=[p['entry'] for p in case['calls']]
            if not case['completed'] or not traced or traced[-1]!=0x1772af:
                raise ValueError('ParticleDraw did not reach the genuine RET4')
            if traced.count(SCAN)!=2 or traced.count(REFERENCE)!=1 or traced.count(GATE)!=1:
                raise ValueError('ParticleDraw did not execute all genuine child starts')
            for pc,_,resume in CALLS:
                if traced.count(pc)!=1 or traced.count(resume)!=1:
                    raise ValueError('ParticleDraw did not execute each original child return')
                edges[hex(pc)]+=1
            packet=[p for p in case['calls'] if p['entry']==0x177157]
            if len(packet)!=1 or packet[0]['gp'][0]&0xff!=row['wants_gate_result']:
                raise ValueError('Original indexed gate did not publish the intended result at 177157')
            branch=bool(0x17715f in history)
            if branch!=bool(row['wants_gate_result']):
                raise ValueError('Original 1770C0 did not consume the indexed gate result')
            joins[hex(0x17715f if branch else 0x177263)]=joins.get(hex(0x17715f if branch else 0x177263),0)+1
            answer=str(row['wants_gate_result']);outcomes[answer]=outcomes.get(answer,0)+1
            writes+=len(case['writes'])
            row['expected']=boundary_packet(case,row,traced[-1]);row['writes']=case['writes']
            row['expected_reads']=case['reads']
        fixture=scratch/'particle-draw-indexed-gate-fixture.json'
        fixture.write_text(json.dumps(rows,separators=(',',':')))
        result.update(passed=True,original_returns=len(rows),original_sites=len(covered),parent_sites=len(roots),
                      actual_direct_returns=edges,gate_al=outcomes,branches=joins,ordered_writes=writes,
                      fixture_sha256=sha(fixture))
    except Exception as error:
        result['error']=repr(error)
        raise
    finally:
        (args.output/'report.json').write_text(json.dumps(result,indent=2,sort_keys=True)+'\n')
    return 0


if __name__=='__main__':
    raise SystemExit(main())
