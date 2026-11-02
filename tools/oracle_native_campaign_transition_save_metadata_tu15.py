#!/usr/bin/env python3
"""Independent unchanged physical TU1.5 transition save metadata reference.

The original four integer-only owners run physically with declared options,
controller rows and profiles. No packed metadata, callback return, participant
identity, instruction or profile-selection outcome is supplied.
"""
import argparse,copy,gzip,hashlib,json,os,struct,subprocess
from pathlib import Path
from capstone.x86_const import X86_OP_MEM
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
import oracle_native_campaign_screen_lifecycle_tu15 as common
import validate_native_geometry_tu15 as geometry

ENTRIES=(0x1acde0,0x1ad350,0x1ad300,0x1acfc0)
OPTIONS,OUTPUT,PROFILE,SOURCE,CONTROLLERS=0x1000000,0x1002000,0x1003000,0x1004000,0x55d770
SIZE,OUT_SIZE=0x1118,0x33b
sha=common.sha

def metadata(image,callers=False):
    common.ENTRIES=ENTRIES;meta=common.metadata(image);meta['required_owners']={};code=_Code(image,'.text')
    for row in meta['sites']:
        ins=code.at(row['instruction'])
        if ins.mnemonic=='test':
            for operand in row['operands']:operand.update(read=True,write=False)
        if ins.bytes.hex() in ('f3a5','f3ab'):
            row['repeat_count_register']=1
            if ins.bytes.hex()=='f3a5':row['operands'].sort(key=lambda op:not op['read'])
    assert sum(row['sites']for row in meta['owners'].values())==214
    if callers:
        registers={name:n for n,name in enumerate(('eax','ecx','edx','ebx','esp','ebp','esi','edi'))};pc=0x19178d
        while pc<0x19179e:
            ins=code.at(pc);operands=[]
            for op in ins.operands:
                if op.type!=X86_OP_MEM or ins.mnemonic=='lea':continue
                base,index=ins.reg_name(op.mem.base),ins.reg_name(op.mem.index)
                operands.append(dict(base=registers[base]if base else-1,index=registers[index]if index else-1,scale=op.mem.scale,displacement=op.mem.disp&0xffffffff,bytes=op.size,read=bool(op.access&1),write=bool(op.access&2)))
            meta['sites'].append(dict(instruction=pc,bytes=ins.bytes.hex(),operands=operands));pc+=ins.size
        assert pc==0x19179e
    return meta

def prepare(image,meta,scratch):
    p=scratch/'physical-original';p.mkdir();section=image.section('.text');(p/'text.bin').write_bytes(image.section_bytes(section));(p/'metadata.json').write_text(json.dumps(meta))
    regions={'inputs':(0xff0000,0x20000),'controllers':(0x55d000,0x4000),'stack':(geometry.STACK,65536)}
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
    for command in (['as','--32',str(p/'reference.s'),'-o',str(p/'reference.o')],['ld','-melf_i386','-T',str(p/'link.ld'),str(p/'reference.o'),'-o',str(p/'reference')],['c++','-std=c++20','-O2',str(ROOT/'tools/observe_native_campaign_transition_save_metadata_tu15.cpp'),'-o',str(p/'observer')]):
        result=subprocess.run(command,text=True,capture_output=True);assert not result.returncode,result.stderr
    return p

