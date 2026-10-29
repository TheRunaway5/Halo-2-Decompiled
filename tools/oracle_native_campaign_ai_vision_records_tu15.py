#!/usr/bin/env python3
"""Physical unchanged original vision record helpers; no supplied outcomes."""
import argparse,copy,gzip,json,os,struct,subprocess
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
import oracle_native_campaign_screen_lifecycle_tu15 as common
import oracle_native_campaign_ai_perception_policy_tu15 as policy
import oracle_native_campaign_ai_perception_group_link_tu15 as transport
import validate_native_geometry_tu15 as geometry
BASE=policy.BASE;BODY=BASE+0xc000;ATTACH=BODY+0x300
TAGS=BASE+0xf000;CHARACTERS=BASE+0x1000;PLAYERS=BASE+0xf500;CLUMPPOOL=BASE+0xf600;CLUMPS=BASE+0xf700;CLOCK=BASE+0xf680
ENTRIES=(0x26eef0,0x1eccc0,0x1d0400,0x272970)
sha=common.sha
def prepare(image,meta,scratch):
    owned=transport.prepare(image,meta,scratch)
    result=subprocess.run(['c++','-std=c++20','-O2',str(ROOT/'tools/observe_native_campaign_ai_vision_records_tu15.cpp'),'-o',str(owned/'observer')],capture_output=True,text=True);assert not result.returncode,result.stderr
    return owned
