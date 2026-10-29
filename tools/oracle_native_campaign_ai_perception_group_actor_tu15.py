#!/usr/bin/env python3
"""Physical274670/274750 actor/perception linking and balanced native distance."""
import argparse,copy,gzip,json,os,struct,subprocess
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
import oracle_native_campaign_screen_lifecycle_tu15 as common
import validate_native_geometry_tu15 as geometry
BASE=0x1000000
ACTORS,PROPERTIES,GROUPS,OBJECTS=BASE+0x100,BASE+0x200,BASE+0x300,BASE+0x400
AROWS,PROWS,GROWS,HEADERS,BODY,PARENT,CLUMP,BITS=BASE+0x1000,BASE+0x4000,BASE+0x6000,BASE+0x8000,BASE+0x9000,BASE+0xb000,BASE+0xd000,BASE+0xe000
sha=common.sha
def prepare(image,meta,scratch):
    owned=scratch/'physical-original';owned.mkdir();section=image.section('.text');(owned/'text.bin').write_bytes(image.section_bytes(section));(owned/'metadata.json').write_text(json.dumps(meta));assembly=f'.code32\n.section .original,"ax"\n.incbin "{owned/"text.bin"}"\n';linker=f'ENTRY(_start)\nSECTIONS{{.original 0x{section.va:x}:{{*(.original)}} '
    for name,address,size in [('bank',BASE,0x10000),('objects',0x4ec000,4096),('groups',0x50e000,4096),('actors',0x502000,4096),('scalar',0x46a000,4096),('maximum',0x456000,4096),('stack',geometry.STACK,65536)]:
        assembly+=f'.section .{name},"aw"\n.space {size}\n';linker+=f'.{name} 0x{address:x}:{{*(.{name})}} '
    assembly+='.section .stop,"ax"\nint3\n.section .bridge,"ax"\n.globl _start\n_start:\nfninit\nint3\njmp _start\n';linker+=f'.stop 0x{geometry.STOP:x}:{{*(.stop)}} .bridge 0x70000000:{{*(.bridge)}}}}\n';(owned/'reference.s').write_text(assembly);(owned/'link.ld').write_text(linker)
    for command in (['as','--32',str(owned/'reference.s'),'-o',str(owned/'reference.o')],['ld','-melf_i386','-T',str(owned/'link.ld'),str(owned/'reference.o'),'-o',str(owned/'reference')],['c++','-std=c++20','-O2',str(ROOT/'tools/observe_native_campaign_ai_perception_group_actor_tu15.cpp'),'-o',str(owned/'observer')]):
        r=subprocess.run(command,text=True,capture_output=True);assert r.returncode==0,r.stderr
    return owned
