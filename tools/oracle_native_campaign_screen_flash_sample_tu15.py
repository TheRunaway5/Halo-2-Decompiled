#!/usr/bin/env python3
"""Physical unchanged TU1.5 screen flash/global fade and finite curve owners.

All execution is an offline original reference. Declared banks/registers are
inputs; no original instruction, dependency result or stopped prefix is changed.
"""
import argparse,copy,gzip,json,os,struct,subprocess
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
import oracle_native_campaign_screen_lifecycle_tu15 as common
import validate_native_geometry_tu15 as geometry
sha=common.sha
BASE,BANK,CLOCK,SCENE,COLOR,DEST=0x1000000,0x1000100,0x1000800,0x1000900,0x1000a00,0x1000b00
ENTRIES=(0x156e40,0x17f8a0,0x37dc0a,0x37d46c,0x331ecb,0x37d536,0x37dc14,0x331f68)
def bits(x):return struct.unpack('<I',struct.pack('<f',x))[0]
def metadata(image):
    common.ENTRIES=ENTRIES;meta=common.metadata(image)
    for site in meta['sites']:
        if site['instruction'] in (0x331f31,0x331f3e):
            assert site['bytes']=='d7'
            site['operands']=[dict(base=3,index=0,index_mask=255,scale=1,displacement=0,bytes=1,read=True,write=False)]
    methods={int.from_bytes(image.read(0x48c8dc+n*4,4),'little')for n in range(16)}
    meta['required_owners']={str(p):True for p in methods-{0x37dc14,0x331f68}|{0x3334f0}}
    return meta
def prepare(image,meta,scratch):
    p=scratch/'physical-original';p.mkdir();section=image.section('.text');(p/'text.bin').write_bytes(image.section_bytes(section));(p/'metadata.json').write_text(json.dumps(meta))
    regions={'inputs':(BASE,0x10000),'constants':(0x46a000,0x5000),'tables':(0x470000,0x20000),'globals':(0x4e9000,0x37000),'stack':(geometry.STACK,65536)}
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
    for command in (['as','--32',str(p/'reference.s'),'-o',str(p/'reference.o')],['ld','-melf_i386','-T',str(p/'link.ld'),str(p/'reference.o'),'-o',str(p/'reference')],['c++','-std=c++20','-O2',str(ROOT/'tools/observe_native_campaign_screen_flash_sample_tu15.cpp'),'-o',str(p/'observer')]):
        r=subprocess.run(command,text=True,capture_output=True);assert not r.returncode,r.stderr
    return p