def inputs():
    rows=[]
    def make(name,entry=0x1acde0,flags=15,selectors=15,options=OPTIONS,destination=OUTPUT,source=PROFILE,private=False,changes=(),byte_changes=(),**fields):
        n=len(rows);banks={}
        def bank(at,size,fill=0):
            lo,hi=at,at+size;overlap=[(p,b)for p,b in banks.items()if p<hi and lo<p+len(b)]
            if overlap:lo=min(lo,*(p for p,b in overlap));hi=max(hi,*(p+len(b)for p,b in overlap))
            data=bytearray([fill]*(hi-lo))
            for p,b in overlap:data[p-lo:p-lo+len(b)]=b;del banks[p]
            banks[lo]=data
        def raw(at,value):
            assert at+len(value)<=0x100000000
            if not any(p<=at and at+len(value)<=p+len(b)for p,b in banks.items()):bank(at,len(value))
            for p,b in banks.items():
                if p<=at and at+len(value)<=p+len(b):b[at-p:at-p+len(value)]=value;return
        def put(at,value,width=4):raw(at,(value&((1<<(width*8))-1)).to_bytes(width,'little'))
        sp=geometry.SP-0x4000+(n%4)*4;opts=sp+0x350 if private else options;dest=sp+0x10 if private else destination
        option_value=bytes((k*29+n*11+0x31)&255 for k in range(SIZE));output_value=bytes((k*31+n*7+0x53)&255 for k in range(OUT_SIZE))
        if not private:raw(opts,option_value);raw(dest,output_value)
        raw(PROFILE,bytes((k*17+n*13+0x25)&255 for k in range(0x1e0)));raw(SOURCE,bytes((k*23+n*7+0x43)&255 for k in range(32)))
        for slot in range(4):
            row=CONTROLLERS+slot*0xc74;raw(row,bytes((k*19+slot*37+n*3+0x11)&255 for k in range(0xc74)))
            put(row,(0x10 if flags&(1<<slot)else 0)|(slot*3),1);put(row+0x1f8,slot if selectors&(1<<slot)else 0xffffffff)
        if entry in(0x1ad300,0x1acfc0)and source==PROFILE:source=SOURCE
        for at,value,width in changes:put(at,value,width)
        for at,value in byte_changes:raw(at,value)
        gp=[0x5b000000+n*37+k*11 for k in range(8)];gp[4]=sp;gp[0]=dest;gp[1]=source;gp[2]=opts;gp[3]=dest
        depth=n%9
        row=dict(id=n,name=name,entry=entry,options=opts,destination=dest,source=source,gp=gp,private_options=private,private_output=private,options_value=option_value.hex(),output_before=output_value.hex(),banks=[dict(address=p,before=b.hex())for p,b in sorted(banks.items())],before_fp=[0x7f|((0,2,3)[n%3]<<8)|((n%4)<<10),((n%8)<<11)|((n*0x100)&0x4700)|(n*3%64),0x1f80|((n%4)<<13)|(0,0x40,0x8000,0x8040)[n%4]|(n*7%64)],live_x87=[struct.pack('<QH',0x8000000000000000+(k<<58),(0x3fff+k%3)|(0x8000 if k%2 else 0)).hex()for k in range(depth)],input_changes=[],**fields)
        rows.append(row);return row
    for flags in range(16):
        for selectors in(0,15):make(f'root_flags_{flags:x}_selectors_{selectors:x}',flags=flags,selectors=selectors)
    for selectors in(1,2,4,8,5,10):make(f'root_mixed_selectors_{selectors:x}',selectors=selectors)
    for flags in(0,1,5,15):make(f'private_{flags:x}',private=True,flags=flags,selectors=10)
    for offset in(1,2,3):make(f'root_alignment_{offset}',options=OPTIONS+offset,destination=OUTPUT+offset)
    for entry in(0x1ad350,0x1ad300,0x1acfc0):
        for absent in(False,True):
            for offset in(0,1,2,3):make(f'direct_{entry:x}_{absent}_{offset}',entry=entry,source=0 if absent else PROFILE if entry==0x1ad350 else SOURCE,destination=OUTPUT+offset)
    for dest in(OPTIONS+8,OPTIONS+0x20,OPTIONS-0x20,CONTROLLERS+0x10,CONTROLLERS-0x11b):make(f'root_alias_{dest:x}',destination=dest,selectors=5)
    for dest in(PROFILE+4,PROFILE+8,PROFILE+0x98,PROFILE+0xc0):make(f'profile_alias_{dest:x}',entry=0x1ad350,destination=dest)
    for entry in(0x1ad300,0x1acfc0):
        for dest in(SOURCE,SOURCE-4,SOURCE+4,SOURCE+5,SOURCE-8):make(f'leaf_alias_{entry:x}_{dest:x}',entry=entry,destination=dest)
    for entry in ENTRIES:
        row=make(f'actual_source_fault_{entry:x}',entry=entry,options=0x40000000 if entry==0x1acde0 else OPTIONS,source=0x40000000,actual_read_fault=True)
        row['banks']=[b for b in row['banks']if b['address']!=0x40000000]
        row=make(f'actual_output_fault_{entry:x}',entry=entry,destination=0x40000000,actual_write_fault=True)
        row['banks']=[b for b in row['banks']if b['address']!=0x40000000]
    for flags,name in((0,'default'),(15,'active')):make('fault_seed_root_'+name,flags=flags)
    make('fault_seed_root_private',private=True,selectors=5)
    make('fault_seed_root_alias',destination=CONTROLLERS+0x10,selectors=5)
    make('fault_seed_profile',entry=0x1ad350)
    make('fault_seed_profile_default',entry=0x1ad350,source=0)
    make('fault_seed_profile_alias',entry=0x1ad350,destination=PROFILE+0x98)
    for entry in(0x1ad300,0x1acfc0):
        make('fault_seed_leaf_'+hex(entry),entry=entry)
        make('fault_seed_leaf_default_'+hex(entry),entry=entry,source=0)
        make('fault_seed_leaf_alias_'+hex(entry),entry=entry,destination=SOURCE+5)
    return rows

