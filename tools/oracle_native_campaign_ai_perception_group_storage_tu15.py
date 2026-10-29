#!/usr/bin/env python3
"""Physical unchanged perception group find/state/reset/property creation."""
import argparse,copy,gzip,json,os,struct,subprocess
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
import oracle_native_campaign_screen_lifecycle_tu15 as common
import validate_native_geometry_tu15 as geometry
BASE,OBJECTS,PROPERTIES,CLUMPS,GROUPS=0x1000000,0x1000100,0x1000200,0x1000300,0x1000400
HEADERS,PROWS,CROWS,GROWS,BODY,POINT=BASE+0x1000,BASE+0x2000,BASE+0x4000,BASE+0x5000,BASE+0x7000,BASE+0x8000
sha=common.sha
def prepare(image,meta,scratch):
    owned=scratch/'physical-original';owned.mkdir();section=image.section('.text')
    (owned/'text.bin').write_bytes(image.section_bytes(section));(owned/'metadata.json').write_text(json.dumps(meta))
    assembly=f'.code32\n.section .original,"ax"\n.incbin "{owned/"text.bin"}"\n'
    linker=f'ENTRY(_start)\nSECTIONS{{.original 0x{section.va:x}:{{*(.original)}} '
    for name,address,size in [('bank',BASE,0x10000),('objects',0x4ec000,4096),('groups',0x50e000,4096),('maximum',0x456000,4096),('point',0x474000,4096),('stack',geometry.STACK,65536)]:
        assembly+=f'.section .{name},"aw"\n.space {size}\n';linker+=f'.{name} 0x{address:x}:{{*(.{name})}} '
    assembly+='.section .stop,"ax"\nint3\n.section .bridge,"ax"\n.globl _start\n_start:\nfninit\nint3\njmp _start\n'
    linker+=f'.stop 0x{geometry.STOP:x}:{{*(.stop)}} .bridge 0x70000000:{{*(.bridge)}}}}\n'
    (owned/'reference.s').write_text(assembly);(owned/'link.ld').write_text(linker)
    for command in (['as','--32',str(owned/'reference.s'),'-o',str(owned/'reference.o')],['ld','-melf_i386','-T',str(owned/'link.ld'),str(owned/'reference.o'),'-o',str(owned/'reference')],['c++','-std=c++20','-O2',str(ROOT/'tools/observe_native_campaign_ai_perception_group_tu15.cpp'),'-o',str(owned/'observer')]):
        result=subprocess.run(command,text=True,capture_output=True);assert result.returncode==0,result.stderr
    return owned
