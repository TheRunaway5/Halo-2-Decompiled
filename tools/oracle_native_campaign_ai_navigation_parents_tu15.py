#!/usr/bin/env python3
"""Whole-call 2186F0/E2300/275CB0 composition over real lookup input banks.

Every callee executes original unchanged i386 code; no stop/return is supplied.
The standalone lookup cohort owns read/store fault prefixes, while this cohort
retains every caller-frame byte and all ordered operations across composition.
"""
import argparse,copy,gzip,json,os,struct
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
import oracle_native_campaign_ai_navigation_lookup_tu15 as lookup
import oracle_native_campaign_ai_area_perform_tu15 as reference

OWNED=(0x2186f0,0xe2300,0x275cb0)
ENTRIES=tuple(dict.fromkeys(OWNED+lookup.ENTRIES+(0xe04b0,0xe22c0,0xbcdf0,0xbbe00)))
BIPED=lookup.BASE+0x4800
OUTPUT=lookup.BASE+0x4c00
FRAME_TABLE=lookup.BASE+0x90000
NONE=0xffffffff

def inputs(image):
    rows=[]
    for seed in lookup.inputs(image):
        if seed['entry']==0x218960:continue
        parent_entries=(0x2186f0,) if seed['entry']==0x2185e0 else (0xe2300,0x275cb0)
        if seed['entry']==0x202660 and seed['matrix']==NONE:continue
        # E04B0 checks the cached surface before E2300 calls the sector
        # lookup; a null BSP is not an admitted cached-surface parent input.
        if seed['entry']==0x202660 and 'static_null_bsp' in seed['name']:continue
        for entry in parent_entries:
            row=copy.deepcopy(seed);banks={b['address']:bytearray.fromhex(b['before']) for b in row['banks']}
            def put(address,value,width=4):
                for base,data in banks.items():
                    if base<=address and address+width<=base+len(data):
                        data[address-base:address-base+width]=(value&((1<<(width*8))-1)).to_bytes(width,'little');return
                banks[address]=bytearray((value&((1<<(width*8))-1)).to_bytes(width,'little'))
            def word(address):
                for base,data in banks.items():
                    if base<=address and address+4<=base+len(data):return int.from_bytes(data[address-base:address-base+4],'little')
                raise ValueError(hex(address))
            # Real object-node matrix and a frame-table row, including signed
            # frame indices. They are ordinary inputs, never original results.
            put(0x505e78,FRAME_TABLE);put(0x46a0d0,0x3f800000);put(lookup.BODY+0x114,52,2)
            put(lookup.BODY+0x116,0x180,2)
            for off,value in ((0,0x3f800000),(4,0x3f800000),(8,0),(12,0),(16,0),(20,0x3f800000),
                              (24,0),(28,0),(32,0),(36,0x3f800000),(40,0x3f800000),(44,0x40000000),(48,0x40400000)):
                put(lookup.BODY+0x180+off,value)
            for index in (100,101,102,-32768):
                at=FRAME_TABLE+index*16;banks[at]=bytearray(16);put(at,0x81370000);put(at+6,0,2);put(at+10,2,1);put(at+11,1,1)
            banks.pop(reference.SP-0x100)
            stack=bytearray(struct.pack('<I',0xdef0cb12)*0xd0)
            gp=row['gp'];gp[4]=reference.SP
            if entry==0x2186f0:
                encoded=gp[0];gp[0]=OUTPUT;gp[6]=lookup.POINT;args=[encoded,0]
            else:
                put(lookup.POOL+0x38,2);put(lookup.POOL+0x24,12)
                put(lookup.ROWS,0x8137,2);put(lookup.ROWS+3,6,1)
                put(lookup.ROWS+12,0x8137,2);put(lookup.ROWS+15,0,1);put(lookup.ROWS+20,BIPED)
                put(BIPED+0x14,NONE);put(BIPED+0xaa,0,1);put(BIPED+0x3dc,0,1)
                put(BIPED+0x350,8);put(BIPED+0x354,seed['matrix']);put(BIPED+0x358,NONE)
                put(BIPED+0x35c,gp[0]);put(BIPED+0x360,gp[1]);put(BIPED+0x364,seed['source'])
                for n in range(3):put(BIPED+0x368+n*4,word(lookup.POINT+n*4))
                put(0x51df54,lookup.SCENARIO);put(lookup.SCENARIO+8,8)
                if entry==0xe2300:gp[0]=0x81370001;args=[OUTPUT,OUTPUT+4,OUTPUT+8,OUTPUT+20,OUTPUT+24]
                else:gp[0]=0xa8123456;gp[1]=0x81370001;args=[OUTPUT,OUTPUT+8]
            struct.pack_into('<'+'I'*(len(args)+1),stack,0x300,reference.STOP,*args)
            banks[reference.SP-0x300]=stack
            row.update(id=len(rows),name=f'{entry:x}_'+seed['name'],entry=entry,matrix=args[0],source=args[1],
                stack_arguments=args[2:],max_steps=10000,implicit_stack_addresses=[],object_navigation_exact=True,
                banks=[dict(address=a,before=b.hex()) for a,b in sorted(banks.items())])
            rows.append(row)
    for seed in list(rows):
        if seed['entry']!=0x2186f0 or '_frame_kind6_' not in seed['name']:continue
        for variant in ('world_space','missing_frame_object','negative_frame_node','zero_frame_scale','aliased_point'):
            row=copy.deepcopy(seed);row.update(id=len(rows),name=seed['name']+'_'+variant)
            if variant=='world_space':row['gp'][1]=NONE
            elif variant=='aliased_point':row['gp'][0]=lookup.POINT
            else:
                address,value,width={
                    'missing_frame_object':(FRAME_TABLE+102*16,NONE,4),
                    'negative_frame_node':(FRAME_TABLE+102*16+6,65535,2),
                    'zero_frame_scale':(lookup.BODY+0x180,0,4)}[variant]
                for bank in row['banks']:
                    data=bytearray.fromhex(bank['before']);base=bank['address']
                    if base<=address and address+width<=base+len(data):
                        data[address-base:address-base+width]=value.to_bytes(width,'little');bank['before']=data.hex();break
                else:raise ValueError(hex(address))
            rows.append(row)
    return rows

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--report',type=Path,required=True);p.add_argument('--verify',type=Path);a=p.parse_args()
    assert not a.output.exists();assert reference.sha(NEW_DEFAULT)==NEW_PIN
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);image=Xbe(NEW_DEFAULT);meta=reference.metadata(image,ENTRIES);owned=reference.prepare(image,meta,scratch)
    rows=reference.physical(owned,inputs(image),'complete');assert all(r['completed'] and r['stopped_at']==reference.STOP for r in rows)
    reached=set().union(*(set(r['executed_sites']) for r in rows));code=_Code(image,'.text')
    bodies={e:set(complete_body(code,e).code) for e in ENTRIES}
    fixture=dict(image_sha256=NEW_PIN,backend='physical-unchanged-i386-offline',owners=meta['owners'],instruction_patches=[],supplied_original_outcomes=False,
        stop_entries=[],bank_encoding='sparse-runs-v1',whole_call_composition=True,cases=reference.compact(rows))
    data=gzip.compress(json.dumps(fixture,separators=(',',':')).encode(),mtime=0)
    if a.verify:assert data==a.verify.read_bytes(),'Independent navigation parent reference differs'
    a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_bytes(data)
    report=dict(passed=True,cases=len(rows),original_instructions=sum(r['original_instructions'] for r in rows),fixture_bytes=len(data),fixture_sha256=reference.sha(a.output),
        coverage={hex(e):dict(reached=len(bodies[e]&reached),total=len(bodies[e])) for e in ENTRIES},independent_repeat_identical=True if a.verify else None,
        scratch=str(scratch),retained_large_artifacts=[])
    a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))
if __name__=='__main__':main()