def base_case(seed):
    excluded={'completed','boundary','after_fp','live_x87_unchanged','pending_x87_temporaries','options_after','output_after','callee_entries','boundary_gp','result_bits','reads','writes','banks','executed_sites','original_instructions','physical_verified'}
    result={k:copy.deepcopy(v)for k,v in seed.items()if k not in excluded};result['banks']=[dict(address=b['address'],before=b['before'])for b in seed['banks']];return result

def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path);parser.add_argument('--report',type=Path,required=True);parser.add_argument('--complete-only',action='store_true');parser.add_argument('--verify',type=Path);parser.add_argument('--callers',action='store_true');args=parser.parse_args();assert sha(NEW_DEFAULT)==NEW_PIN
    assert args.callers or(args.output and not args.output.exists())
    image=Xbe(NEW_DEFAULT);meta=metadata(image,args.callers);scratch=Path(os.environ['HALO2_TEST_SCRATCH']);owned=prepare(image,meta,scratch);declared=inputs()
    if args.callers:
        selected=[base_case(next(row for row in declared if row['name']==name))for name in('private_0','private_1','private_5','private_f')]
        for depth in range(9):
            row=copy.deepcopy(selected[-1]);row['name']='caller_live_'+str(depth);row['before_fp'][1]=(depth%8<<11)|0x241;row['live_x87']=[struct.pack('<QH',0x8000000000000000+(n<<58),0x3fff+n%3).hex()for n in range(depth)];selected.append(row)
        for index,row in enumerate(selected):row.update(id=index,entry=0x19178d,caller=True,stop_address=0x19179e)
        rows=geometry.physical(owned,selected,'actual-callers');packets=[]
        for row in rows:
            assert row['completed']and row['live_x87_unchanged']and row['before_fp']==row['after_fp'];assert row['options_value']==row['options_after'];assert len(row['callee_entries'])==1
            entry=row['callee_entries'][0];sp=row['gp'][4];assert entry['gp'][2]==sp+0x350 and entry['gp'][4]==sp-8;assert struct.unpack('<II',bytes.fromhex(entry['stack_hex']))==(0x19179e,sp+0x10);assert row['boundary_gp'][4]==sp
            for n in(3,5,6,7):assert row['gp'][n]==row['boundary_gp'][n]
            packets.append(dict(name=row['name'],initial_gp=row['gp'],callee_entry=entry,returned_gp=row['boundary_gp'],before_fp=row['before_fp'],after_fp=row['after_fp'],older_values=len(row['live_x87']),older_values_preserved=True,options_unchanged=True,output_sha256=hashlib.sha256(bytes.fromhex(row['output_after'])).hexdigest(),original_instructions=row['original_instructions']))
        report=dict(passed=True,scope='Actual19178D..19179E original private options/metadata CALL191799 andfull214-siteclosure; stops before unrelated logging.',original_sha256=NEW_PIN,profiles=len(rows),actual_calls=len(rows),original_instructions=sum(r['original_instructions']for r in rows),packets=packets,instruction_patches=[],supplied_original_outcomes=False)
    else:
        rows=geometry.physical(owned,declared,'complete');prefix=[]
        if not args.complete_only:
            for seed in(row for row in rows if row['name'].startswith('fault_seed')):
                for phase,events in(('before_read',seed['reads']),('before_write',seed['writes']),('after_write',seed['writes'])):
                    groups={}
                    for index,event in enumerate(events):groups.setdefault(event['instruction'],[]).append(index)
                    cuts=set()
                    for indices in groups.values():cuts.update(indices if len(indices)<=6 else(indices[0],indices[1],indices[len(indices)//2],indices[-2],indices[-1]))
                    for ordinal in sorted(cuts):
                        row=base_case(seed);row.update(id=len(rows)+len(prefix),name=seed['name']+'_'+phase+'_'+str(ordinal),fault=dict(phase=phase,ordinal=ordinal));prefix.append(row)
            for start in range(0,len(prefix),32):rows+=geometry.physical(owned,prefix[start:start+32],'prefix-'+str(start))
        visited=set().union(*(set(row['executed_sites'])for row in rows));payloads={}
        for row in rows:
            assert row['before_fp']==row['after_fp']and row['live_x87_unchanged']
            if row['private_options']:assert row['options_after']==row['options_value']
            for bank in row['banks']:
                for phase in('before','after'):
                    raw=bank.pop(phase);digest=hashlib.sha256(bytes.fromhex(raw)).hexdigest();assert digest not in payloads or payloads[digest]==raw;payloads[digest]=raw;bank[phase+'_ref']=digest
        fixture=dict(image_sha256=NEW_PIN,backend='physical-unchanged-i386-offline',owners=meta['owners'],instruction_patches=[],supplied_original_outcomes=False,bank_format='physical-complete-bank-intern-v1',bank_data=payloads,cases=rows)
        args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_bytes(gzip.compress(json.dumps(fixture,separators=(',',':')).encode(),mtime=0));repeat=None
        if args.verify:repeat=args.output.read_bytes()==args.verify.read_bytes();assert repeat,'Independent save metadata original repeat differs'
        report=dict(passed=True,scope=__doc__,owners=meta['owners'],cases=len(rows),complete_calls=sum(row['completed']for row in rows),fault_prefixes=len(prefix),actual_faults=sum(row.get('actual_read_fault',False)or row.get('actual_write_fault',False)for row in rows),original_sites=len(visited),uncovered_sites=[hex(row['instruction'])for row in meta['sites']if row['instruction']not in visited],original_instructions=sum(row['original_instructions']for row in rows),ordered_reads=sum(len(row['reads'])for row in rows),ordered_stores=sum(len(row['writes'])for row in rows),fixture=str(args.output),fixture_bytes=args.output.stat().st_size,fixture_sha256=sha(args.output),repeat_identical=repeat)
    report.update(original_sha256=NEW_PIN,source_hashes={str(Path(__file__).relative_to(ROOT)):sha(__file__),'tools/observe_native_campaign_transition_save_metadata_tu15.cpp':sha(ROOT/'tools/observe_native_campaign_transition_save_metadata_tu15.cpp')},owned_scratch=str(scratch),retained_large_artifacts=[],limits=['OrdinaryDF0 retained RAM and disjoint private options/output frame.','Original execution offline-only; no broader191750 logging/profile persistence/platform/session behavior claimed.'])
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:v for k,v in report.items()if k in('cases','complete_calls','original_sites','uncovered_sites','fixture_bytes','profiles','actual_calls')}))

if __name__=='__main__':main()
