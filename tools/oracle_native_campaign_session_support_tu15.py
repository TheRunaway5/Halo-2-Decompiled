#!/usr/bin/env python3
"""Independent physical TU1.5 call-free session support reference.

Original606A0/7E7F0/599B0 execute unchanged with declared retained inputs.
No callbacks, session success, resources, clocks or return values are supplied.
"""
import argparse,copy,gzip,hashlib,json,os,struct,subprocess
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
import oracle_native_campaign_screen_lifecycle_tu15 as common
import validate_native_geometry_tu15 as geometry

ENTRIES=(0x606a0,0x7e7f0,0x599b0)
SESSION,CHANNELS,DESCRIPTOR=0x1000000,0x1010000,0x1020000
sha=common.sha

def metadata(image,callers=False):
    common.ENTRIES=ENTRIES;meta=common.metadata(image);meta['required_owners']={};code=_Code(image,'.text')
    for row in meta['sites']:
        if code.at(row['instruction']).mnemonic in('cmp','test'):
            for op in row['operands']:op.update(read=True,write=False)
    assert sum(row['sites']for row in meta['owners'].values())==102
    if callers:
        registers={name:n for n,name in enumerate(('eax','ecx','edx','ebx','esp','ebp','esi','edi'))}
        for first,last in((0x5b204,0x5b212),(0x5b0c1,0x5b0c8),(0x79d0e,0x79d16)):
            pc=first
            while pc<last:
                ins=code.at(pc);operands=[]
                for op in ins.operands:
                    if op.type!=3 or ins.mnemonic=='lea':continue
                    base,index=ins.reg_name(op.mem.base),ins.reg_name(op.mem.index)
                    operands.append(dict(base=registers[base]if base else-1,index=registers[index]if index else-1,scale=op.mem.scale,displacement=op.mem.disp&0xffffffff,bytes=op.size,read=bool(op.access&1),write=bool(op.access&2)))
                if ins.mnemonic in('cmp','test'):
                    for op in operands:op.update(read=True,write=False)
                meta['sites'].append(dict(instruction=pc,bytes=ins.bytes.hex(),operands=operands));pc+=ins.size
            assert pc==last
    return meta

def prepare(image,meta,scratch):
    p=scratch/'physical-original';p.mkdir();section=image.section('.text');(p/'text.bin').write_bytes(image.section_bytes(section));(p/'metadata.json').write_text(json.dumps(meta))
    regions={'inputs':(0xff0000,0x40000),'stack':(geometry.STACK,65536)}
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
    (p/'reference.s').write_text(assembly);(p/'link.ld').write_text(f'ENTRY(_start)\nSECTIONS{{.original 0x{section.va:x}:{{*(.original)}} '+''.join(f'.{name} 0x{address:x}:{{*(.{name})}} 'for name,(address,_)in regions.items())+f'.stop 0x{geometry.STOP:x}:{{*(.stop)}} .bridge 0x70000000:{{*(.bridge)}}}}\n')
    for command in (['as','--32',str(p/'reference.s'),'-o',str(p/'reference.o')],['ld','-melf_i386','-T',str(p/'link.ld'),str(p/'reference.o'),'-o',str(p/'reference')],['c++','-std=c++20','-O2',str(ROOT/'tools/observe_native_campaign_session_support_tu15.cpp'),'-o',str(p/'observer')]):
        result=subprocess.run(command,text=True,capture_output=True);assert not result.returncode,result.stderr
    return p

