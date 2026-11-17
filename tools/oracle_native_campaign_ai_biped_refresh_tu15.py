#!/usr/bin/env python3
"""Physical unchanged E20F0/E1DD0/1FD8A0 reference over retained DATA.

Caller locals and collision scratch are internal intermediates. This cohort
compares retained banks, ordered publications, AL, saved registers and FP;
it does not claim private-stack preimage or volatile-register equivalence.
"""
import argparse
import copy
import gzip
import json
import os
from pathlib import Path
import struct

import oracle_native_campaign_ai_area_perform_tu15 as reference
import validate_native_camera_geometric_query_tu15 as geometry
import validate_native_camera_instanced_query_tu15 as instance
import validate_native_camera_dynamic_query_tu15 as dynamic
import oracle_native_campaign_ai_navigation_lookup_tu15 as lookup
from oracle_smg_firing_1_1 import Xbe, _Code
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from generate_physics_live_support_tu15 import complete_body

OWNED=(0xe20f0,0xe1dd0,0x1fd8a0)
ENTRIES=tuple(dict.fromkeys((*OWNED,*geometry.OWNERS,*instance.OWNERS,*dynamic.OWNERS,*lookup.ENTRIES,0xe2300,0x275cb0,0x2186f0,0xe04b0,0x111d10,0x218b20,0x30bd0,
    0xbbe00,0xbbd10,0x16c290,0x16c530,0x24dc90,0xba970,0x16e360)))
BASE=0x1010000
POOL,ROWS,BODY,ACTORS,ACTOR,OUT,ORIGIN,DIRECTION=tuple(BASE+n for n in (0,0x100,0x400,0x1000,0x1100,0x2000,0x2100,0x2120))
NONE=0xffffffff

