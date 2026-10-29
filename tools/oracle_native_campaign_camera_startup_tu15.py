#!/usr/bin/env python3
"""Physical unchanged TU1.5 CRC and real camera storage constructors, no supplied callees."""
import argparse,copy,gzip,json,os,struct,subprocess
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
import oracle_native_campaign_screen_lifecycle_tu15 as common
import validate_native_geometry_tu15 as geometry
sha=common.sha
ENTRIES=(0x166a80,0x166ae0,0x1888a0,0x157e10,0x13e9e0,0x381710,0x381730,0x381750)
BASE,INPUT,CHECKSUM,TABLE=0x1000000,0x1004000,0x1005000,0x1006000
def prepare(image,meta,scratch):
    p=scratch/'physical-original';p.mkdir();section=image.section('.text');(p/'text.bin').write_bytes(image.section_bytes(section));(p/'metadata.json').write_text(json.dumps(meta))
    regions={'data':(BASE,0x10000),'globals':(0x4e9000,0x89000),'constants':(0x46a000,0x1000),'stack':(geometry.STACK,65536)}
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
    for command in (['as','--32',str(p/'reference.s'),'-o',str(p/'reference.o')],['ld','-melf_i386','-T',str(p/'link.ld'),str(p/'reference.o'),'-o',str(p/'reference')],['c++','-std=c++20','-O2',str(ROOT/'tools/observe_native_campaign_camera_startup_tu15.cpp'),'-o',str(p/'observer')]):
        r=subprocess.run(command,text=True,capture_output=True);assert not r.returncode,r.stderr
    return p