def inputs():
    rows=[];seed=next(s for s in policy.inputs()if s['name'].startswith('fault_seed'))
    def make(entry=0x26eef0,name='record',flags=4,count=24,offset=0x300,packed=(0,7,0x1234),output=BASE+0xff00,private=False,chain=(0,-1,1),character=0xe24e0000,players=(0x80000001,0x80000002),player=0x80000002,clump=0xe8000000,tick=0x12345678,changes=(),fp=(0x27f,0,0x1f80),live=0):
        n=len(rows);banks={b['address']:bytearray.fromhex(b['before'])for b in seed['banks']}
        def put(p,v,w=4):
            for first,data in banks.items():
                if first<=p and p+w<=first+len(data):data[p-first:p-first+w]=(v&((1<<(8*w))-1)).to_bytes(w,'little');return
            banks[p]=bytearray((v&((1<<(8*w))-1)).to_bytes(w,'little'))
        put(0x4f0414,TAGS)
        for i in range(3):put(TAGS+i*16+8,CHARACTERS+i*0x100);put(CHARACTERS+i*0x100+8,0xffffffff);put(CHARACTERS+i*0x100+0x3c,0)
        for i,c in enumerate(chain):
            put(TAGS+i*16+8,CHARACTERS+i*0x100);put(CHARACTERS+i*0x100+8,0xe24e0000+i+1 if i+1<len(chain)else 0xffffffff);put(CHARACTERS+i*0x100+0x3c,c);put(CHARACTERS+i*0x100+0x40,0x87654321+i)
        put(BODY+0x10a,flags,1);put(BODY+0x120,count,2);put(BODY+0x122,offset,2)
        for i,p in enumerate(packed):put(BODY+offset+i*8+4,p,2)
        put(0x5020a4,PLAYERS)
        for i,p in enumerate(players):put(PLAYERS+i*0x1c,p)
        put(0x50eef8,CLUMPPOOL);put(CLUMPPOOL+0x44,CLUMPS);put(0x51df54,CLOCK);put(CLOCK+8,tick)
        for p,v,w in changes:put(p,v,w)
        gp=seed['gp'].copy();gp[4]=geometry.SP+n%4*4;dest=geometry.SP+0x500 if private else output
        if entry==0x26eef0:gp[0]=0xe5000000
        elif entry==0x1eccc0:gp[1]=character
        elif entry==0x1d0400:gp[2]=player
        elif entry==0x272970:gp[0]=clump
        rows.append(dict(id=n,name=f'{name}_{n}',entry=entry,gp=gp,matrix=dest if entry==0x26eef0 else 0,source=0,object=0xe5000000,character=character,player=player,clump=clump,output=dest,private_output=private,child_output=0,banks=[dict(address=p,before=data.hex())for p,data in sorted(banks.items())],before_fp=list(fp),live_x87=[struct.pack('<QH',0x8000000000000000+(k<<58),0x3fff+k%3).hex()for k in range(live)]))
    for flags in range(256):make(flags=flags,name='all_attachment_flag_bytes')
    for count in (-32768,-8,-1,0,1,7,8,16,24,32760):
        for output in (0,BASE+0xff00):make(count=count,packed=(0x1234,),output=output,name='actual_count_rule')
    for packed in ((),(0,),(7,),(8,),(0xffff,),(0,7,0x1234),(0x1118,0x2220)):
        for offset in (-0x100,0,1,0x300):
            for private in (False,True):make(count=len(packed)*8,packed=packed,offset=offset,private=private,name='signed_offset_first_match')
    for output in (ATTACH+4,ATTACH+5,BODY+0x120,0x4ecbd0,BASE+0xff01):make(output=output,name='retained_output_alias')
    for chain in ((0,),(1,),(-1,0),(0,0,0),(0,-1,1),(-2147483648,2147483647),(-1,1,1)):
        for character in (0xffffffff,0xe24e0000,0x12340001):make(entry=0x1eccc0,character=character,chain=chain,name='inherited_first_positive')
    for players in ((0x80000001,0x80000002),(0xffffffff,0x80000002),(0x80000001,0x80000001)):
        for player in (0xffffffff,0x80000001,0x80000002,0x80000003):make(entry=0x1d0400,players=players,player=player,name='two_player_slot')
    for clump in (0xe8000000,0x12340001,0xffffffff):
        for tick in (0,0xffffffff,0x80000000):
            # NONE is an unchecked low-index datum in the source, so map its
            # actual output rather than inventing a validation branch.
            at=CLUMPS+0x38
            make(entry=0x272970,clump=clump,tick=tick,changes=[(CLUMPPOOL+0x44,(CLUMPS-(clump&0xffff)*80)&0xffffffff,4),(at,0x76543210,4)],name='clump_tick_datum')
    for alias in (CLOCK+8,CLUMPPOOL+0x44,0x51df54):make(entry=0x272970,changes=[(CLUMPPOOL+0x44,(alias-0x38)&0xffffffff,4)],name='captured_clock_output_alias')
    for entry in ENTRIES:
        for rounding in range(4):make(entry=entry,name='unchanged_fp_state',fp=(0x37f|(rounding<<10),(rounding<<11)|0x21,0x1f80|(rounding<<13)|0x8040),live=rounding*2)
    for entry in ENTRIES:make(entry=entry,name='fault_seed_'+hex(entry))
    make(name='fault_seed_private',private=True);make(name='fault_seed_output_alias',output=ATTACH+4)
    return rows
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--report',type=Path,required=True);p.add_argument('--verify',action='store_true');a=p.parse_args();assert a.verify or not a.output.exists();assert sha(NEW_DEFAULT)==NEW_PIN
    image=Xbe(NEW_DEFAULT);common.ENTRIES=ENTRIES;meta=common.metadata(image);owned=prepare(image,meta,Path(os.environ['HALO2_TEST_SCRATCH']));rows=geometry.physical(owned,inputs(),'complete');prefix=[]
    for seed in(r for r in rows if r['name'].startswith('fault_seed')):
        for phase,events in (('before_read',seed['reads']),('before_write',seed['writes']),('after_write',seed['writes'])):
            for n in range(len(events)):
                row=copy.deepcopy(seed);row.update(id=len(rows)+len(prefix),name=seed['name']+'_'+phase+'_'+str(n),fault=dict(phase=phase,ordinal=n));prefix.append(row)
    rows+=geometry.physical(owned,prefix,'prefix');visited=set().union(*(set(r['executed_sites'])for r in rows));fixture=transport.intern_banks(dict(image_sha256=NEW_PIN,backend='physical-unchanged-i386-offline',owners=meta['owners'],instruction_patches=[],supplied_original_outcomes=False,cases=rows));encoded=gzip.compress(json.dumps(fixture,separators=(',',':')).encode(),mtime=0)
    if a.verify:assert a.output.read_bytes()==encoded,'Vision records independent physical repeat differs'
    else:a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_bytes(encoded)
    report=dict(passed=True,owners=meta['owners'],cases=len(rows),complete_calls=sum(r['completed']for r in rows),fault_prefixes=len(prefix),original_sites_executed=len(visited),uncovered_sites=[hex(s['instruction'])for s in meta['sites']if s['instruction']not in visited],fixture=str(a.output),fixture_bytes=len(encoded),fixture_sha256=sha(a.output),retained_large_artifacts=[],source_hashes={str(Path(__file__).relative_to(ROOT)):sha(__file__),'tools/observe_native_campaign_ai_vision_records_tu15.cpp':sha(ROOT/'tools/observe_native_campaign_ai_vision_records_tu15.cpp')});a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))
if __name__=='__main__':main()
