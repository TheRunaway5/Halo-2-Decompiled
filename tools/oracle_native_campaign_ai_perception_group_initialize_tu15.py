#!/usr/bin/env python3
"""Physical274020 target/alliance initialization; campaign policy is real data."""
import argparse,copy,gzip,json,os,struct,subprocess
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
import oracle_native_campaign_screen_lifecycle_tu15 as common
import validate_native_geometry_tu15 as geometry
from oracle_native_campaign_ai_perception_group_storage_tu15 import BASE,OBJECTS,PROPERTIES,CLUMPS,GROUPS,HEADERS,PROWS,CROWS,GROWS,BODY,POINT
sha=common.sha
GAME,AI,ENGINES=BASE+0xa000,BASE+0xb000,BASE+0xc000
def prepare(image,meta,scratch):
    owned=scratch/'physical-original';owned.mkdir();section=image.section('.text');(owned/'text.bin').write_bytes(image.section_bytes(section));(owned/'metadata.json').write_text(json.dumps(meta))
    assembly=f'.code32\n.section .original,"ax"\n.incbin "{owned/"text.bin"}"\n';linker=f'ENTRY(_start)\nSECTIONS{{.original 0x{section.va:x}:{{*(.original)}} '
    for name,address,size in [('bank',BASE,0x10000),('objects',0x4ec000,4096),('groups',0x50e000,4096),('policy',0x47d000,4096),('point',0x474000,4096),('game',0x4f3000,4096),('engine',0x4f6000,4096),('ai',0x502000,4096),('vtable',0x56d000,4096),('stack',geometry.STACK,65536)]:
        assembly+=f'.section .{name},"aw"\n.space {size}\n';linker+=f'.{name} 0x{address:x}:{{*(.{name})}} '
    assembly+='.section .stop,"ax"\nint3\n.section .bridge,"ax"\n.globl _start\n_start:\nfninit\nint3\njmp _start\n';linker+=f'.stop 0x{geometry.STOP:x}:{{*(.stop)}} .bridge 0x70000000:{{*(.bridge)}}}}\n';(owned/'reference.s').write_text(assembly);(owned/'link.ld').write_text(linker)
    for command in (['as','--32',str(owned/'reference.s'),'-o',str(owned/'reference.o')],['ld','-melf_i386','-T',str(owned/'link.ld'),str(owned/'reference.o'),'-o',str(owned/'reference')],['c++','-std=c++20','-O2',str(ROOT/'tools/observe_native_campaign_ai_perception_group_tu15.cpp'),'-o',str(owned/'observer')]):
        r=subprocess.run(command,text=True,capture_output=True);assert r.returncode==0,r.stderr
    return owned