def cases(image):
    result=[];seed=geometry.cases(image)[0]
    variants=[dict(name='default'),dict(name='fall',state=5),dict(name='contact',contact=True),
        dict(name='actor_other',actor=True,kind=1),dict(name='actor_inactive',actor=True),
        dict(name='actor_fall',actor=True,state=5),dict(name='actor_action5',actor=True,action=5),
        dict(name='actor_mode',actor=True,mode=0x50000cb),dict(name='actor_action6',actor=True,action=6,state=4),
        dict(name='actor_action7',actor=True,action=7,state=4),dict(name='actor_action6_other',actor=True,action=6),
        dict(name='actor_target',actor=True,prediction=True),dict(name='actor_target_zero',actor=True,prediction=True,zero=True),
        dict(name='actor_target_expired',actor=True,prediction=True,expired=True)]
    for v in variants:
      for rounding in range(4):
        raw=bytearray(0x2400)
        def put(p,value,width=4):raw[p-BASE:p-BASE+width]=(value&((1<<(8*width))-1)).to_bytes(width,'little')
        put(POOL+0x44,ROWS);put(ROWS+8,BODY);put(ROWS,0x8137,2)
        for off,value,width in ((0x14,NONE,4),(0x12c,0x81230000 if v.get('actor') else NONE,4),
                (0xaa,v.get('kind',0),1),(0x3dc,v.get('state',0),1),(0x348,0x1000 if v.get('contact') else 0,2),
                (0x354,NONE,4),(0x35c,NONE,4),(0x360,NONE,4),(0x346,0x500,2),(0x536,v.get('action',0),2),(0x12a,0x280,2),
                (0x280,0,4),(0x286,0,2),(0x2e8,0,4),(0x2f0,v.get('mode',NONE),4),(0x2fc,NONE,4)):
            put(BODY+off,value,width)
        for off in (0x368,0x380,0x3ec,0x64):
            for n,value in enumerate((0xbf800000,0,0)):put(BODY+off+n*4,value)
        for off in (0x70,0x374):
            for n,value in enumerate((0xbf800000,0,0)):put(BODY+off+n*4,value)
        put(ACTORS+0x44,ACTOR);put(ACTOR+0x18,0x81370000);put(ACTOR+0x264,v.get('prediction',False),1)
        put(ACTOR+0x634,0 if v.get('expired') else 9)
        for off in (0x238,0x638):
            for n,value in enumerate((0 if off==0x238 or v.get('zero') else 0x3f800000,0,0)):put(ACTOR+off+n*4,value)
        put(ACTOR+0x644,0xffff,2)
        raw[OUT-BASE:OUT-BASE+0x80]=bytes(range(0x80))
        for n,value in enumerate((0xbf800000,0,0)):put(ORIGIN+n*4,value)
        for n,value in enumerate((0x3f800000,0,0)):put(DIRECTION+n*4,value)
        banks={b['address']:bytes.fromhex(b['before']) for b in seed['banks'] if not reference.STACK<=b['address']<reference.STACK+reference.STACK_SIZE}
        banks[BASE]=bytes(raw)
        # Coherent empty instance and spatial-object populations. The original
        # still traverses the declared BSP and computes its own hit/miss.
        data=bytearray(banks[0x1000000]);struct.pack_into('<I',data,0x41a8,0);struct.pack_into('<I',data,0x4900,NONE);banks[0x1000000]=bytes(data)
        for p,value in ((0x4ecbd0,POOL),(0x5020c8,ACTORS),(0x51df54,BASE+0x2200),(0x4eabb0,0x1004900)):
            banks[p]=struct.pack('<I',value)
        for p,size in ((0x4f3ce0,12),(0x4f44e8,8),(0x4eabc8,8)):
            banks[p]=bytes(size)
        for p in (0x46a0dc,0x46a12c,0x46a240,0x46a0ec):banks[p]=image.read(p,4)
        for p in (0x474d10,0x474d1c):
            pointer=struct.unpack('<I',image.read(p,4))[0];banks[p]=image.read(p,4);banks[pointer]=image.read(pointer,12)
        for entry in (0xe20f0,0xe1dd0,0x1fd8a0):
            if entry==0x1fd8a0 and not v.get('actor'):continue
            n=len(result);gp=[0xa1520000+n*31+k*7 for k in range(8)];gp[4]=reference.SP;gp[7]=0x81370000
            if entry==0xe20f0:args=[NONE,NONE,OUT,OUT+16,OUT+20,OUT+24,OUT+28]
            elif entry==0xe1dd0:
                gp[0]=ORIGIN;gp[1]=DIRECTION;gp[3]=NONE;args=[0x81370000,NONE,0x40000000,OUT,OUT+32,OUT+16,OUT+20,OUT+24,OUT+28]
            else:gp[0]=0x81230000;gp[1]=OUT;gp[3]=OUT+32;args=[0,0]
            result.append(dict(id=n,name=f'{entry:x}_{v["name"]}_rc{rounding}',entry=entry,gp=gp,matrix=args[0],source=args[1],stack_arguments=args[2:],
                before_fp=[0x27f|(rounding<<10),0,0x1f80|(rounding<<13)],live_x87=[],max_steps=100000,
                banks=[dict(address=p,before=data.hex()) for p,data in sorted(banks.items())]))
    base=next(r for r in result if r['name']=='e1dd0_default_rc0')
    for variant in ('null_origin','null_outputs','deleting_cached_object','cached_instance','cached_instance_null_outputs','dynamic_sector','cached_dynamic_object'):
      for rounding in range(4):
        row=copy.deepcopy(base);row['name']='e1dd0_'+variant+'_rc'+str(rounding)
        row['before_fp']=[0x27f|(rounding<<10),0,0x1f80|(rounding<<13)]
        banks={b['address']:bytearray.fromhex(b['before']) for b in row['banks']}
        banks[0x4ecc0c]=bytearray(struct.pack('<I',geometry.bsp.D))
        if variant=='null_origin':row['gp'][0]=0
        if variant=='deleting_cached_object':
            row['source']=0x81370000;banks[BASE][ROWS-BASE+2]=0x10
        if variant=='dynamic_sector':
            struct.pack_into('<I',banks[BASE],BODY-BASE+0x354,0)
            struct.pack_into('<I',banks[0x1000000],geometry.bsp.D-0x1000000+0x2c,0x1004a00)
            banks[0x1000000][0x4a00:0x4a08]=bytes(8)
            banks[0x1000000][0x4a04]=1
        if variant.startswith('cached_instance'):
            seed=next(c for c in instance.cases(image) if c['entry']==0x16c000 and '_flags1_scale3f800000' in c['name'])
            geometry_data=banks[0x1000000]
            for b in seed['banks']:
                if not reference.STACK<=b['address']<reference.STACK+reference.STACK_SIZE:
                    banks[b['address']]=bytearray.fromhex(b['before'])
            # Both queries share a genuine scenario descriptor. Preserve the
            # static BSP/empty scene fields for a cached-instance miss.
            data=banks[0x1000000]
            for offset,width in ((0x10,4),(0x30,4),(0xa0,4)):
                data[0x6000+offset:0x6000+offset+width]=geometry_data[0x4600+offset:0x4600+offset+width]
            struct.pack_into('<I',data,0x41a8,0);struct.pack_into('<I',data,0x4900,NONE)
            row['gp'][3]=0
        if variant=='cached_dynamic_object':
            # A real collision model with two BSP regions supplies the cached
            # object hit. Move this row's biped DATA beyond that model bank.
            own=banks[BASE];shift=0x20000
            for off in range(0,len(own)-3,4):
                value=struct.unpack_from('<I',own,off)[0]
                if BASE<=value<BASE+len(own):struct.pack_into('<I',own,off,value+shift)
            seed=dynamic.cases(image)[0]
            constants={p:data for p,data in banks.items() if p<0x1000000}
            banks={b['address']:bytearray.fromhex(b['before']) for b in seed['banks'] if not reference.STACK<=b['address']<reference.STACK+reference.STACK_SIZE}
            for p,data in constants.items():banks.setdefault(p,data)
            banks[BASE+shift]=own;banks[0x4ecbd0]=bytearray(struct.pack('<I',POOL+shift))
            struct.pack_into('<I',own,ROWS-BASE+20,0x1007000)
            struct.pack_into('<I',banks[0x1000000],0x7014,NONE)
            row['source']=0xabcd0001
            row['gp']=[v+shift if BASE<=v<BASE+len(own) else v for v in row['gp']]
            row['stack_arguments']=[v+shift if BASE<=v<BASE+len(own) else v for v in row['stack_arguments']]
            banks[0x474c24]=bytearray(b'\xff\xff')
        if variant.endswith('null_outputs'):row['stack_arguments'][1:]=[0]*6
        row['banks']=[dict(address=p,before=data.hex()) for p,data in sorted(banks.items())]
        row['id']=len(result);result.append(row)
    for seed in [r for r in result if r['entry']==0xe20f0]:
      for entry in (0xe2300,0x275cb0):
        if entry==0x275cb0 and 'actor_other' in seed['name']:continue
        row=copy.deepcopy(seed);row['entry']=entry;row['name']=hex(entry)[2:]+'_composed_'+seed['name']
        banks={b['address']:bytearray.fromhex(b['before']) for b in row['banks']}
        own=banks[BASE]
        for offset in (0x354,0x358,0x35c,0x360,0x364):struct.pack_into('<I',own,BODY-BASE+offset,NONE)
        struct.pack_into('<I',own,BODY-BASE+0x350,0);struct.pack_into('<I',own,0x2208,1)
        # Coherent absent navigation block after the real collision result;
        # 202660 and 2186F0 execute their own source empty/world-space paths.
        scenario=struct.unpack('<I',banks[0x4ecc18])[0]
        struct.pack_into('<I',banks[0x1000000],scenario-0x1000000+0xc4,0)
        row['gp'][0 if entry==0xe2300 else 1]=0x81370000
        args=[OUT+16,OUT+20,OUT,OUT+24,OUT+28] if entry==0xe2300 else [OUT+16,OUT]
        row.update(matrix=args[0],source=args[1],stack_arguments=args[2:],id=len(result),banks=[dict(address=p,before=data.hex())for p,data in sorted(banks.items())])
        result.append(row)
    return result

