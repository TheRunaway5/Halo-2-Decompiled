#!/usr/bin/env python3
"""Physical273B70 genuine forced-new-group private-output caller profile."""
import argparse,copy,gzip,json,os,struct,subprocess
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
import oracle_native_campaign_screen_lifecycle_tu15 as common
import validate_native_geometry_tu15 as geometry
BASE=0x1000000
OBJECTS,ACTORS,PLAYERS,ATTACHMENTS=BASE+0x100,BASE+0x200,BASE+0x300,BASE+0x400
HEADERS,AROWS,PROWS,ATTACH_ROWS,BODY,CHILD1,CHILD2,PARENT,CLUMP,GAME,AI,TAGS,DEFINITION=BASE+0x1000,BASE+0x2000,BASE+0x4000,BASE+0x5000,BASE+0x6000,BASE+0x8000,BASE+0x9000,BASE+0xa000,BASE+0xc000,BASE+0xd000,BASE+0xd100,BASE+0xe000,BASE+0xe100
sha=common.sha
def prepare(image,meta,scratch):
    owned=scratch/'physical-original';owned.mkdir();section=image.section('.text');(owned/'text.bin').write_bytes(image.section_bytes(section));(owned/'metadata.json').write_text(json.dumps(meta));assembly=f'.code32\n.section .original,"ax"\n.incbin "{owned/"text.bin"}"\n';linker=f'ENTRY(_start)\nSECTIONS{{.original 0x{section.va:x}:{{*(.original)}} '
    for name,address,size in [('bank',BASE,0x10000),('objects',0x4ec000,4096),('actors',0x502000,4096),('players',0x4f5000,4096),('game',0x4f3000,4096),('tags',0x4f0000,4096),('generation',0x4ea000,4096),('attachments',0x510000,4096),('scalars',0x46a000,4096),('stack',geometry.STACK,65536)]:
        assembly+=f'.section .{name},"aw"\n.space {size}\n';linker+=f'.{name} 0x{address:x}:{{*(.{name})}} '
    assembly+='.section .stop,"ax"\nint3\n.section .bridge,"ax"\n.globl _start\n_start:\nfninit\nint3\njmp _start\n';linker+=f'.stop 0x{geometry.STOP:x}:{{*(.stop)}} .bridge 0x70000000:{{*(.bridge)}}}}\n';(owned/'reference.s').write_text(assembly);(owned/'link.ld').write_text(linker)
    for command in (['as','--32',str(owned/'reference.s'),'-o',str(owned/'reference.o')],['ld','-melf_i386','-T',str(owned/'link.ld'),str(owned/'reference.o'),'-o',str(owned/'reference')],['c++','-std=c++20','-O2',str(ROOT/'tools/observe_native_campaign_ai_perception_classify_tu15.cpp'),'-o',str(owned/'observer')]):
        r=subprocess.run(command,text=True,capture_output=True);assert r.returncode==0,r.stderr
    return owned