def inputs(image):
    rows=[]
    def make(entry=0x274020,name='initialize',body_kind=0,kind=1,team=2,clump_team=3,mode=1,allied=False,primary_policy=False,attachment=0,attachment_owner=0xe8000002,attachment_member=0xe8000004,extension=0x400,player=0xffffffff,actor=0xe1000002,record=GROWS,clump=CROWS,default=POINT,priority=3,policy=None,changes=()):
        n=len(rows);banks={BASE:bytearray((k*13+n*7+3)&255 for k in range(0x10000)),0x4ec000:bytearray([0x53]*4096),0x50e000:bytearray([0x57]*4096),0x47d000:bytearray(image.read(0x47d000,4096)),0x474000:bytearray(image.read(0x474000,4096)),0x4f3000:bytearray([0x61]*4096),0x4f6000:bytearray([0x67]*4096),0x502000:bytearray([0x71]*4096),0x56d000:bytearray([0]*4096)}
        def put(p,v,w=4):
            for first,data in banks.items():
                if first<=p and p+w<=first+len(data):data[p-first:p-first+w]=(v&((1<<(w*8))-1)).to_bytes(w,'little');return
            raise ValueError((hex(p),w))
        for p,v in [(0x4ecbd0,OBJECTS),(OBJECTS+0x44,HEADERS),(HEADERS+12+8,BODY),(0x474ce8,default),(BODY+0x12c,actor),(BODY+0x13c,player),(BODY+0x134,attachment),(0x4f3218,GAME),(GAME+8,mode),(0x5020c4,AI),(0x4f63b8,ENGINES),(ENGINES+0xc14,3)]:put(p,v)
        put(BODY+0xaa,body_kind,1);put(BODY+0xb2,0xa4,1);put(BODY+0x138,team,2);put(BODY+0x12e,team,2);put(BODY+0x13a,extension,2);put(clump+0x12,clump_team,2)
        if body_kind&31==12 and attachment==0:put(BODY+extension+4,attachment_owner);put(BODY+extension+8,attachment_member)
        for i in range(16):put(AI+0xa4+i*4,0)
        if 0<=team<16 and 0<=clump_team<16:
            index=clump_team*16+team
            put(AI+0xc4+(index>>5)*4,(1<<(index&31))if allied else 0)
            put(AI+0xa4+(index>>5)*4,(1<<(index&31))if primary_policy else 0)
        if policy is not None:put(0x47d634+kind*16,policy,2)
        for p,v,w in changes:put(p,v,w)
        gp=[0x5a010000+n*31+k*7 for k in range(8)];gp[4]=geometry.SP+n%4*4
        if entry==0x274020:gp[0]=0xe9000001;gp[1]=kind&0xffff;gp[2]=clump;gp[7]=record
        else:
            gp[1]=clump_team&0xffff
            if entry==0x1e78f0:gp[2]=team&0xffff
            else:gp[6]=team&0xffff
        rows.append(dict(id=n,name=f'{name}_{n}',entry=entry,gp=gp,matrix=priority if entry==0x274020 else 0,source=0,record=record,object=0xe9000001,clump=clump,kind=kind,priority=priority,first=clump_team,second=team,banks=[dict(address=p,before=data.hex())for p,data in sorted(banks.items())],before_fp=[0x27f|((n%4)<<10),((n%8)<<11)|(n*3%64),0x1f80|((n%4)<<13)|(0,0x40,0x8000,0x8040)[n%4]],live_x87=[struct.pack('<QH',0x8000000000000000+(k<<58),0x3fff+k%3|(0x8000 if k%2 else 0)).hex()for k in range((0,1,3,8)[n%4])]))
    for body_kind in range(256):make(name='all_type_bytes',body_kind=body_kind,allied=bool(body_kind&1))
    for body_kind in (0,1,12,44):
        for team,clump_team in [(-1,1),(1,-1),(-2,1),(1,-2),(16,2),(2,16),(0,0),(15,15),(0,15),(15,0)]:
            for mode in (0,1,2,3):make(name='team_policy',body_kind=body_kind,team=team,clump_team=clump_team,mode=mode)
    for first in (-32768,-2,-1,0,1,15,16,32767):
        for second in (-2,-1,0,1,15,16):
            for mode in (0,1,2,3):make(entry=0x1e7810,name='team_owner',team=second,clump_team=first,mode=mode,allied=bool(len(rows)&1))
    for attachment in (0,1,2,0xffffffff):
        for owner in (0xffffffff,0xe8000002):
            for member in (0xffffffff,0xe8000004):make(name='attachment_state',body_kind=12,attachment=attachment,attachment_owner=owner,attachment_member=member)
    for kind in (-1,0,1,2,8):
        for policy in (0,1,2,0xffff):make(name='signed_kind_policy',kind=kind,policy=policy)
    for record,default,clump in [(GROWS+1,POINT+1,CROWS+1),(BODY,POINT,CROWS),(BODY+0x120,POINT,CROWS),(CROWS,POINT,CROWS),(POINT,POINT+0x98,CROWS),(GROWS, GROWS+0x98,CROWS)]:make(name='owner_alias',record=record,default=default,clump=clump)
    for player in (0,0xe1000003,0xffffffff):make(name='player_presence',player=player)
    for first,second in [(-1,1),(1,-1),(1,1),(-2,1),(1,-2),(16,1),(1,16),(0,1),(15,0),(0,15)]:
        for mode in (0,1,2):
            for a,b in ((False,False),(True,False),(True,True),(False,True)):make(entry=0x1e78f0,name='secondary_policy',clump_team=first,team=second,mode=mode,primary_policy=a,allied=b)
    for body_kind,attachment in [(0,0),(12,0),(12,2),(5,0)]:make(name='fault_seed_initialize',body_kind=body_kind,attachment=attachment,allied=True)
    make(entry=0x1e7810,name='fault_seed_team',team=15,clump_team=14,allied=True)
    make(entry=0x1e78f0,name='fault_seed_secondary',team=15,clump_team=14,primary_policy=True,allied=False)
    return rows
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--report',type=Path,required=True);p.add_argument('--verify',action='store_true');a=p.parse_args();assert a.verify or not a.output.exists();assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT)
    common.ENTRIES=(0x274020,0x2660f0,0x1e7810,0x1e78f0,0x160a60,0xc1e90);meta=common.metadata(image);owned=prepare(image,meta,Path(os.environ['HALO2_TEST_SCRATCH']));rows=geometry.physical(owned,inputs(image),'complete');prefix=[]
    for seed in (r for r in rows if r['name'].startswith('fault_seed')):
        for phase,events in (('before_read',seed['reads']),('before_write',seed['writes']),('after_write',seed['writes'])):
            for n in range(len(events)):
                row=copy.deepcopy(seed);row.update(id=len(rows)+len(prefix),name=seed['name']+'_'+phase+'_'+str(n),fault=dict(phase=phase,ordinal=n));prefix.append(row)
    rows+=geometry.physical(owned,prefix,'prefix');visited=set().union(*(set(r['executed_sites'])for r in rows));uncovered=[s['instruction']for s in meta['sites']if s['instruction']not in visited];fixture=dict(image_sha256=NEW_PIN,backend='physical-unchanged-i386-offline',owners=meta['owners'],instruction_patches=[],supplied_original_outcomes=False,cases=rows);encoded=gzip.compress(json.dumps(fixture,separators=(',',':')).encode(),mtime=0)
    if a.verify:assert a.output.read_bytes()==encoded,'Physical initializer repeat differs'
    else:a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_bytes(encoded)
    report=dict(passed=True,owners=meta['owners'],cases=len(rows),complete_calls=sum(r['completed']for r in rows),fault_prefixes=len(prefix),original_sites_executed=len(visited),uncovered_sites=[hex(p)for p in uncovered],fixture=str(a.output),fixture_bytes=len(encoded),fixture_sha256=sha(a.output),retained_large_artifacts=[],unknown_multiplayer_callback_closed=False,full2749F0_closed=False,source_hashes={str(Path(__file__).relative_to(ROOT)):sha(__file__),'tools/observe_native_campaign_ai_perception_group_tu15.cpp':sha(ROOT/'tools/observe_native_campaign_ai_perception_group_tu15.cpp')});a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))
if __name__=='__main__':main()
