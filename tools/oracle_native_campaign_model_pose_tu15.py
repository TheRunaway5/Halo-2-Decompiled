#!/usr/bin/env python3
"""Physical unchanged170820 hierarchy,144BF0/144CF0 and145560 comparison.

Private affine values and the traversal queue are actual original locals;
declared banks are model inputs, poses, real output and mutable lazy metadata.
"""
import argparse,copy,gzip,json,os,struct,subprocess
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from capstone.x86_const import X86_OP_MEM
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
import oracle_native_campaign_screen_lifecycle_tu15 as common
import validate_native_geometry_tu15 as geometry
sha=common.sha
MODEL,RECORDS,POSES,DEST,FORWARD,UP,POSITION=0x1000000,0x1001000,0x1008000,0x100b000,0x1010000,0x1010020,0x1010040
def bits(x):return struct.unpack('<I',struct.pack('<f',x))[0]
def prepare(image,meta,scratch):
    p=scratch/'physical-original';p.mkdir();section=image.section('.text');(p/'text.bin').write_bytes(image.section_bytes(section));(p/'metadata.json').write_text(json.dumps(meta))
    regions={'inputs':(MODEL,0x20000),'signs':(0x45f000,4096),'constants':(0x46a000,4096),'lazy':(0x56e000,4096),'stack':(geometry.STACK,65536)}
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
    for command in (['as','--32',str(p/'reference.s'),'-o',str(p/'reference.o')],['ld','-melf_i386','-T',str(p/'link.ld'),str(p/'reference.o'),'-o',str(p/'reference')],['c++','-std=c++20','-O2',str(ROOT/'tools/observe_native_campaign_model_pose_tu15.cpp'),'-o',str(p/'observer')]):
        r=subprocess.run(command,text=True,capture_output=True);assert not r.returncode,r.stderr
    return p
