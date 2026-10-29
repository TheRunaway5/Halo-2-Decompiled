#!/usr/bin/env python3
"""Unchanged physical camera control/seat selection/mode publication; genuine unit predicates."""
import argparse,copy,gzip,json,os,struct,subprocess
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
import oracle_native_campaign_screen_lifecycle_tu15 as common
import validate_native_geometry_tu15 as geometry
sha=common.sha
ENTRIES=(0x158560,0x158340,0x158900,0xbcdf0,0x111cd0,0xc9550)
BASE,CONTROL,DIRECTOR,USERS,PLAYERS,OBJECTS,PLAYER_ROWS,HEADERS,OUT=0x1000000,0x1000000,0x1000100,0x1000200,0x1000300,0x1000800,0x1001000,0x1003000,0x1000280
TAGS,DEFINITION,SEATS,SCENARIO,DURATIONS=0x100c000,0x100d000,0x100e200,0x100f000,0x1010000
def prepare(image,meta,scratch):
    p=scratch/'physical-original';p.mkdir();section=image.section('.text');(p/'text.bin').write_bytes(image.section_bytes(section));(p/'metadata.json').write_text(json.dumps(meta))
    regions={'data':(BASE,0x20000),'globals':(0x4e9000,0x36000),'constants':(0x46a000,0x1000),'whitelist':(0x473000,0x1000),'stack':(geometry.STACK,65536)}
    assembly=f'.code32\n.section .original,"ax"\n.incbin "{p/"text.bin"}"\n'
    for name,(_,size)in regions.items():assembly+=f'.section .{name},"aw"\n.space {size}\n'
    assembly+='''.section .stop,"ax"
int3
.section .bridge,"ax"
.globl _start
_start:
fninit
int3
jmp _start
'''
    (p/'reference.s').write_text(assembly);(p/'link.ld').write_text(f'ENTRY(_start)\nSECTIONS{{.original 0x{section.va:x}:{{*(.original)}} '+''.join(f'.{n} 0x{a:x}:{{*(.{n})}} 'for n,(a,_)in regions.items())+f'.stop 0x{geometry.STOP:x}:{{*(.stop)}} .bridge 0x70000000:{{*(.bridge)}}}}\n')
    for command in(['as','--32',str(p/'reference.s'),'-o',str(p/'reference.o')],['ld','-melf_i386','-T',str(p/'link.ld'),str(p/'reference.o'),'-o',str(p/'reference')],['c++','-std=c++20','-O2',str(ROOT/'tools/observe_native_campaign_camera_control_tu15.cpp'),'-o',str(p/'observer')]):
        result=subprocess.run(command,text=True,capture_output=True);assert not result.returncode,result.stderr
    return p
