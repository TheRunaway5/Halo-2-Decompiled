#!/usr/bin/env python3
"""Independent physical154F80/155190/1550B0/CE340 participant preparation.

Every identifier, pool, unit, item and tag is a declared original input. No
participant ID, equipment outcome, callee return or instruction is supplied.
"""
import argparse,copy,gzip,hashlib,json,os,struct,subprocess
from pathlib import Path
from capstone.x86_const import X86_OP_MEM
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
import oracle_native_campaign_screen_lifecycle_tu15 as common
import validate_native_geometry_tu15 as geometry

ENTRIES=(0x154f80,0x155190,0x1550b0,0xce340)
OPTIONS,GLOBALS,PLAYERS,BITMAP,PROWS,GAME,SCENARIO,SROWS=(0x1000000,0x1010000,0x1011000,0x1012000,0x1020000,0x1030000,0x1040000,0x1041000)
OBJECTS,OROWS,BODIES,REGISTRY,TAGS,BLOCKS,REFS,CLOCK,OUTPUT=(0x1050000,0x1051000,0x1060000,0x1070000,0x1080000,0x1090000,0x10a0000,0x10b0000,0x10c0000)
NONE=0xffffffff;SIZE=0x1118;sha=common.sha
def datum(n):return 0xe2000000|n
def identity(n):return bytes((0x41+n*7+k*11)&255 for k in range(12))
def metadata(image):
    common.ENTRIES=ENTRIES;meta=common.metadata(image);meta['required_owners']={};code=_Code(image,'.text')
    for row in meta['sites']:
        ins=code.at(row['instruction'])
        if row['instruction'] in(0x155011,0x155221):
            assert ins.mnemonic=='test' and ins.bytes[0]==0x85
            assert len(row['operands'])==1 and row['operands'][0]['bytes']==4
            row['operands'][0].update(read=True,write=False)
        if ins.bytes.hex()in('f3a5','f3ab','f3a6'):
            row['repeat_count_register']=1
            if ins.bytes.hex()=='f3a5':row['operands'].sort(key=lambda o:not o['read'])
            if ins.bytes.hex()=='f3a6':row['operands'].sort(key=lambda o:o['base']!=6)
    assert sum(r['sites']for r in meta['owners'].values())==430
    return meta
def prepare(image,meta,scratch):
    p=scratch/'physical-original';p.mkdir();section=image.section('.text');(p/'text.bin').write_bytes(image.section_bytes(section));(p/'metadata.json').write_text(json.dumps(meta))
    regions={'inputs':(0x1000000,0x100000),'one':(0x46a000,4096),'hundred':(0x450000,4096),'objects':(0x4ec000,4096),'registry':(0x4f0000,4096),'game':(0x4f3000,4096),'players':(0x4f5000,4096),'clock':(0x51d000,4096),'stack':(geometry.STACK,65536)}
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
    for cmd in(['as','--32',str(p/'reference.s'),'-o',str(p/'reference.o')],['ld','-melf_i386','-T',str(p/'link.ld'),str(p/'reference.o'),'-o',str(p/'reference')],['c++','-std=c++20','-O2',str(ROOT/'tools/observe_native_campaign_transition_participants_tu15.cpp'),'-o',str(p/'observer')]):
        r=subprocess.run(cmd,text=True,capture_output=True);assert not r.returncode,r.stderr
    return p