def inputs(image):
    rows=[]
    def make(name,count=4,declared=None,lazy=0,destination=DEST,model=MODEL,poses=POSES,links=None,changes=(),**fields):
        n=len(rows);banks={MODEL:bytearray(0x50),RECORDS:bytearray([0x35]*max(count,1)*96),POSES:bytearray(max(count,1)*32+52),DEST:bytearray([0xa5]*max(count,1)*52),FORWARD:bytearray(96),0x56e680:bytearray(8)}
        def put(p,value,width=4):
            raw=(value&((1<<(width*8))-1)).to_bytes(width,'little')
            for first,bank in banks.items():
                if first<=p and p+width<=first+len(bank):bank[p-first:p-first+width]=raw;return
            banks[p]=bytearray(raw)
        put(MODEL+0x48,count if declared is None else declared);put(MODEL+0x4c,RECORDS)
        for p,values in ((FORWARD,(1.,.2,.1)),(UP,(.1,.2,1.)),(POSITION,(3.,5.,7.))):
            for k,v in enumerate(values):put(p+k*4,bits(v))
        for node in range(count):
            record=RECORDS+node*96;put(record+4,node-1,2);put(record+6,-1,2);put(record+8,node+1 if node+1<count else-1,2)
            for k,v in enumerate((.1+node*.03,.2-node*.01,.3,.9,1.+node,2.,3.,1.)):put(POSES+node*32+k*4,bits(v))
        for node,parent,first,second in links or():
            for off,v in ((4,parent),(8,first),(6,second)):put(RECORDS+node*96+off,v,2)
        for p,size in ((0x46a0d0,4),(0x46a0ec,4),(0x45f490,16)):banks[p]=bytearray(image.read(p,size))
        put(0x56e680,bits(.0001));put(0x56e684,lazy)
        for p,v,w in changes:put(p,v,w)
        gp=[0x5a010000+n*31+k*7 for k in range(8)];gp[0]=UP;gp[1]=FORWARD;gp[2]=POSITION;gp[4]=geometry.SP+n%4*4
        rows.append(dict(id=n,name=name,entry=0x170820,gp=gp,model=model,destination=destination,poses=poses,position=POSITION,forward=FORWARD,up=UP,banks=[dict(address=p,before=b.hex())for p,b in sorted(banks.items())],before_fp=[0x7f|((0,2,3)[n%3]<<8)|((n%4)<<10),((n%8)<<11)|((n*0x100)&0x4700)|(n*3%64),0x1f80|((n%4)<<13)|(0,0x40,0x8000,0x8040)[n%4]|(n*7%64)],live_x87=[struct.pack('<QH',0x8000000000000000+(k<<58),0x3fff+k%3|(0x8000 if k%2 else 0)).hex()for k in range((0,1,3,8)[n%4])],input_changes=[],**fields))
    for count in(0,1,2,4,8,255):
        for lazy in(0,1,2,0x80000001):make(f'nodes{count}_lazy{lazy:x}',count=count,lazy=lazy)
    for declared in(-2147483648,-1,0):make(f'nonpositive{declared}',declared=declared)
    for zero in(0,1):make(f'branched_tree{zero}',count=5,links=((0,-1,1,-1),(1,0,3,2),(2,0,-1,4),(3,1,-1,-1),(4,2,-1,-1)),changes=tuple((POSES+k*4,0,4)for k in range(4))if zero else())
    make('disconnected_records',count=5,links=((0,-1,1,-1),(1,0,-1,-1)))
    for address in(FORWARD,FORWARD+4,UP+8,POSITION,POSES,POSES+4,POSES+8,POSES+12,POSES+28,POSES+32,POSES+36,POSES+40,POSES+44,POSES+60):
        for value in(0,0x80000000,1,0x80000001,0x00800000,0x7f7fffff,0xff7fffff,0x7f800000,0xff800000,0x7fc12345,0x7f812345):make(f'fp_{address:x}_{value:x}',changes=((address,value,4),))
    for value in(0,0x7f800000,0x7fc12345):make(f'lazy_epsilon_{value:x}',lazy=1,changes=((0x56e680,value,4),))
    for destination in(POSES,POSES+4,FORWARD):make(f'output_alias_{destination:x}',count=1,destination=destination)
    make('actual_model_fault',model=0x40000000,actual_read_fault=True)
    make('actual_pose_fault',poses=0x40000000,actual_read_fault=True)
    make('actual_output_fault',destination=0x40000000,actual_write_fault=True)
    make('fault_seed_lazy',count=2,lazy=0);make('fault_seed_ready',count=2,lazy=1);make('fault_seed_zero',count=1,changes=tuple((POSES+k*4,0,4)for k in range(4)))
    make('fault_seed_packed_history',count=2,changes=tuple([(UP+4,bits(3e38),4),(UP+8,bits(-3e38),4),(POSES+40,bits(3e38),4)]+[(POSES+k*4,bits(v),4)for k,v in enumerate((.1,.4,.4,.8,0.,0.,0.,1.))]))
    return rows
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--report',type=Path,required=True);p.add_argument('--verify',type=Path);a=p.parse_args();assert not a.output.exists();assert sha(NEW_DEFAULT)==NEW_PIN
    image=Xbe(NEW_DEFAULT);common.ENTRIES=(0x170820,0x144bf0,0x144cf0,0x145560);meta=common.metadata(image);meta['required_owners']={}
    # Capstone incorrectly labels these memory destinations as reads. Their
    # unchanged machine instructions perform an eight-byte publication.
    code=_Code(image,'.text')
    for site in meta['sites']:
        instruction=code.at(site['instruction'])
        if instruction.mnemonic in ('movhps','movlps') and instruction.operands[0].type==X86_OP_MEM:
            assert len(site['operands'])==1 and site['operands'][0]['bytes']==8
            site['operands'][0].update(read=False,write=True)
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);owned=prepare(image,meta,scratch);rows=geometry.physical(owned,inputs(image),'complete');prefix=[]
    for seed in(r for r in rows if r['name'].startswith('fault_seed')):
        for phase,events in(('before_read',seed['reads']),('before_write',seed['writes']),('after_write',seed['writes'])):
            for n in range(len(events)):
                row=copy.deepcopy(seed);row.update(id=len(rows)+len(prefix),name=seed['name']+'_'+phase+'_'+str(n),fault=dict(phase=phase,ordinal=n));prefix.append(row)
    rows+=geometry.physical(owned,prefix,'prefix');visited=set().union(*(set(r['executed_sites'])for r in rows));uncovered=[s['instruction']for s in meta['sites']if s['instruction']not in visited];assert not uncovered,uncovered
    fixture=dict(image_sha256=NEW_PIN,backend='physical-unchanged-i386-offline',owners=meta['owners'],instruction_patches=[],supplied_original_outcomes=False,cases=rows);a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_bytes(gzip.compress(json.dumps(fixture,separators=(',',':')).encode(),mtime=0));repeat=None
    if a.verify:repeat=a.output.read_bytes()==a.verify.read_bytes();assert repeat,'Physical model pose repeat differs'
    report=dict(passed=True,cases=len(rows),complete_calls=sum(r['completed']for r in rows),fault_prefixes=len(prefix),actual_source_faults=3,original_sites=len(visited),uncovered_sites=uncovered,original_instructions=sum(r['original_instructions']for r in rows),ordered_reads=sum(len(r['reads'])for r in rows),ordered_stores=sum(len(r['writes'])for r in rows),owners=meta['owners'],fixture=str(a.output),fixture_bytes=a.output.stat().st_size,fixture_sha256=sha(a.output),original_sha256=NEW_PIN,repeat_identical=repeat,owned_scratch=str(scratch),retained_large_artifacts=[],source_hashes={str(Path(__file__).relative_to(ROOT)):sha(__file__),'tools/observe_native_campaign_model_pose_tu15.cpp':sha(ROOT/'tools/observe_native_campaign_model_pose_tu15.cpp')},scope_limits=['Original valid hierarchies queued within255privatecells; malformed queue overflow remains explicit capability boundary','Original masked SIMD policy; actual host RCPSS compared against unchanged original on this physical host, not physical Xbox estimate proof','Whole sky/renderer/New Game acceptance remains separate'])
    a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');print(f'{len(rows)} physical model cases/{len(visited)} sites')
if __name__=='__main__':main()