def inputs(image):
    rows=[]
    def make(name,global_ticks=-1,user=0,remaining=15,fresh=0,curve=0,duration=30.,intensity=.6,destination=DEST,alternate=0,scene_sign=0,changes=(),**fields):
        n=len(rows);data=bytearray((k*13+n*7+3)&255 for k in range(0x1000));banks={BASE:data,0x4f54f8:bytearray([0xa5]*24)}
        def put(p,value,width=4):
            raw=(value&((1<<(8*width))-1)).to_bytes(width,'little')
            if BASE<=p and p+width<=BASE+len(data):data[p-BASE:p-BASE+width]=raw
            else:
                for start,bank in banks.items():
                    if start<=p and p+width<=start+len(bank):bank[p-start:p-start+width]=raw;return
                banks[p]=bytearray(raw)
        put(0x51df5c,BANK);put(0x51df54,CLOCK);put(0x51df60,alternate,1)
        put(BANK,10);put(BANK+4,global_ticks,2);put(BANK+6,1,1);put(CLOCK+8,26)
        put(0x4ecc20,SCENE);put(SCENE+0x12,scene_sign,1)
        put(0x474c70,COLOR);put(0x474c78,COLOR+0x20)
        for p in (0x4f54f8,COLOR,COLOR+0x20):
            for k,v in enumerate((.2,.4,.6)):put(p+k*4,bits(v+(0.1 if p==COLOR else 0)))
        slot=BANK+0x30+0xa0*(user if user in range(4)else 0)
        put(slot+0x18,1,2);put(slot+0x1c,bits(duration));put(slot+0x20,curve,2);put(slot+0x24,bits(intensity));put(slot+0x7e,remaining,2);put(slot+0x88,fresh,1)
        for k,v in enumerate((.7,.2,.3,.4)):put(slot+0x28+k*4,bits(v))
        for p,size in ((0x46a0d0,4),(0x46a0c4,4),(0x46a178,4),(0x46a1d0,8),(0x46a43c,4),(0x46ecd0,4),(0x46ecb4,16),(0x48c8cc,80),(0x48d4f4,4),(0x47524c,16)):
            banks[p]=bytearray(image.read(p,size))
        # Round-down interpolation can read the preceding sample. These exact
        # adjacent original bytes are inputs, not an invented guard sample.
        selected=5 if global_ticks>0 else curve
        if selected:
            at=(0x4782a8+selected*1024)&0xffffffff
            if 0x470001<=at<0x48fc00:banks[at-1]=bytearray(image.read(at-1,1026))
        for p,v,w in changes:put(p,v,w)
        gp=[0x5a010000+n*31+k*7 for k in range(8)];gp[0]=user&0xffffffff;gp[4]=geometry.SP+n%4*4;gp[6]=destination
        depth=(0,1,3,5)[n%4]
        rows.append(dict(id=n,name=name,entry=0x156e40,gp=gp,matrix=0,source=0,user=user&0xffffffff,destination=destination,
            banks=[dict(address=p,before=b.hex())for p,b in sorted(banks.items())],
            before_fp=[0x7f|((0,2,3)[n%3]<<8)|((n%4)<<10),((n%8)<<11)|((n*0x100)&0x4700)|(n*3%64),0x1f80|((n%4)<<13)|(0,0x40,0x8000,0x8040)[n%4]|(n*7%64)],
            live_x87=[struct.pack('<QH',0x8000000000000000+(k<<58),0x3fff+k%3|(0x8000 if k%2 else 0)).hex()for k in range(depth)],input_changes=[],**fields))
    for ticks in(-32768,-2,0,1,16,48,32767):
        for alternate,sign in((0,0),(1,0),(1,128)):
            for incoming in(0,1):make(f'global_{ticks}_{alternate}_{sign}_{incoming}',global_ticks=ticks,alternate=alternate,scene_sign=sign,changes=((BANK+6,incoming,1),))
    for user in(-1,0,1,2,3):
        for remaining in(-32768,-1,0,1,30,32767):
            for fresh in(0,1,2):make(f'local_user{user}_ticks{remaining}_fresh{fresh}',user=user,remaining=remaining,fresh=fresh)
    for curve in range(16):
        for remaining in(0,1,15,29,30):make(f'curve{curve}_ticks{remaining}',curve=curve,remaining=remaining,fresh=1)
    for duration in(0.,-1.,.1,1.,32767.):
        for amount in(-2.,0.,.5,1.,2.):make(f'duration{duration}_intensity{amount}',duration=duration,intensity=amount,fresh=1)
    for value in(0x80000000,1,0x80000001,0x00800000,0x7f7fffff,0xff7fffff,0x7f800000,0xff800000,0x7fc12345,0x7f812345):
        make(f'intensity_bits{value:x}',duration=0.,changes=((BANK+0x30+0x24,value,4),))
        make(f'duration_bits{value:x}',curve=1,changes=((BANK+0x30+0x1c,value,4),))
    for p in(BANK,BANK+4,BANK+0x30+0x18,BANK+0x30+0x1c,BANK+0x30+0x24,BANK+0x30+0x28,COLOR,0x4f54f8):
        make(f'local_alias_{p:x}',destination=p);make(f'global_alias_{p:x}',destination=p,global_ticks=48)
    for ticks in(0,1,10,40,0xffffffff):make(f'global_clock{ticks}',global_ticks=30,changes=((CLOCK+8,ticks,4),))
    for curve in(0,1,5):
        for value in(0x3f000000,0x3f800001):make(f'live_one_{curve}_{value:x}',curve=curve,changes=((0x46a0d0,value,4),))
    for policy in(0,0x2694):
        for errors in(0,1):
            make(f'crt_policy_{policy:x}_{errors}',curve=1,changes=((0x46ecd0,policy,4),(0x48d4f4,errors,4)))
    make('actual_missing_bank',changes=((0x51df5c,0x40000000,4),),actual_read_fault=True)
    make('actual_missing_output',destination=0x40000000,actual_write_fault=True)
    for name,kwargs in [('global',dict(global_ticks=48)),('local',dict(curve=1)),('zero',dict(curve=1,remaining=0,fresh=1)),('alternate',dict(global_ticks=0,alternate=1,scene_sign=128)),('inactive',dict(remaining=0))]:make('fault_seed_'+name,**kwargs)
    make('fault_seed_crt_legacy',curve=1,changes=((0x46ecd0,0,4),))
    return rows
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--report',type=Path,required=True);p.add_argument('--verify',type=Path);a=p.parse_args();assert not a.output.exists();assert sha(NEW_DEFAULT)==NEW_PIN
    image=Xbe(NEW_DEFAULT);meta=metadata(image);scratch=Path(os.environ['HALO2_TEST_SCRATCH']);owned=prepare(image,meta,scratch);rows=geometry.physical(owned,inputs(image),'complete');prefix=[]
    for seed in(r for r in rows if r['name'].startswith('fault_seed')):
        for phase,events in(('before_read',seed['reads']),('before_write',seed['writes']),('after_write',seed['writes'])):
            for n in range(len(events)):
                row=copy.deepcopy(seed);row.update(id=len(rows)+len(prefix),name=seed['name']+'_'+phase+'_'+str(n),fault=dict(phase=phase,ordinal=n));prefix.append(row)
    rows+=geometry.physical(owned,prefix,'prefix');visited=set().union(*(set(r['executed_sites'])for r in rows));uncovered=[s['instruction']for s in meta['sites']if s['instruction']not in visited]
    fixture=dict(image_sha256=NEW_PIN,backend='physical-unchanged-i386-offline',owners=meta['owners'],instruction_patches=[],supplied_original_outcomes=False,cases=rows);a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_bytes(gzip.compress(json.dumps(fixture,separators=(',',':')).encode(),mtime=0));repeat=None
    if a.verify:repeat=a.output.read_bytes()==a.verify.read_bytes();assert repeat,'Physical screen flash repeat differs'
    report=dict(passed=True,cases=len(rows),complete_calls=sum(r['completed']for r in rows),fault_prefixes=len(prefix),actual_source_faults=2,original_sites=len(visited),uncovered_sites=uncovered,original_instructions=sum(r['original_instructions']for r in rows),ordered_reads=sum(len(r['reads'])for r in rows),ordered_stores=sum(len(r['writes'])for r in rows),owners=meta['owners'],fixture=str(a.output),fixture_bytes=a.output.stat().st_size,fixture_sha256=sha(a.output),original_sha256=NEW_PIN,repeat_identical=repeat,owned_scratch=str(scratch),retained_large_artifacts=[],source_hashes={str(Path(__file__).relative_to(ROOT)):sha(__file__),'tools/observe_native_campaign_screen_flash_sample_tu15.cpp':sha(ROOT/'tools/observe_native_campaign_screen_flash_sample_tu15.cpp')},scope_limits=['Masked FP exceptions and at most five older live original x87 values','CRT nonfinite descriptor handlers and genuine3334F0 matherror remain required owners','Private curve values use hardware numerical policy, not an instruction/register emulator','Full graphical intro and Armory handoff remain separate acceptance'])
    a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');print(f'{len(rows)} physical flash cases/{len(visited)} admitted sites')
if __name__=='__main__':main()