def inputs(image):
    rows=[]
    def make(entry,name,active=0,occupied=True,unit_none=False,parent_none=False,salt=0x1234,header_salt=None,header_type=0,highwater=8,stride=12,parent_kind=0,seat=0,flags=0,action=0x0400004a,invalid_animation=None,view=0,callback=0x245400,transition=0,matg=1,duration=0x3f000000,output=OUT,changes=(),input_changes=(),**fields):
        n=len(rows);banks={CONTROL:bytearray(4),DIRECTOR:bytearray(0x80),USERS:bytearray(0x40),PLAYERS:bytearray(0x80),OBJECTS:bytearray(0x80),PLAYER_ROWS:bytearray(4*0x21c),HEADERS:bytearray(8*12),OUT:bytearray(4),TAGS:bytearray(0x80),DEFINITION:bytearray(0x200),SEATS-0x200:bytearray(0x800),SCENARIO:bytearray(0x100),DURATIONS:bytearray(0x20),0x4f5504:bytearray(0x510),0x473990:bytearray(image.read(0x473990,20)),0x46a0d0:bytearray(image.read(0x46a0d0,4)),0x46a0e8:bytearray(image.read(0x46a0e8,4))}
        for k in range(4):banks[BASE+0x4000+k*0x800]=bytearray(0x400);banks[BASE+0x8000+k*0x800]=bytearray(0xb0)
        def put(at,value,width=4):
            raw=int(value&((1<<(width*8))-1)).to_bytes(width,'little')
            for p,b in banks.items():
                if p<=at and at+width<=p+len(b):b[at-p:at-p+width]=raw;return
            banks[at]=bytearray(raw)
        for at,value in((0x4f5504,CONTROL),(0x51df6c,DIRECTOR),(0x4f54f0,USERS),(0x4f54f4,PLAYERS),(0x4ecbd0,OBJECTS),(0x4f0414,TAGS),(0x4ecc20,matg),(0x4ecc1c,SCENARIO),(PLAYERS+0x44,PLAYER_ROWS),(OBJECTS+0x38,highwater),(OBJECTS+0x24,stride),(OBJECTS+0x44,HEADERS),(TAGS+4*16+8,DEFINITION),(DEFINITION+0x1cc,SEATS),(SCENARIO+0xec,DURATIONS)):
            put(at,value)
        for off in(8,12,16):put(DURATIONS+off,duration)
        for k in range(4):
            datum=((salt&0xffff)<<16)|k;unit=BASE+0x4000+k*0x800;parent=BASE+0x8000+k*0x800
            put(USERS+0xc+k*4,0x43210000|k if occupied else 0xffffffff)
            put(PLAYER_ROWS+k*0x21c+0x2c,0xffffffff if unit_none else datum)
            put(HEADERS+k*12,header_salt if header_salt is not None else salt,2);put(HEADERS+k*12+3,header_type,1);put(HEADERS+k*12+8,unit)
            put(HEADERS+(k+4)*12,0x5678,2);put(HEADERS+(k+4)*12+8,parent)
            put(unit+0x14,0xffffffff if parent_none else 0x56780004+k);put(unit+0x12a,0x300,2);put(unit+0x1fc,seat,2)
            put(unit+0x300,1);put(unit+0x306,0,2);put(unit+0x368,1);put(unit+0x37c,action)
            if invalid_animation is not None:put(unit+invalid_animation,0xffff if invalid_animation==0x306 else 0xffffffff,2 if invalid_animation==0x306 else 4)
            put(parent,4);put(parent+0xaa,parent_kind,1)
        put(SEATS+seat*0xb0,flags)
        for at,value,width in changes:put(at,value,width)
        gp=[0x5a010000+n*31+k*7 for k in range(8)];gp[4]=geometry.SP+n%4*4
        gp[3]=(salt&0xffff)<<16;gp[0]=view&0xffffffff;gp[2]=callback
        if entry==0x158340:argument=output
        else:argument=active if entry==0x158560 else transition
        external=[]
        for change in input_changes:
            change=dict(change)
            if change['address']=='argument':change['address']=gp[4]+4
            external.append(change)
        rows.append(dict(id=n,name=name,entry=entry,gp=gp,matrix=argument,source=0,unit=gp[3],view=view,callback=callback,output=output,banks=[dict(address=p,before=b.hex())for p,b in sorted(banks.items())],before_fp=[0x7f|((0,2,3)[n%3]<<8)|((n%4)<<10),((n%8)<<11)|((n*0x100)&0x4700)|(n*3%64),0x1f80|((n%4)<<13)|(0,0x40,0x8000,0x8040)[n%4]|(n*7%64)],live_x87=[struct.pack('<QH',0x8000000000000000+(k<<58),0x3fff+k%3|(0x8000 if k%2 else 0)).hex()for k in range((0,1,3,8)[n%4])],input_changes=external,**fields))
    for active in(0,1,2,255):
        for occupied in(False,True):make(0x158560,f'control_{active}_{occupied}',active=active,occupied=occupied)
    for entry in(0x158340,0x158560):
        for flag in(0,0x10,0x40,0x50):
            for action in(0xffffffff,0x0400004a,0x05000049,0x07000679,0x0700067a,0x0700067b,0x0700067c,0x12345678):make(entry,f'unit_mode_{entry:x}_{flag:x}_{action:x}',flags=flag,action=action)
        for salt in(0,1,0x7fff,0x8000,0xffff):make(entry,f'salt_{entry:x}_{salt:x}',salt=salt,flags=0x50)
        for hs in(0,1,0xffff):make(entry,f'wrong_salt_{entry:x}_{hs:x}',header_salt=hs)
        for kind in(0,1,2,3,31,32,33,255):make(entry,f'header_type_{entry:x}_{kind}',header_type=kind)
        for high in(0,1,4,0xffffffff):make(entry,f'highwater_{entry:x}_{high:x}',highwater=high)
        for pk in(0,1,2,31,32,33):make(entry,f'parent_type_{entry:x}_{pk}',parent_kind=pk,flags=0x50)
        for seat in(-2,-1,0,1,2):make(entry,f'signed_seat_{entry:x}_{seat}',seat=seat,flags=0x50)
        for off in(0x300,0x306,0x368):make(entry,f'invalid_animation_{entry:x}_{off:x}',invalid_animation=off,flags=0x50)
        make(entry,f'parent_none_{entry:x}',parent_none=True);make(entry,f'unit_none_{entry:x}',unit_none=True)
    for transition in(0,1,255):
        for callback in(0x245400,0x245ea0,0x16f6a0):
            for view in(0,1,3):make(0x158900,f'set_mode_{transition}_{callback:x}_{view}',transition=transition,callback=callback,view=view)
    make(0x158900,'set_mode_no_globals',transition=1,matg=0)
    for value in(0,0x80000000,1,0x80000001,0x7f800000,0xff800000,0x7fc12345,0x7f812345):
        make(0x158900,f'duration_{value:x}',transition=1,duration=value)
        make(0x158900,f'comparison_operand_{value:x}',transition=1,changes=((0x46a0e8,value,4),))
    for out in(USERS+0xc,HEADERS,BASE+0x4014,BASE+0x412a,0x473990):make(0x158340,f'mode_output_alias_{out:x}',output=out,flags=0x50,actual_read_fault=out==BASE+0x4014)
    make(0x158560,'dynamic_inactive_argument_becomes_active',input_changes=[dict(after_store=0,address='argument',bytes='01')])
    make(0x158560,'dynamic_cached_users',input_changes=[dict(after_store=0,address=0x4f54f0,bytes=(USERS+0x10).to_bytes(4,'little').hex())])
    make(0x158560,'dynamic_fresh_director',active=1,input_changes=[dict(after_store=3,address=0x51df6c,bytes=(DIRECTOR+0x20).to_bytes(4,'little').hex())])
    for entry,kwargs in((0x158340,dict(flags=0x50)),(0x158560,dict(flags=0x50)),(0x158900,dict(transition=1))):make(entry,f'fault_seed_{entry:x}',**kwargs)
    make(0x158560,'fault_seed_active',active=1)
    make(0x158340,'actual_missing_parent_body',changes=((HEADERS+4*12+8,0x40000000,4),),actual_read_fault=True)
    make(0x158560,'actual_missing_director',active=1,changes=((0x51df6c,0x40000000,4),),actual_write_fault=True)
    return rows
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--report',type=Path,required=True);p.add_argument('--verify',type=Path);a=p.parse_args();assert not a.output.exists();assert sha(NEW_DEFAULT)==NEW_PIN
    image=Xbe(NEW_DEFAULT);common.ENTRIES=ENTRIES;meta=common.metadata(image);scratch=Path(os.environ['HALO2_TEST_SCRATCH']);owned=prepare(image,meta,scratch);rows=geometry.physical(owned,inputs(image),'complete');prefix=[]
    for seed in(r for r in rows if r['name'].startswith('fault_seed')):
        for phase,events in(('before_read',seed['reads']),('before_write',seed['writes']),('after_write',seed['writes'])):
            for n in range(len(events)):
                row=copy.deepcopy(seed);row.update(id=len(rows)+len(prefix),name=seed['name']+'_'+phase+'_'+str(n),fault=dict(phase=phase,ordinal=n));prefix.append(row)
    rows+=geometry.physical(owned,prefix,'prefix');visited=set().union(*(set(r['executed_sites'])for r in rows));uncovered=[s['instruction']for s in meta['sites']if s['instruction']not in visited];assert not uncovered,uncovered
    fixture=dict(image_sha256=NEW_PIN,backend='physical-unchanged-i386-offline',owners=meta['owners'],instruction_patches=[],supplied_original_outcomes=False,cases=rows);a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_bytes(gzip.compress(json.dumps(fixture,separators=(',',':')).encode(),mtime=0));repeat=None
    if a.verify:repeat=a.output.read_bytes()==a.verify.read_bytes();assert repeat,'Physical camera control repeat differs'
    report=dict(passed=True,cases=len(rows),complete_calls=sum(r['completed']for r in rows),fault_prefixes=len(prefix),actual_source_faults=sum(bool(r.get('actual_read_fault')or r.get('actual_write_fault'))for r in rows),original_sites=len(visited),original_instructions=sum(r['original_instructions']for r in rows),ordered_reads=sum(len(r['reads'])for r in rows),ordered_stores=sum(len(r['writes'])for r in rows),owners=meta['owners'],fixture=str(a.output),fixture_bytes=a.output.stat().st_size,fixture_sha256=sha(a.output),original_sha256=NEW_PIN,repeat_identical=repeat,owned_scratch=str(scratch),retained_large_artifacts=[],source_hashes={str(Path(__file__).relative_to(ROOT)):sha(__file__),'tools/observe_native_campaign_camera_control_tu15.cpp':sha(ROOT/'tools/observe_native_campaign_camera_control_tu15.cpp')},scope_limits=['BCDF0 exact unit-mask3 profile used by158340; no arbitrary generaltype-mask ABI claimed','Numeric transport only158560 ordinary void caller; selector callbacks are published original identities, not executed or supplied samples','Ordinary disjoint private loop/mode-output scratch; original dead volatile packet results retired','Director/controller initializer/animated sampling/observer and full New Game native closure remain open'])
    a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');print(f'{len(rows)} physical camera control cases/all{len(visited)} sites; fixture {a.output.stat().st_size} bytes')
if __name__=='__main__':main()