def inputs():
    rows=[]
    def make(name,entry=0x606a0,phase=5,flag=1,peer=2,shift=1,link=1,channels=CHANNELS,descriptor=DESCRIPTOR,kind=16,word=None,first=0,selected=(1,1,1),scan=(),session=None,changes=(),**fields):
        n=len(rows);session=SESSION+(n%4)if session is None else session;banks={}
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
        bank(session,0x78b8,0x53);bank(CHANNELS,0x2000,0xa5);bank(DESCRIPTOR,32)
        put(session+8,channels);put(session+0x10,0xa5b60000|shift);put(session+0x741c,phase);put(session+0x7420,flag,1)
        for slot in range(16):
            row=session+0x72dc+slot*20
            for k in range(3):put(row+k,0,1)
            put(row+4,0xffffffff)
        selected_row=(session+0x72dc+peer*20)&0xffffffff
        for k,value in enumerate(selected):put(selected_row+k,value,1)
        put(selected_row+4,link)
        for slot,flags in scan:
            for k,value in enumerate(flags):put(session+0x72dc+slot*20+k,value,1)
        if channels==CHANNELS and link!=0xffffffff:put((channels+link*0x550+0xb1)&0xffffffff,0xff,1)
        for k in range(8):put(DESCRIPTOR+k*2,0,2)
        put(DESCRIPTOR+0x10,0xbeef,2);put(DESCRIPTOR+0x12,kind,2)
        if word is not None:put(DESCRIPTOR+word*2,0x8001,2)
        if kind in(-1,4):put(DESCRIPTOR,first)
        for at,value,width in changes:put(at,value,width)
        gp=[0x5d000000+n*31+k*11 for k in range(8)];gp[4]=geometry.SP+(n%4)*4;gp[7]=session;gp[2]=peer if entry==0x606a0 else descriptor if entry==0x7e7f0 else session;depth=n%9
        rows.append(dict(id=n,name=name,entry=entry,session=session,peer_index=peer,descriptor=descriptor,options=0,gp=gp,banks=[dict(address=p,before=b.hex())for p,b in sorted(banks.items())],before_fp=[0x7f|((0,2,3)[n%3]<<8)|((n%4)<<10),((n%8)<<11)|((n*0x100)&0x4700)|(n*3%64),0x1f80|((n%4)<<13)|(0,0x40,0x8000,0x8040)[n%4]|(n*7%64)],live_x87=[struct.pack('<QH',0x8000000000000000+(k<<58),(0x3fff+k%3)|(0x8000 if k%2 else 0)).hex()for k in range(depth)],input_changes=[],**fields));return rows[-1]
    for shift in range(256):make('shift_'+str(shift),shift=shift)
    for phase in(-0x80000000,-2,-1,0,1,2,3,4,5,6,7,8,9,0x7fffffff):make('clear_phase_'+str(phase),phase=phase)
    for peer in range(16):make('clear_peer_'+str(peer),peer=peer)
    for peer in(0xffffffff,0xcccccccd):make('clear_wrapped_peer_'+hex(peer),peer=peer)
    for link in(0xffffffff,0,2,0x10000000,0x80000001):make('clear_link_'+hex(link),link=link)
    for selected in((1,0,1),(1,1,0),(0,1,1)):make('selected_flags_'+str(selected),selected=selected)
    for slot in(0,1,3,15):make('scan_stop_'+str(slot),scan=((slot,(1,1,1)),))
    make('scan_lazy_flags',scan=((0,(0,1,1)),(1,(1,0,1)),(3,(1,1,0)),(15,(1,1,1))))
    for phase in(7,8):
        for flag in range(256):make(f'policy_{phase}_{flag}',entry=0x599b0,phase=phase,flag=flag)
    for phase in(-0x80000000,-2,-1,0,1,2,3,4,5,6,9,0x7fffffff):make('policy_phase_'+str(phase),entry=0x599b0,phase=phase,flag=0xff)
    for kind in(-0x8000,-2,-1,0,1,3,4,5,15,16,17,0x7fff):
        for first in(0,1,0x80000000):make(f'address_kind_{kind}_{first:x}',entry=0x7e7f0,kind=kind,first=first)
    for word in range(8):make('address_word_'+str(word),entry=0x7e7f0,word=word)
    make('address_null',entry=0x7e7f0,descriptor=0)
    for offset in(1,2,3):
        row=make('address_alignment_'+str(offset),entry=0x7e7f0);row['descriptor']+=offset;row['gp'][2]+=offset
    for target in('row_tail','session_pointer','phase'):
        session=SESSION;peer=2;record=session+0x72dc+peer*20
        at=record+0x10 if target=='row_tail'else session+8 if target=='session_pointer'else session+0x741c
        make('clear_alias_'+target,session=session,peer=peer,link=0,shift=0,channels=(at-0xb1)&0xffffffff)
    for entry in ENTRIES:
        row=make('actual_root_fault_'+hex(entry),entry=entry,actual_read_fault=True)
        row['gp'][7 if entry==0x606a0 else 2]=0x40000000
        if entry!=0x7e7f0:row['session']=0x40000000
        else:row['descriptor']=0x40000000
    make('actual_channel_fault',channels=0x40000000,actual_read_fault=True)
    make('none_link_unreadable_channel',channels=0x40000000,link=0xffffffff)
    make('fault_seed_clear',scan=((0,(0,1,1)),(1,(1,0,1)),(3,(1,1,0)),(15,(1,1,1))))
    make('fault_seed_none',link=0xffffffff)
    make('fault_seed_phase3',phase=3)
    make('fault_seed_alias',session=SESSION,channels=SESSION+0x741c-0xb1,link=0,shift=0)
    make('fault_seed_address',entry=0x7e7f0)
    make('fault_seed_address4',entry=0x7e7f0,kind=4,first=1)
    make('fault_seed_policy',entry=0x599b0,phase=7,flag=0xff)
    return rows