def inputs(image):
    rows=[];bits=lambda f:struct.unpack('<I',struct.pack('<f',f))[0]
    def make(entry=0x274750,name='property_link',actors=1,old_head=False,group_head=False,full=False,capacity=16,parent=False,scale=0x3f800000,property_rows=PROWS,clump=CLUMP,vector=(0,0,0),target=(0x3fc00000,0xc0000000,0x40400000),rounding=0,precision=2,depth=0,changes=()):
        n=len(rows);banks={BASE:bytearray((k*13+n*7+3)&255 for k in range(0x10000)),0x4ec000:bytearray([0x53]*4096),0x50e000:bytearray([0x57]*4096),0x502000:bytearray([0x61]*4096),0x46a000:bytearray(image.read(0x46a000,4096)),0x456000:bytearray(image.read(0x456000,4096))}
        def put(p,v,w=4):
            for first,data in banks.items():
                if first<=p and p+w<=first+len(data):data[p-first:p-first+w]=(v&((1<<(8*w))-1)).to_bytes(w,'little');return
            raise ValueError((hex(p),w))
        for p,v in [(0x5020c8,ACTORS),(ACTORS+0x44,AROWS),(0x50eef0,PROPERTIES),(PROPERTIES+0x44,property_rows),(PROPERTIES+0x24,0x3c),(PROPERTIES+0x34,0),(PROPERTIES+0x38,8),(PROPERTIES+0x20,capacity),(PROPERTIES+0x3c,4),(PROPERTIES+0x48,BITS),(0x50eef4,GROUPS),(GROUPS+0x44,GROWS),(0x4ecbd0,OBJECTS),(OBJECTS+0x44,HEADERS),(HEADERS+12+8,BODY),(HEADERS+24+8,PARENT),(BODY+0x14,0xea000002 if parent else 0xffffffff),(CLUMP+0x14,0xe4000001),(GROWS+0x18,0xe2000006 if group_head else 0xffffffff),(GROWS+8,0xea000001),(BITS,0xffff if full else 0xf0)]:put(p,v)
        put(PROPERTIES+0x40,0x8000,2);put(GROWS+2,5,2);put(GROWS+4,1,2);put(clump+0x18,0xe1000000 if actors else 0xffffffff)
        for i in range(max(actors,1)):
            actor=AROWS+i*0x888;put(actor+0x80,0xe1000000+i+1 if i+1<actors else 0xffffffff);put(actor+0x58,0xe2000004+i if old_head else 0xffffffff)
            for lane in range(3):put(actor+0x238+lane*4,vector[lane]);put(BODY+0x64+lane*4,target[lane])
        put(property_rows+0x20,0xea000001)
        put(BODY+0x18,0,1);put(PARENT+0x116,0x100,2)
        matrix=PARENT+0x100
        for i in range(13):put(matrix+i*4,bits(0.))
        for i in (1,5,9):put(matrix+i*4,bits(1.))
        put(matrix,scale);put(matrix+0x28,bits(2.));put(matrix+0x2c,bits(-3.));put(matrix+0x30,bits(4.))
        for p,v,w in changes:put(p,v,w)
        gp=[0x5a010000+n*31+k*7 for k in range(8)];gp[4]=geometry.SP+n%4*4
        if entry==0x274750:gp[0]=0xe1000000;gp[2]=0xe2000000
        else:gp[0]=clump
        rows.append(dict(id=n,name=f'{name}_{n}',entry=entry,gp=gp,matrix=0xe3000000,source=0,actor=0xe1000000,property=0xe2000000,clump=clump,group=0xe3000000,banks=[dict(address=p,before=data.hex())for p,data in sorted(banks.items())],before_fp=[0x7f|(precision<<8)|(rounding<<10),((n%8)<<11)|((n*0x100)&0x4500)|(n*3%64),0x1f80|(rounding<<13)|(0,0x40,0x8000,0x8040)[n%4]],live_x87=[struct.pack('<QH',0x8000000000000000+(k<<58),0x3fff+k%3|(0x8000 if k%2 else 0)).hex()for k in range(depth)],numeric_profile='complete source bits/status; private x87 temporary fault transport excluded'))
    for precision in (0,2,3):
        for rounding in range(4):
            for depth in (0,1,3):
                for parent in (False,True):make(name='controls',rounding=rounding,precision=precision,depth=depth,parent=parent,old_head=bool(len(rows)&1),scale=0x40000000)
    for lane in range(3):
        for value in (0x80000000,1,0x00800000,0x7f7fffff,0x7f800000,0xff800000,0x7fc12345,0x7f812345):
            vector=[0,0,0];vector[lane]=value;make(name='numeric_actor',vector=tuple(vector),precision=(0,2,3)[len(rows)%3],rounding=len(rows)%4,depth=len(rows)%4)
            target=[0x3fc00000,0xc0000000,0x40400000];target[lane]=value;make(name='numeric_target',target=tuple(target),parent=bool(len(rows)&1),scale=0x40000000)
    for first,second in [(0x7fc12345,0xffc54321),(0x7f812345,0xff854321)]:
        make(name='paired_nan',vector=(first,0,0),target=(second,0,0));make(name='paired_nan',vector=(0,first,0),target=(0,second,0));make(name='paired_nan',vector=(0,0,first),target=(0,0,second))
    for actors in (0,1,2,3):
        for old_head in (False,True):
            for group_head in (False,True):
                for full in (False,True):make(entry=0x274670,name='group_fanout',actors=actors,old_head=old_head,group_head=group_head,full=full,parent=actors==2,scale=0x40000000,depth=3 if actors else 8)
    for capacity in (0,1,2):make(entry=0x274670,name='partial_property_refusal',actors=3,capacity=capacity)
    make(name='actor_head_alias',old_head=True,property_rows=AROWS-0x14,changes=[(AROWS+0x58,0xe2000001,4)])
    make(entry=0x274670,name='empty_clump_group_alias',actors=0,clump=GROWS)
    make(name='fault_seed_property',old_head=True,parent=True,scale=0x40000000,rounding=2,depth=3)
    make(name='fault_seed_alias',old_head=True,property_rows=AROWS-0x14,changes=[(AROWS+0x58,0xe2000001,4)],depth=1)
    make(entry=0x274670,name='fault_seed_group',actors=2,old_head=True,group_head=True,parent=True,scale=0x40000000,depth=1)
    return rows
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--report',type=Path,required=True);p.add_argument('--verify',action='store_true');a=p.parse_args();assert a.verify or not a.output.exists();assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);common.ENTRIES=(0x274670,0x274750,0x2745e0,0x16e6a0,0xbbe00);meta=common.metadata(image)
    for site in meta['sites']:
        if site['bytes'].startswith('f3'):site['repeat_count_register']=1
        if site['instruction']==0x16e6c5:assert site['bytes']=='852c8b';site['operands'][0]['read']=True
        # Capstone labels x87 FSTP m32 as a read/write operand. The original
        # D9/3 store does not read its destination; its before bytes belong to
        # store diagnostics only, never the semantic source-read sequence.
        if site['instruction']==0x2747eb:
            assert site['bytes']=='d95b28' and len(site['operands'])==1
            site['operands'][0]['read']=False;site['operands'][0]['write']=True
    owned=prepare(image,meta,Path(os.environ['HALO2_TEST_SCRATCH']));rows=geometry.physical(owned,inputs(image),'complete');prefix=[]
    for seed in (r for r in rows if r['name'].startswith('fault_seed')):
        for phase,events in (('before_read',seed['reads']),('before_write',seed['writes']),('after_write',seed['writes'])):
            for n in range(len(events)):
                row=copy.deepcopy(seed);row.update(id=len(rows)+len(prefix),name=seed['name']+'_'+phase+'_'+str(n),fault=dict(phase=phase,ordinal=n));prefix.append(row)
    rows+=geometry.physical(owned,prefix,'prefix');visited=set().union(*(set(r['executed_sites'])for r in rows));uncovered=[s['instruction']for s in meta['sites']if s['instruction']not in visited];fixture=dict(image_sha256=NEW_PIN,backend='physical-unchanged-i386-offline',owners=meta['owners'],instruction_patches=[],supplied_original_outcomes=False,cases=rows);encoded=gzip.compress(json.dumps(fixture,separators=(',',':')).encode(),mtime=0)
    if a.verify:assert a.output.read_bytes()==encoded,'Physical actor-link repeat differs'
    else:a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_bytes(encoded)
    report=dict(passed=True,owners=meta['owners'],cases=len(rows),complete_calls=sum(r['completed']for r in rows),fault_prefixes=len(prefix),original_sites_executed=len(visited),uncovered_sites=[hex(p)for p in uncovered],fixture=str(a.output),fixture_bytes=len(encoded),fixture_sha256=sha(a.output),retained_large_artifacts=[],full2749F0_closed=False,private_numeric_fault_transport_closed=False,source_hashes={str(Path(__file__).relative_to(ROOT)):sha(__file__),'tools/observe_native_campaign_ai_perception_group_actor_tu15.cpp':sha(ROOT/'tools/observe_native_campaign_ai_perception_group_actor_tu15.cpp')});a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))
if __name__=='__main__':main()