def main():
    p=argparse.ArgumentParser();p.add_argument('--output',type=Path,required=True);p.add_argument('--report',type=Path,required=True);p.add_argument('--verify',type=Path)
    a=p.parse_args();assert reference.sha(NEW_DEFAULT)==NEW_PIN
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);image=Xbe(NEW_DEFAULT);meta=reference.metadata(image,ENTRIES)
    original=reference.prepare(image,meta,scratch);rows=reference.physical(original,cases(image),'complete');assert all(r['completed'] for r in rows)
    # The original checkpoints are post-publication, selected only from the
    # new owners' retained-output stores, never from supplied child outcomes.
    prefixes=[];seen=set()
    for row in rows:
        for ordinal,event in enumerate(row['writes']):
            if event['instruction'] in seen or not any(lo<=event['instruction']<hi for lo,hi in ((0xe20f0,0xe24f0),(0xe1dd0,0xe1fb0),(0x1fd8a0,0x1fda60),(0x275cb0,0x275e40))):continue
            seen.add(event['instruction']);r=copy.deepcopy(row);r['id']=len(rows)+len(prefixes);r['name']+='_after_'+hex(event['instruction']);r['fault']=dict(phase='after_write',ordinal=ordinal)
            for b in r['banks']:b.pop('after',None)
            prefixes.append(r)
    rows+=reference.physical(original,prefixes,'prefix')
    reached=set().union(*(set(r['executed_sites']) for r in rows));code=_Code(image,'.text')
    fixture=dict(image_sha256=NEW_PIN,backend='physical-unchanged-i386-biped-refresh-retained-v1',instruction_patches=[],supplied_original_outcomes=False,
        owners=meta['owners'],bank_encoding='sparse-runs-v1',cases=reference.compact(rows))
    data=gzip.compress(json.dumps(fixture,separators=(',',':')).encode(),mtime=0)
    if a.verify:assert data==a.verify.read_bytes()
    a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_bytes(data)
    report=dict(passed=True,cases=len(rows),prefixes=len(prefixes),coverage={hex(e):dict(reached=len(set(complete_body(code,e).code)&reached),total=len(complete_body(code,e).code)) for e in OWNED},
        fixture_sha256=reference.sha(a.output),fixture_bytes=len(data),scratch=str(scratch),retained_large_artifacts=[],scope='Retained DATA; semantic collision scratch; no arbitrary private stack or volatile GP equivalence')
    a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))
if __name__=='__main__':main()