def inputs(image):
    rows=[]
    def make(entry,name,kind=0,attachment=0,extension=0x400,owner=0xe1000003,group_targets=(),group_owners=(),group_flags=(),group_state=0,member_states=(),default=POINT+0x100,point=POINT,full=False,cursor=0,salt=0x8000,property_rows=PROWS,group_rows=GROWS,changes=()):
        n=len(rows);banks={BASE:bytearray((k*13+n*7+3)&255 for k in range(0x10000)),0x4ec000:bytearray([0x53]*4096),0x50e000:bytearray([0x57]*4096),0x456000:bytearray(image.read(0x456000,4096)),0x474000:bytearray(image.read(0x474000,4096))}
        def put(p,value,width=4):
            for first,data in banks.items():
                if first<=p and p+width<=first+len(data):data[p-first:p-first+width]=(value&((1<<(width*8))-1)).to_bytes(width,'little');return
            raise ValueError((hex(p),width))
        for p,v in [(0x4ecbd0,OBJECTS),(OBJECTS+0x44,HEADERS),(HEADERS+12+8,BODY),(0x50eef0,PROPERTIES),(0x50eef4,GROUPS),(0x50eef8,CLUMPS),(GROUPS+0x44,group_rows),(CLUMPS+0x44,CROWS),(PROPERTIES+0x44,property_rows),(PROPERTIES+0x48,BASE+0x800),(PROPERTIES+0x34,cursor),(PROPERTIES+0x38,8 if full else 0),(PROPERTIES+0x20,8),(PROPERTIES+0x24,0x3c),(PROPERTIES+0x3c,8 if full else 0),(BASE+0x800,255 if full else 0),(BODY+0x134,attachment),(0x474ce8,default)]:put(p,v)
        put(PROPERTIES+0x40,salt,2);put(BODY+0xaa,kind,1);put(BODY+0x13a,extension,2)
        if kind==12 and attachment==0:put(BODY+extension+4,owner)
        put(CROWS+0x50+0x14,0xe2000000 if group_targets else 0xffffffff)
        for i,target in enumerate(group_targets):
            record=group_rows+i*0xc4;put(record+8,target);put(record+0x14,0xe2000000+i+1 if i+1<len(group_targets) else 0xffffffff);put(record+0x22,group_flags[i] if i<len(group_flags) else 0,1);put(record+0x1c,group_owners[i] if i<len(group_owners) else 0xffffffff)
        if entry!=0x274e30:
            put(group_rows+2,kind,2);put(group_rows+4,group_state,2);put(group_rows+8,0xe1000001)
        put(group_rows+0x18,0xe3000000 if member_states else 0xffffffff)
        for i,state in enumerate(member_states):put(property_rows+i*0x3c+0x24,state,2);put(property_rows+i*0x3c+0x34,0xe3000000+i+1 if i+1<len(member_states) else 0xffffffff)
        for p,v,w in changes:put(p,v,w)
        gp=[0x5a010000+n*31+k*7 for k in range(8)];gp[4]=geometry.SP+n%4*4
        if entry==0x274e30:gp[0]=0xe4000001;gp[7]=0xe1000001
        elif entry==0x274ae0:gp[1]=0xe2000000
        elif entry==0x2745e0:gp[0]=0xe2000000
        else:gp[0]=point
        rows.append(dict(id=n,name=f'{name}_{n}',entry=entry,gp=gp,matrix=0,source=0,point=point,group=0xe2000000,object=0xe1000001,clump=0xe4000001,banks=[dict(address=p,before=data.hex()) for p,data in sorted(banks.items())],before_fp=[0x27f|((n%4)<<10),((n%8)<<11)|(n*3%64),0x1f80|((n%4)<<13)|(0,0x40,0x8000,0x8040)[n%4]],live_x87=[struct.pack('<QH',0x8000000000000000+(k<<58),0x3fff+k%3|(0x8000 if k%2 else 0)).hex() for k in range((0,1,3,8)[n%4])]))
    for kind in (0,1,5,12,255):
        for targets in ((),(0xe1000001,),(0xe1000005,0xe1000001),(0xe1000005,0xe1000006)):
            make(0x274e30,'find_object',kind=kind,group_targets=targets)
    for state in (0,1,0xffff):
        for states in ((),(0,),(1,),(0xffff,0,1),(0xffff,0,0xffff)):
            make(0x274ae0,'active_state',group_state=state,member_states=states)
    for owner in (0xffffffff,0xe1000003):
        for attachment in (0,1,2):
            for flags in ((0,1),(1,1)):
                make(0x274e30,'find_attached_owner',kind=12,owner=owner,attachment=attachment,group_targets=(0xe1000005,0xe1000006),group_owners=(0xe1000003,0xe1000003),group_flags=flags)
    for extension in (-0x100,0,0x100,0x400):make(0x274e30,'find_extension',kind=12,extension=extension,group_targets=(0xe1000002,),group_owners=(0xe1000003,),group_flags=(1,))
    for default in (POINT+0x100,POINT,POINT+0x40,POINT+0x48,POINT+0x4c,POINT-8):
        for alignment in (0,1,3):make(0x2660f0,'reset_point_alias',default=default+alignment,point=POINT+alignment)
    for full in (False,True):
        for salt in (0,0x7fff,0x8000,0xfffe):
            for state in (0,1,0xffff):make(0x2745e0,'create_property',full=full,salt=salt,kind=5,group_state=state)
    for offset in (0,2,4,8,0x18,0x24,0x44):make(0x2745e0,'create_group_alias',property_rows=GROWS+offset)
    for offset in (0x80,0x84,0x88):make(0x2745e0,'create_near_header',property_rows=PROPERTIES+offset)
    make(0x274e30,'fault_seed_find',kind=12,group_targets=(0xe1000002,0xe1000003),group_owners=(0xe1000003,0xe1000004),group_flags=(0,1))
    make(0x274ae0,'fault_seed_active',member_states=(0,0xffff,1))
    make(0x2660f0,'fault_seed_reset',default=POINT+0x40)
    make(0x2745e0,'fault_seed_create',salt=0xfffe,group_state=1)
    make(0x2745e0,'fault_seed_create_alias',property_rows=GROWS+2)
    return rows
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--report',type=Path,required=True);p.add_argument('--verify',action='store_true');a=p.parse_args()
    assert a.verify or not a.output.exists();assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT)
    common.ENTRIES=(0x274e30,0x274ae0,0x2660f0,0x2745e0,0x16e6a0);meta=common.metadata(image)
    for site in meta['sites']:
        if site['bytes'].startswith('f3'):site['repeat_count_register']=1
        if site['instruction']==0x16e6c5:assert site['bytes']=='852c8b';site['operands'][0]['read']=True
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);owned=prepare(image,meta,scratch);rows=geometry.physical(owned,inputs(image),'complete');prefix=[]
    for seed in (r for r in rows if r['name'].startswith('fault_seed')):
        for phase,events in (('before_read',seed['reads']),('before_write',seed['writes']),('after_write',seed['writes'])):
            for n in range(len(events)):
                row=copy.deepcopy(seed);row.update(id=len(rows)+len(prefix),name=seed['name']+'_'+phase+'_'+str(n),fault=dict(phase=phase,ordinal=n));prefix.append(row)
    rows+=geometry.physical(owned,prefix,'prefix');visited=set().union(*(set(r['executed_sites'])for r in rows));uncovered=[s['instruction'] for s in meta['sites'] if s['instruction'] not in visited]
    fixture=dict(image_sha256=NEW_PIN,backend='physical-unchanged-i386-offline',owners=meta['owners'],instruction_patches=[],supplied_original_outcomes=False,cases=rows);encoded=gzip.compress(json.dumps(fixture,separators=(',',':')).encode(),mtime=0)
    if a.verify:assert a.output.read_bytes()==encoded,'Independent physical reference repeat differs'
    else:a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_bytes(encoded)
    report=dict(passed=True,original_sha256=NEW_PIN,owners=meta['owners'],cases=len(rows),complete_calls=sum(r['completed']for r in rows),fault_prefixes=len(prefix),original_sites_executed=len(visited),uncovered_sites=[hex(p) for p in uncovered],fixture=str(a.output),fixture_bytes=len(encoded),fixture_sha256=sha(a.output),retained_large_artifacts=[],full2749F0_closed=False,source_hashes={str(Path(__file__).relative_to(ROOT)):sha(__file__),'tools/observe_native_campaign_ai_perception_group_tu15.cpp':sha(ROOT/'tools/observe_native_campaign_ai_perception_group_tu15.cpp')});a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))
if __name__=='__main__':main()