def inputs(image):
    rows=[]
    def make(name,entry=0x155190,players=1,occupied=None,scenario=0,timed=False,private=False,options=OPTIONS,destination=OUTPUT,changes=(),byte_changes=(),health=0x3e800000,**fields):
        n=len(rows);banks={}
        def bank(at,size,fill=0):
            lo,hi=at,at+size;overlap=[(p,b)for p,b in banks.items()if p<hi and lo<p+len(b)]
            if overlap:lo=min(lo,*(p for p,b in overlap));hi=max(hi,*(p+len(b)for p,b in overlap))
            data=bytearray([fill]*(hi-lo))
            for p,b in overlap:data[p-lo:p-lo+len(b)]=b;del banks[p]
            banks[lo]=data
        def put(at,value,width=4):
            raw=(value&((1<<(width*8))-1)).to_bytes(width,'little')
            for p,b in banks.items():
                if p<=at and at+width<=p+len(b):b[at-p:at-p+width]=raw;return
            bank(at,width);put(at,value,width)
        def raw(at,value):
            if not any(p<=at and at+len(value)<=p+len(b)for p,b in banks.items()):bank(at,len(value))
            for k,v in enumerate(value):put(at+k,v,1)
        sp=geometry.SP-0x4000+(n%4)*4;at=sp+0x350 if private else options
        for p,size,fill in((GLOBALS,0x100,0x57),(PLAYERS,76,0),(BITMAP,4,0),(PROWS,20*0x21c,0x39),(GAME,0x2000,0x63),(SCENARIO,0x108,0),(SROWS,0x40,0),(OBJECTS,76,0),(OROWS,24*12,0),(BODIES,24*0x300,0),(REGISTRY,24*16,0),(TAGS,24*0x400,0),(BLOCKS,24*0xa0,0),(REFS,24*0x200,0),(CLOCK,16,0),(OUTPUT,64,0xa5)):
            bank(p,size,fill)
        if not private:bank(at,SIZE,0xa5)
        raw_options=bytearray((k*29+17)&255 for k in range(SIZE))
        for slot in range(16):raw_options[0x2e2+slot*0xe4:0x2ee+slot*0xe4]=identity(slot)
        if not private:raw(at,raw_options)
        for p,v in((0x4f54f0,GLOBALS),(0x4f54f4,PLAYERS),(0x4f3218,GAME),(0x4ecc20,SCENARIO),(0x4ecbd0,OBJECTS),(0x4f0414,REGISTRY),(0x51df54,CLOCK),(PLAYERS+0x20,20),(PLAYERS+0x24,0x21c),(PLAYERS+0x38,players),(PLAYERS+0x44,PROWS),(PLAYERS+0x48,BITMAP),(OBJECTS+0x24,12),(OBJECTS+0x44,OROWS),(BITMAP,((1<<players)-1)if occupied is None else occupied),(SCENARIO+0x100,1),(SCENARIO+0x104,SROWS),(CLOCK+8,1000)):put(p,v)
        put(SROWS+0x2c,scenario,2)
        for slot in range(16):raw(GAME+0x388+slot*0xe4,bytes((slot*17+k*23+0x31)&255 for k in range(56)))
        for index in range(20):
            player=PROWS+index*0x21c;put(player,0xe300+index,2);put(player+2,index*3,1);raw(player+4,identity(index));put(player+0x2c,datum(index))
        for index in range(24):
            body=BODIES+index*0x300;tag=TAGS+index*0x400;put(OROWS+index*12+8,body);put(body,index);put(REGISTRY+index*16+8,tag);put(tag+0x1fc,0x20+index,1);put(tag+0x2c0,1);put(tag+0x2d0,0);put(tag+0x2d4,BLOCKS+index*0xa0)
            put(body+0x184,health);put(body+0x1fc,0 if timed else NONE,2);put(body+0x212,0,1);put(body+0x213,1,1)
            for slot in range(4):put(body+0x218+slot*4,datum(20+slot));put(body+0x228+slot*4,900+slot*11)
            put(body+0x23e,0x40+index,1);put(body+0x23f,0x60+index,1)
        for index in range(20,24):put(BODIES+index*0x300+0x22a,0x311+index,2);put(BODIES+index*0x300+0x22c,0x412+index,2)
        for p,size in((0x46a0d0,4),(0x4500b8,4)):raw(p,image.read(p,size))
        for p,v,w in changes:put(p,v,w)
        for p,data in byte_changes:raw(p,data)
        gp=[0x5a000000+n*37+k*11 for k in range(8)];gp[4]=sp;gp[0]=at if entry==0x154f80 else datum(20)if entry==0x1550b0 else datum(0);gp[3]=destination
        depth=n%8;rows.append(dict(id=n,name=name,entry=entry,options=at,item=datum(20),destination=destination,unit=datum(0),gp=gp,private_options=private,options_value=raw_options.hex(),banks=[dict(address=p,before=b.hex())for p,b in sorted(banks.items())],before_fp=[0x7f|((0,2,3)[n%3]<<8)|((n%4)<<10),((n%8)<<11)|((n*0x100)&0x4700)|(n*3%64),0x1f80|((n%4)<<13)|(0,0x40,0x8000,0x8040)[n%4]|(n*7%64)],live_x87=[struct.pack('<QH',0x8000000000000000+(k<<58),(0x3fff+k%3)|(0x8000 if k%2 else 0)).hex()for k in range(depth)],input_changes=[],**fields))
        return rows[-1]
    for players in(0,1,2,4,16,17):
        for private in(False,True):make(f'capture_{players}_{private}',entry=0x154f80,players=players,private=private)
    for players in(0,1,2,16,17):
        for scenario in(0,1,2):
            for timed in(False,True):make(f'prepare_{players}_{scenario}_{timed}',players=players,scenario=scenario,timed=timed)
    for private in(False,True):make(f'private_prepare_{private}',private=private,scenario=1,timed=True)
    for changes,name in((((PROWS+0x2c,NONE,4),),'no_unit'),(((PROWS+0x2c,NONE,4),(PROWS+0x21c+0x2c,NONE,4)),'two_no_units')):make(name,players=2,changes=changes)
    make('last_identity_match',byte_changes=((OPTIONS+0x2e2+15*0xe4,identity(0)),))
    make('identity_no_match',byte_changes=((PROWS+4,b'\xff'*12),))
    make('sparse_players',players=17,occupied=(1<<2)|(1<<16))
    make('capture_sparse_players',entry=0x154f80,players=17,occupied=(1<<2)|(1<<16))
    for first,second in((-1,-1),(0,0),(3,2),(7,-1)):
        for slots in((NONE,)*4,(datum(20),NONE,datum(22),NONE)):
            changes=((BODIES+0x212,first,1),(BODIES+0x213,second,1))+tuple((BODIES+0x218+i*4,v,4)for i,v in enumerate(slots))
            make(f'spare_{first}_{second}_{slots[0]:x}',entry=0xce340,changes=changes)
            if first<=3 and second<=3:make(f'normal_slots_{first}_{second}_{slots[0]:x}',changes=changes)
    for times,name in(((1000,1000,1000,1000),'ties'),((0,0xffffffff,0x80000000,0x7fffffff),'wrapped'),((999,1,998,2),'ordering')):
        make('timed_'+name,timed=True,changes=tuple((BODIES+0x228+i*4,v,4)for i,v in enumerate(times)))
    make('timed_duplicate_datum',timed=True,changes=((BODIES+0x21c,datum(20),4),(BODIES+0x228,999,4),(BODIES+0x22c,998,4),(BODIES+0x230,900,4),(BODIES+0x234,900,4)))
    item=BODIES+20*0x300;tag=TAGS+20*0x400;block=BLOCKS+20*0xa0
    for count in(-1,0,1):
        for ref,alternate,kind in((NONE,NONE,0),(NONE,4,0),(NONE,4,17),(21,NONE,0),(21,NONE,18)):
            make(f'item_fallback_{count}_{ref:x}_{alternate:x}_{kind}',entry=0x1550b0,changes=((tag+0x2d0,count,4),(block+0x90,ref,4),(block+0x98,alternate,4),(block+0x88,kind,1),(TAGS+21*0x400+0x128,kind,1)))
    for optional in(-1,0,1):make(f'item_optional_{optional}',entry=0x1550b0,changes=((tag+0x2c0,optional,4),))
    for value in(0,0x80000000,1,0x80000001,0x00800000,0x3f800000,0x40000000,0xbf800000,0x3eaaaaab,0x3f000001,0x7f7fffff,0xff7fffff,0x7f800000,0xff800000,0x7fc12345,0x7f812345):
        for mode in range(4):
            r=make(f'item_fp_{value:x}_{mode}',entry=0x1550b0,health=value);r['before_fp'][0]=(r['before_fp'][0]&~0xc00)|(mode<<10);r['before_fp'][2]=(r['before_fp'][2]&~0x6000)|(mode<<13)
    for destination in(item+0x184,item+0x22a,tag+0x2c0-6):make(f'item_alias_{destination:x}',entry=0x1550b0,destination=destination)
    make('prepare_alias_current_game',options=GAME+12)
    make('capture_alias_globals',entry=0x154f80,options=GLOBALS-0x238)
    make('capture_alias_player_rows',entry=0x154f80,options=PROWS-0x2d4)
    for entry in ENTRIES:
        make(f'actual_player_or_object_fault_{entry:x}',entry=entry,changes=(((0x4f54f4 if entry in(0x154f80,0x155190) else 0x4ecbd0),0x40000000,4),),actual_read_fault=True)
    failed=make('actual_capture_output_fault',entry=0x154f80,options=0x40000000,actual_write_fault=True)
    failed['banks']=[b for b in failed['banks']if b['address']!=0x40000000]
    make('actual_item_output_fault',entry=0x1550b0,destination=0x40000000,actual_write_fault=True)
    for entry,name in((0x154f80,'capture'),(0x155190,'prepare'),(0x1550b0,'item'),(0xce340,'spare')):make('fault_seed_'+name,entry=entry)
    make('fault_seed_private_prepare',private=True,timed=True)
    make('fault_seed_fractional_item',entry=0x1550b0,health=0x3eaaaaab)
    make('fault_seed_private_capture',entry=0x154f80,private=True)
    make('fault_seed_prepare_alias',options=GAME+12)
    return rows