def inputs(image):
    rows=[];fbits=lambda f:struct.unpack('<I',struct.pack('<f',f))[0]
    def make(name='classify',body_kind=0,object=0xea000001,salt=0xea00,extent=16,generation=7,already=False,child_already=False,actor=0xe1000000,player=0xffffffff,team=2,clump_team=3,special=False,allied=False,mode=1,attachment=0,members=0,member_state=0,parent=False,scale=0x3f800000,dimension=0x3f800000,target=(0x3fc00000,0xc0000000,0x40400000),origin=(0,0,0),flags=0,changes=()):
        n=len(rows);banks={BASE:bytearray((k*13+n*7+3)&255 for k in range(0x10000)),0x4ec000:bytearray([0x53]*4096),0x502000:bytearray([0x57]*4096),0x4f5000:bytearray([0x61]*4096),0x4f3000:bytearray([0x67]*4096),0x4f0000:bytearray([0x71]*4096),0x4ea000:bytearray([0x79]*4096),0x510000:bytearray([0x83]*4096),0x46a000:bytearray(image.read(0x46a000,4096))}
        def put(p,v,w=4):
            for first,data in banks.items():
                if first<=p and p+w<=first+len(data):data[p-first:p-first+w]=(v&((1<<(w*8))-1)).to_bytes(w,'little');return
            raise ValueError((hex(p),w))
        for p,v in [(0x4ecbd0,OBJECTS),(OBJECTS+0x38,extent),(OBJECTS+0x24,12),(OBJECTS+0x44,HEADERS),(HEADERS+12+8,BODY),(HEADERS+24+8,PARENT),(HEADERS+36+8,CHILD1),(HEADERS+48+8,CHILD2),(0x5020c8,ACTORS),(ACTORS+0x44,AROWS),(0x4f54f4,PLAYERS),(PLAYERS+0x44,PROWS),(0x510fa8,ATTACHMENTS),(ATTACHMENTS+0x44,ATTACH_ROWS),(0x4f3218,GAME),(GAME+8,mode),(0x5020c4,AI),(0x4f0414,TAGS),(TAGS+8,DEFINITION),(DEFINITION+0xd0,dimension),(0x4eabcc,generation),(0x4eabd4,generation if already else generation-1),(BODY+0x14,0xea000002 if parent else 0xffffffff),(BODY+0x12c,actor if body_kind!=5 else flags),(BODY+0x13c,player),(BODY+0x134,attachment),(BODY,0xe5000000),(BODY+0x408,0xe6000000 if members else 0xffffffff),(ATTACH_ROWS+0x18,0xea000003 if members else 0xffffffff)]:put(p,v)
        put(HEADERS+12,salt,2);put(HEADERS+15,body_kind,1);put(BODY+0xaa,body_kind,1);put(BODY+0x10a,4 if special else 0,1);put(BODY+0x13a,0x400,2);put(BODY+0x138,team,2);put(AROWS+0x24,team,2);put(PROWS+0xc0,team,1);put(CLUMP+0x12,clump_team,2)
        if body_kind==12:put(BODY+0x12e,team,2)
        for i in range(8):put(AI+0xc4+i*4,0)
        if 0<=team<16 and 0<=clump_team<16:
            index=team*16+clump_team;put(AI+0xc4+(index>>5)*4,(1<<(index&31))if allied else 0)
        for i in range(3):put(BODY+0x64+i*4,target[i]);put(CLUMP+4+i*4,origin[i])
        for i,(body,next_object) in enumerate([(CHILD1,0xea000004 if members>=2 else 0xffffffff),(CHILD2,0xffffffff)]):
            put(body+0x134,member_state);put(body+0x13a,0x400,2);put(body+0x40c,next_object);put(0x4eabd0+(3+i)*4,generation if child_already else generation-1)
        put(BODY+0x18,0,1);put(PARENT+0x116,0x100,2)
        for i in range(13):put(PARENT+0x100+i*4,0)
        for i in (1,5,9):put(PARENT+0x100+i*4,fbits(1.))
        put(PARENT+0x100,scale)
        for p,v,w in changes:put(p,v,w)
        gp=[0x5a010000+n*31+k*7 for k in range(8)];gp[4]=geometry.SP+n%4*4;output=geometry.SP+0x400;depth=(0,1,3,8)[n%4]
        rows.append(dict(id=n,name=f'{name}_{n}',entry=0x273b70,gp=gp,arguments=[object,CLUMP,0xffffffff,output,1],private_output=output,object=object,clump=CLUMP,banks=[dict(address=p,before=data.hex())for p,data in sorted(banks.items())],before_fp=[0x27f|((n%4)<<10),((n%8)<<11)|(n*3%64),0x1f80|((n%4)<<13)|(0,0x40,0x8000,0x8040)[n%4]],live_x87=[struct.pack('<QH',0x8000000000000000+(k<<58),0x3fff+k%3|(0x8000 if k%2 else 0)).hex()for k in range(depth)]))
    for kind in range(256):make(name='all_type_bytes',body_kind=kind)
    for kind in (0,1):
        for actor,player in [(0xe1000000,0xffffffff),(0xffffffff,0xffffffff),(0xffffffff,0xe2000000)]:
            for team,clump_team in [(2,3),(3,3),(-1,3),(2,-1),(-2,3),(16,3)]:
                for special in (False,True):
                    for allied in (False,True):make(name='unit_policy',body_kind=kind,actor=actor,player=player,team=team,clump_team=clump_team,special=special,allied=allied)
    for attachment in (0,1,2):
        for members in (0,1,2):
            for member_state in (0,1):
                for allied in (False,True):make(name='attachment_traversal',body_kind=12,attachment=attachment,members=members,member_state=member_state,allied=allied)
    make(name='attachment_already_stamped',body_kind=12,members=2,child_already=True)
    for dimension in (0,0x3f800000,0x80000000,0xbf800000,0x7f800000,0x7fc12345,0x7f812345):
        for parent in (False,True):
            for flags in (0,0x20):make(name='definition_dimension',body_kind=5,dimension=dimension,parent=parent,flags=flags)
    for salt,extent,object in [(0,16,0xea000001),(1,16,0xea000001),(0xea00,0,0xea000001),(0xea00,16,0xffffffff)]:make(name='object_admission',salt=salt,extent=extent,object=object)
    for kind in (0,1,5,12,255):make(name='already_stamped',body_kind=kind,already=True)
    for value in (1,0x7f7fffff,0x7f800000,0x7fc12345,0x7f812345):
        for parent in (False,True):make(name='numeric_policy',parent=parent,target=(value,0x7f812346,0x40400000),origin=(0xffc54321,value,1),scale=0x40000000)
    for _ in range(4):make(name='forced_sum_overflow',target=(fbits(1.6e19),0,fbits(1.6e19)))
    make(name='fault_seed_unit',special=True,parent=True,scale=0x40000000)
    make(name='fault_seed_attachment',body_kind=12,members=2)
    make(name='fault_seed_definition',body_kind=5,parent=True,flags=0x20)
    return rows
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--report',type=Path,required=True);p.add_argument('--verify',action='store_true');a=p.parse_args();assert a.verify or not a.output.exists();assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);common.ENTRIES=(0x273b70,0xbcdf0,0xbbe00,0x1e7810,0x160a60,0xc1e90,0x29a990,0xc0ca0);meta=common.metadata(image);owned=prepare(image,meta,Path(os.environ['HALO2_TEST_SCRATCH']));rows=geometry.physical(owned,inputs(image),'complete');prefix=[]
    for seed in (r for r in rows if r['name'].startswith('fault_seed')):
        for phase,events in (('before_read',seed['reads']),('before_write',seed['writes']),('after_write',seed['writes'])):
            for n in range(len(events)):
                row=copy.deepcopy(seed);row.update(id=len(rows)+len(prefix),name=seed['name']+'_'+phase+'_'+str(n),fault=dict(phase=phase,ordinal=n));prefix.append(row)
    rows+=geometry.physical(owned,prefix,'prefix');visited=set().union(*(set(r['executed_sites'])for r in rows));uncovered=[s['instruction']for s in meta['sites']if s['instruction']not in visited];fixture=dict(image_sha256=NEW_PIN,backend='physical-unchanged-i386-offline',caller_profile='274450: force1,previous_groupNONE,valid private16byte output',owners=meta['owners'],instruction_patches=[],supplied_original_outcomes=False,cases=rows);encoded=gzip.compress(json.dumps(fixture,separators=(',',':')).encode(),mtime=0)
    if a.verify:assert a.output.read_bytes()==encoded,'Physical forced-classifier repeat differs'
    else:a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_bytes(encoded)
    report=dict(passed=True,owners=meta['owners'],cases=len(rows),complete_calls=sum(r['completed']for r in rows),fault_prefixes=len(prefix),original_sites_executed=len(visited),uncovered_sites=[hex(p)for p in uncovered],fixture=str(a.output),fixture_bytes=len(encoded),fixture_sha256=sha(a.output),retained_large_artifacts=[],generic273B70_closed=False,full2749F0_closed=False,source_hashes={str(Path(__file__).relative_to(ROOT)):sha(__file__),'tools/observe_native_campaign_ai_perception_classify_tu15.cpp':sha(ROOT/'tools/observe_native_campaign_ai_perception_classify_tu15.cpp')});a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))
if __name__=='__main__':main()