def base_case(seed):
    excluded={'completed','boundary','after_fp','live_x87_unchanged','pending_x87_temporaries','callee_entries','boundary_gp','result_bits','reads','writes','banks','executed_sites','original_instructions','physical_verified'}
    row={k:copy.deepcopy(value)for k,value in seed.items()if k not in excluded};row['banks']=[dict(address=b['address'],before=b['before'])for b in seed['banks']];return row

def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path);parser.add_argument('--report',type=Path,required=True);parser.add_argument('--complete-only',action='store_true');parser.add_argument('--verify',type=Path);parser.add_argument('--callers',action='store_true');args=parser.parse_args();assert sha(NEW_DEFAULT)==NEW_PIN
    assert args.callers or(args.output and not args.output.exists())
    image=Xbe(NEW_DEFAULT);meta=metadata(image,args.callers);scratch=Path(os.environ['HALO2_TEST_SCRATCH']);owned=prepare(image,meta,scratch);declared=inputs()
    if args.callers:
        names=['clear_peer_0','clear_peer_2','clear_peer_15','clear_phase_3','clear_phase_8','clear_link_0xffffffff','policy_7_0','policy_7_2','policy_8_255','policy_phase_2','policy_phase_3','address_kind_4_1','address_kind_-1_0','address_word_7']
        selected=[base_case(next(row for row in declared if row['name']==name))for name in names]
        for depth in range(9):
            row=copy.deepcopy(selected[1]);row['name']='caller_live_'+str(depth);row['before_fp'][1]=(depth%8<<11)|0x241;row['live_x87']=[struct.pack('<QH',0x8000000000000000+(n<<58),0x3fff+n%3).hex()for n in range(depth)];selected.append(row)
        for index,row in enumerate(selected):
            entry=row['entry'];row.update(id=index,caller=True)
            if entry==0x606a0:row['gp'][6]=row['session'];row['gp'][5]=row['peer_index'];row['gp'][3]=(row['session']+0x72dc+row['peer_index']*20)&0xffffffff;row.update(entry=0x5b204,stop_address=0x5b212)
            elif entry==0x599b0:
                row['gp'][3]=row['session'];at=row['session']+0x741c
                bank=next(b for b in row['banks']if b['address']<=at and at+4<=b['address']+len(b['before'])//2)
                row['gp'][6]=int.from_bytes(bytes.fromhex(bank['before'])[at-bank['address']:at-bank['address']+4],'little')
                row.update(entry=0x5b0c1,stop_address=0x5b0c8)
            else:row['gp'][6]=(row['descriptor']-0x5c)&0xffffffff;row.update(entry=0x79d0e,stop_address=0x79d16)
        rows=geometry.physical(owned,selected,'actual-callers');packets=[]
        for row in rows:
            assert row['completed']and row['live_x87_unchanged']and row['before_fp']==row['after_fp'];assert len(row['callee_entries'])==1
            entry=row['callee_entries'][0];sp=row['gp'][4];assert entry['gp'][4]==sp-4;assert struct.unpack('<I',bytes.fromhex(entry['stack_hex']))==(row['stop_address'],);assert row['boundary_gp'][4]==sp
            for n in(3,5,6,7):assert row['boundary_gp'][n]==entry['gp'][n]
            packets.append(dict(name=row['name'],initial_gp=row['gp'],session=row['session'],peer_index=row['peer_index'],descriptor=row['descriptor'],callee_entry=entry,returned_gp=row['boundary_gp'],returned_al=row['boundary_gp'][0]&255,before_fp=row['before_fp'],after_fp=row['after_fp'],older_values=len(row['live_x87']),older_values_preserved=True,original_instructions=row['original_instructions']))
        report=dict(passed=True,scope='Actual5B20D/5B0C3/79D11 CALLs and trueRET before caller continuation. No other parent lifecycle executed.',profiles=len(rows),actual_calls=len(rows),original_instructions=sum(row['original_instructions']for row in rows),packets=packets,instruction_patches=[],supplied_original_outcomes=False)
    else:
        rows=geometry.physical(owned,declared,'complete');prefix=[]
        if not args.complete_only:
            for seed in(row for row in rows if row['name'].startswith('fault_seed')):
                for phase,events in(('before_read',seed['reads']),('before_write',seed['writes']),('after_write',seed['writes'])):
                    for ordinal in range(len(events)):
                        row=base_case(seed);row.update(id=len(rows)+len(prefix),name=seed['name']+'_'+phase+'_'+str(ordinal),fault=dict(phase=phase,ordinal=ordinal));prefix.append(row)
            for start in range(0,len(prefix),32):rows+=geometry.physical(owned,prefix[start:start+32],'prefix-'+str(start))
        visited=set().union(*(set(row['executed_sites'])for row in rows));assert len(visited)==102
        payloads={}
        for row in rows:
            assert row['before_fp']==row['after_fp']and row['live_x87_unchanged']
            for bank in row['banks']:
                for phase in('before','after'):
                    raw=bank.pop(phase);digest=hashlib.sha256(bytes.fromhex(raw)).hexdigest();assert digest not in payloads or payloads[digest]==raw;payloads[digest]=raw;bank[phase+'_ref']=digest
        fixture=dict(image_sha256=NEW_PIN,backend='physical-unchanged-i386-offline',owners=meta['owners'],instruction_patches=[],supplied_original_outcomes=False,bank_format='physical-complete-bank-intern-v1',bank_data=payloads,cases=rows)
        args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_bytes(gzip.compress(json.dumps(fixture,separators=(',',':')).encode(),mtime=0));repeat=None
        if args.verify:repeat=args.output.read_bytes()==args.verify.read_bytes();assert repeat,'Independent original session-support repeat differs'
        report=dict(passed=True,scope=__doc__,owners=meta['owners'],cases=len(rows),complete_calls=sum(row['completed']for row in rows),fault_prefixes=len(prefix),actual_faults=sum(row.get('actual_read_fault',False)for row in rows),original_sites=len(visited),uncovered_sites=[],original_instructions=sum(row['original_instructions']for row in rows),ordered_reads=sum(len(row['reads'])for row in rows),ordered_stores=sum(len(row['writes'])for row in rows),fixture=str(args.output),fixture_bytes=args.output.stat().st_size,fixture_sha256=sha(args.output),repeat_identical=repeat)
    report.update(original_sha256=NEW_PIN,source_hashes={str(Path(__file__).relative_to(ROOT)):sha(__file__),'tools/observe_native_campaign_session_support_tu15.cpp':sha(ROOT/'tools/observe_native_campaign_session_support_tu15.cpp')},owned_scratch=str(scratch),retained_large_artifacts=[],limits=['Ordinary retained RAM/disjoint private caller stack only; no device-memory or private-frame aliases.','Code-table599D4 is authenticated executable dispatch encoding, not an emulated retained data dependency.','No parent callback, resource, clock, session-success or full completion outcome supplied.'])
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({key:value for key,value in report.items()if key in('cases','complete_calls','original_sites','fixture_bytes','profiles','actual_calls')}))

if __name__=='__main__':main()