def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True);parser.add_argument('--report',type=Path,required=True);parser.add_argument('--complete-only',action='store_true');parser.add_argument('--verify',type=Path);a=parser.parse_args();assert not a.output.exists();assert sha(NEW_DEFAULT)==NEW_PIN
    image=Xbe(NEW_DEFAULT);meta=metadata(image);scratch=Path(os.environ['HALO2_TEST_SCRATCH']);owned=prepare(image,meta,scratch);rows=geometry.physical(owned,inputs(image),'complete');prefix=[]
    if not a.complete_only:
        for seed in(r for r in rows if r['name'].startswith('fault_seed')):
            for phase,events in(('before_read',seed['reads']),('before_write',seed['writes']),('after_write',seed['writes'])):
                by_site={}
                for index,event in enumerate(events):by_site.setdefault(event['instruction'],[]).append(index)
                ordinals=set()
                for site,indices in by_site.items():
                    ordinals.update(indices if len(indices)<=6 else(indices[0],indices[1],indices[len(indices)//2],indices[-2],indices[-1]))
                for index in sorted(ordinals):
                    excluded={'completed','boundary','after_fp','live_x87_unchanged','pending_x87_temporaries','options_after','boundary_gp','result_bits','reads','writes','banks','executed_sites','original_instructions','physical_verified'}
                    row={k:copy.deepcopy(v)for k,v in seed.items()if k not in excluded};row['banks']=[dict(address=b['address'],before=b['before'])for b in seed['banks']]
                    row.update(id=len(rows)+len(prefix),name=seed['name']+'_'+phase+'_'+str(index),fault=dict(phase=phase,ordinal=index));prefix.append(row)
        for start in range(0,len(prefix),32):rows+=geometry.physical(owned,prefix[start:start+32],'prefix-'+str(start))
    visited=set().union(*(set(r['executed_sites'])for r in rows));payloads={}
    for row in rows:
        for bank in row['banks']:
            for phase in('before','after'):
                raw=bank.pop(phase);digest=hashlib.sha256(bytes.fromhex(raw)).hexdigest();assert digest not in payloads or payloads[digest]==raw;payloads[digest]=raw;bank[phase+'_ref']=digest
    fixture=dict(image_sha256=NEW_PIN,backend='physical-unchanged-i386-offline',owners=meta['owners'],instruction_patches=[],supplied_original_outcomes=False,bank_format='physical-complete-bank-intern-v1',bank_data=payloads,cases=rows)
    a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_bytes(gzip.compress(json.dumps(fixture,separators=(',',':')).encode(),mtime=0));repeat=None
    if a.verify:repeat=a.output.read_bytes()==a.verify.read_bytes();assert repeat,'Independent participant reference repeat differs'
    report=dict(passed=True,scope=__doc__,owners=meta['owners'],cases=len(rows),complete_calls=sum(r['completed']for r in rows),fault_prefixes=len(prefix),original_sites=len(visited),uncovered_sites=[hex(s['instruction'])for s in meta['sites']if s['instruction']not in visited],original_instructions=sum(r['original_instructions']for r in rows),ordered_reads=sum(len(r['reads'])for r in rows),ordered_stores=sum(len(r['writes'])for r in rows),fixture=str(a.output),fixture_bytes=a.output.stat().st_size,fixture_sha256=sha(a.output),original_sha256=NEW_PIN,repeat_identical=repeat,source_hashes={str(Path(__file__).relative_to(ROOT)):sha(__file__),'tools/observe_native_campaign_transition_participants_tu15.cpp':sha(ROOT/'tools/observe_native_campaign_transition_participants_tu15.cpp')},owned_scratch=str(scratch),retained_large_artifacts=[],scope_limits=['OrdinaryDF0 string direction','Opaque12-byte identities supplied only as declared source inputs, never generated runtime IDs','No network/profile/session outcome or full191750 completion claim'])
    a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(dict(cases=len(rows),complete=report['complete_calls'],sites=len(visited),uncovered=report['uncovered_sites'],fixture_bytes=report['fixture_bytes'])))
if __name__=='__main__':main()
