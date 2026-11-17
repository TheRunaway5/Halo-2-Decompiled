#!/usr/bin/env python3
"""Unchanged physical scheduler parents executing genuine HS expressions.

The inputs contain syntax trees and real retained HS frames, never evaluated
argument/result substitutions. Native replay uses the actual compiled HS
parser/evaluator and independently owned lifecycle/command builtins.
"""
import argparse,gzip,json,os,struct
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
import oracle_native_campaign_ai_area_perform_tu15 as ref
import oracle_native_campaign_ai_path_search_tu15 as physical
import oracle_native_campaign_hs_command_builtins_tu15 as hs
OWNED=(0x211840,0x262590,0x262710)
ENTRIES=tuple(dict.fromkeys(OWNED+hs.ENTRIES+(0x20fee0,0x2b0bf0)))
NONE=0xffffffff
def inputs(image):
    rows=[]
    for profile in physical.FP_PROFILES[:3]:
        for program in ('flag','begin_sleep','begin_go_to','begin_flag_sleep','begin_flag_go_to'):
            for entry in OWNED:
                if entry==0x262710 and program.endswith('go_to'):continue
                for wake in (0,0xfffffffd):
                    row=hs.make(image,0x2b2470,0,1,True,True,profile=profile)
                    banks={b['address']:bytearray.fromhex(b['before']) for b in row['banks']}
                    extra=0x1060000;banks[extra]=bytearray(0x2000)
                    def put(at,value,size=4):
                        for base,raw in banks.items():
                            if base<=at and at+size<=base+len(raw):raw[at-base:at-base+size]=(value&((1<<(size*8))-1)).to_bytes(size,'little');return
                        banks[at]=bytearray((value&((1<<(size*8))-1)).to_bytes(size,'little'))
                    def desc(op):
                        at=0x480c00+op*4;definition=int.from_bytes(image.read(at,4),'little')
                        banks[at]=bytearray(image.read(at,4));banks[definition]=bytearray(image.read(definition,16))
                    def node(n,op,kind,flags,next=NONE,value=0):
                        at=hs.SY+n*20
                        for off,v,size in ((0,0x9abc,2),(2,op,2),(4,kind,2),(6,flags,2),(8,next,4),(12,0,4),(16,value,4)):put(at+off,v,size)
                    datum=lambda n:0x9abc0000|n
                    if program!='flag':
                        desc(0);node(0,0,4,0,value=datum(1));node(1,2,2,1,next=datum(2))
                        first=2
                        if program.startswith('begin_flag_'):
                            node(2,443,4,0,next=datum(5),value=datum(3));node(3,2,2,1,next=datum(4));node(4,5,5,1,value=0);first=5
                        op=384 if program.endswith('go_to') else 20;desc(op)
                        node(first,op,4,0,value=datum(first+1))
                        node(first+1,2,2,1,next=datum(first+2) if op==384 else NONE)
                        if op==384:node(first+2,28,28,1,value=2)
                    hid=row['gp'][7];thread=hs.HR+0x418;command=hs.CR+0xd4;aid=0xe3610001;cid=0xa7230001
                    actor_pool=extra+0x500;actor_rows=extra+0x600;actor=actor_rows+0x888;clock=extra+0x400
                    put(0x5020c8,actor_pool);put(actor_pool+0x44,actor_rows);put(actor,aid>>16,2);put(actor+0x30,7)
                    put(actor+0x858,cid);put(actor+0x85c,cid);put(command+0x38,hid);put(command+0x30,1,2)
                    put(command+0x44,0,1);put(command+0x7e,0,1);put(thread+8,wake)
                    put(0x51df54,clock);put(clock+8,1000)
                    for address in (0x50ef00,0x50ef04,0x50eee4,0x50eed4,0x50eed8):put(address,NONE)
                    row['entry']=entry;row['gp'][0]=hid
                    if entry==0x211840:row['matrix']=0;row['source']=0
                    elif entry==0x262590:row['matrix']=aid;row['source']=cid
                    else:row['gp'][3]=aid;row['gp'][7]=cid;row['matrix']=extra+0x410;row['source']=0
                    row.update(id=len(rows),name=f'{entry:x}_{program}_wake{wake:x}_fp{profile}',stack_arguments=[],stop_before_entries=[],
                        banks=[dict(address=p,before=bytes(raw).hex()) for p,raw in sorted(banks.items())],max_steps=100000,
                        command_program=program,owned_entry=entry)
                    rows.append(row)
    return rows
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--report',type=Path,required=True);p.add_argument('--select');p.add_argument('--verify',type=Path);a=p.parse_args()
    assert not a.output.exists();assert ref.sha(NEW_DEFAULT)==NEW_PIN
    image=Xbe(NEW_DEFAULT);scratch=Path(os.environ['HALO2_TEST_SCRATCH']);meta=ref.metadata(image,ENTRIES);owned=ref.prepare(image,meta,scratch);rows=inputs(image)
    if a.select:
        import re
        rows=[r for r in rows if re.search(a.select,r['name'])]
        for n,r in enumerate(rows):r['id']=n
    rows=physical.physical(owned,rows,'whole');assert all(r['completed'] and r.get('stopped_at',ref.STOP)==ref.STOP for r in rows)
    reached=set().union(*(set(r['executed_sites']) for r in rows));code=_Code(image,'.text')
    coverage={hex(e):dict(reached=len(set(complete_body(code,e).code)&reached),sites=len(complete_body(code,e).code)) for e in ENTRIES}
    fixture=dict(fixture_kind='native-hs-command-lifecycle-parents-v1',image_sha256=NEW_PIN,backend='physical-unchanged-i386-offline',owners=meta['owners'],instruction_patches=[],supplied_original_outcomes=False,bank_encoding='sparse-runs-v1',cases=ref.compact(rows))
    encoded=gzip.compress(json.dumps(fixture,separators=(',',':')).encode(),mtime=0)
    if a.verify:assert encoded==a.verify.read_bytes()
    a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_bytes(encoded)
    report=dict(passed=not a.select,cases=len(rows),coverage=coverage,stores=sum(len(r['writes']) for r in rows),fixture=str(a.output),fixture_bytes=len(encoded),fixture_sha256=ref.sha(a.output),original_sha256=NEW_PIN,owned_scratch=str(scratch),independent_repeat_identical=bool(a.verify))
    a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))
if __name__=='__main__':main()