def inputs(image):
    rows=[]
    def make(entry,name,count=9,flag=1,checksum=CHECKSUM,input=INPUT,table=TABLE,offset=0x100,base=BASE,changes=(),input_changes=(),**fields):
        n=len(rows);banks={BASE:bytearray((i*13+n*7+3)&255 for i in range(0x2000)),INPUT:bytearray((i*13+n*7+3)&255 for i in range(0x400)),CHECKSUM:bytearray(4),TABLE:bytearray((i*13+n*7+3)&255 for i in range(0x404)),0x4f2950:bytearray(0x100),0x4f5504:bytearray((i*7+n*3)&255 for i in range(0x510)),0x4f9b54:bytearray(4),0x51df50:bytearray(4),0x51df6c:bytearray(4),0x56e225:bytearray([flag]+[0]*0x32),0x56e258:bytearray((i*11+n*5+17)&255 for i in range(1024)),0x55d6e4:bytearray(12)}
        def put(at,value,width=4):
            raw=int(value&((1<<(width*8))-1)).to_bytes(width,'little')
            for p,b in banks.items():
                if p<=at and at+width<=p+len(b):b[at-p:at-p+width]=raw;return
            banks[at]=bytearray(raw)
        for p,source in((0x46ab88,0x46ab88),(0x46a164,0x46a164),(0x46a5f8,0x46a5f8)):banks[p]=bytearray(image.read(source,4))
        put(0x4f2950,base);put(0x4f2954,offset);put(0x4f295c,0x12345678+n*7)
        if checksum!=0x40000000:put(checksum,0x9abcdef0+n*11)
        for p,value,width in changes:put(p,value,width)
        gp=[0x5a010000+n*31+k*7 for k in range(8)];gp[4]=geometry.SP+n%4*4
        if entry==0x166a80:gp[0]=input;gp[7]=count&0xffffffff
        if entry==0x166ae0:gp[2]=table
        rows.append(dict(id=n,name=name,entry=entry,gp=gp,matrix=checksum,source=0,count=count,input=input,checksum=checksum,table=table,banks=[dict(address=p,before=b.hex())for p,b in sorted(banks.items())],before_fp=[0x7f|((0,2,3)[n%3]<<8)|((n%4)<<10),((n%8)<<11)|((n*0x100)&0x4700)|(n*3%64),0x1f80|((n%4)<<13)|(0,0x40,0x8000,0x8040)[n%4]|(n*7%64)],live_x87=[struct.pack('<QH',0x8000000000000000+(k<<58),0x3fff+k%3|(0x8000 if k%2 else 0)).hex()for k in range((0,1,3,8)[n%4])],input_changes=list(input_changes),**fields))
    for flag in(0,1,2,255):
        for count in(-2147483648,-1,0,1,4,9,31,256):make(0x166a80,f'crc_{flag}_{count}',flag=flag,count=count)
    for flag in(0,1):
        for inp in(INPUT,0x56e258,0x56e225,0x4f2950):
            for output in(CHECKSUM,INPUT,0x56e258,0x4f2950):make(0x166a80,f'crc_alias_{flag}_{inp:x}_{output:x}',flag=flag,count=9,input=inp,checksum=output)
    for p in(TABLE,TABLE+1,0x56e258,INPUT,0x4f5504):make(0x166ae0,f'table_{p:x}',table=p)
    for entry in(0x1888a0,0x157e10,0x13e9e0):
        for flag in(0,1,255):
            for offset in(0,1,0x100,0x1000):make(entry,f'constructor_{entry:x}_{flag}_{offset}',flag=flag,offset=offset)
        for base,offset in((0xfffffff0,0x1000110),(0x4f2900,0x50),(0x4f5504,0),(0x56e200,0x58)):
            make(entry,f'constructor_alias_{entry:x}_{base:x}_{offset:x}',base=base,offset=offset)
    for entry in(0x381710,0x381730,0x381750):
        for value in(0,0x80000000,1,0x7f800000,0x7fc12345,0x7f812345):
            source={0x381710:0x46ab88,0x381730:0x46a164,0x381750:0x46a5f8}[entry]
            make(entry,f'default_{entry:x}_{value:x}',changes=((source,value,4),))
    make(0x157e10,'dynamic_fresh_second_allocation',input_changes=[dict(after_store=325,address=0x4f2950,bytes=(BASE+0x1000).to_bytes(4,'little').hex()),dict(after_store=325,address=0x4f2954,bytes=(0x200).to_bytes(4,'little').hex())])
    make(0x157e10,'dynamic_cached_second_base',input_changes=[dict(after_store=326,address=0x4f2950,bytes=(BASE+0x2000).to_bytes(4,'little').hex())])
    make(0x166a80,'dynamic_table_after_input_checksum_init',input_changes=[dict(after_store=0,address=0x56e258+0x44,bytes=(0x76543210).to_bytes(4,'little').hex())],flag=0)
    for entry in(0x166a80,0x166ae0,0x1888a0,0x157e10,0x13e9e0):make(entry,f'fault_seed_{entry:x}',flag=1,count=4)
    make(0x166a80,'fault_seed_crc_cold',flag=0,count=4)
    make(0x166a80,'actual_crc_unmapped_input',input=0x40000000,actual_read_fault=True)
    make(0x166a80,'actual_crc_unmapped_checksum',checksum=0x40000000,actual_read_fault=True)
    make(0x166ae0,'actual_crc_unmapped_table',table=0x40000000,actual_write_fault=True)
    return rows
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--report',type=Path,required=True);p.add_argument('--verify',type=Path);a=p.parse_args();assert not a.output.exists();assert sha(NEW_DEFAULT)==NEW_PIN
    image=Xbe(NEW_DEFAULT);common.ENTRIES=ENTRIES;meta=common.metadata(image);scratch=Path(os.environ['HALO2_TEST_SCRATCH']);owned=prepare(image,meta,scratch);rows=geometry.physical(owned,inputs(image),'complete');prefix=[]
    for seed in(r for r in rows if r['name'].startswith('fault_seed')):
        for phase,events in(('before_read',seed['reads']),('before_write',seed['writes']),('after_write',seed['writes'])):
            indices=range(len(events))if phase=='before_read' or len(events)<35 else sorted({0,1,7,127,254,255,256,322,323,324,325,326,len(events)-1}&set(range(len(events))))
            for n in indices:
                row=copy.deepcopy(seed);row.update(id=len(rows)+len(prefix),name=seed['name']+'_'+phase+'_'+str(n),fault=dict(phase=phase,ordinal=n));prefix.append(row)
    rows+=geometry.physical(owned,prefix,'prefix');visited=set().union(*(set(r['executed_sites'])for r in rows));uncovered=[s['instruction']for s in meta['sites']if s['instruction']not in visited];assert not uncovered,uncovered
    fixture=dict(image_sha256=NEW_PIN,backend='physical-unchanged-i386-offline',owners=meta['owners'],instruction_patches=[],supplied_original_outcomes=False,cases=rows);a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_bytes(gzip.compress(json.dumps(fixture,separators=(',',':')).encode(),mtime=0));repeat=None
    if a.verify:repeat=a.output.read_bytes()==a.verify.read_bytes();assert repeat,'Physical camera startup repeat differs'
    report=dict(passed=True,cases=len(rows),complete_calls=sum(r['completed']for r in rows),fault_prefixes=len(prefix),actual_source_faults=3,original_sites=len(visited),original_instructions=sum(r['original_instructions']for r in rows),ordered_reads=sum(len(r['reads'])for r in rows),ordered_stores=sum(len(r['writes'])for r in rows),owners=meta['owners'],fixture=str(a.output),fixture_bytes=a.output.stat().st_size,fixture_sha256=sha(a.output),original_sha256=NEW_PIN,repeat_identical=repeat,owned_scratch=str(scratch),retained_large_artifacts=[],source_hashes={str(Path(__file__).relative_to(ROOT)):sha(__file__),'tools/observe_native_campaign_camera_startup_tu15.cpp':sha(ROOT/'tools/observe_native_campaign_camera_startup_tu15.cpp')},scope_limits=['Constructor native private size input is the authenticated four-byte value, not guest stack','CRC core uses actual writable polynomial table and signed count, without host CRC substitution','Controller initialization/control/animation/observer and full New Game native closure remain open'])
    a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');print(f'{len(rows)} physical camera startup cases/all{len(visited)} sites; fixture {a.output.stat().st_size} bytes')
if __name__=='__main__':main()
