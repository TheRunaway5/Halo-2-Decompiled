#!/usr/bin/env python3
"""Physical unchanged TU1.5 initialized CRT thread context and random draw.

Actual i386 Linux TLS setup supplies the declared FS base. The original CALLs
to unavailable allocation/fatal owners retire physically and stop at their
real callee entry. No allocation, fatal return, seed or random result is given.
"""
import argparse,copy,gzip,hashlib,json,os,struct,subprocess
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
import oracle_native_campaign_screen_lifecycle_tu15 as common
import validate_native_geometry_tu15 as geometry

ENTRIES=(0x32c075,0x330179)
FS,THREAD,TABLE,SLOT,DATA=0x1000000,0x1001000,0x1002000,0x1003000,0x1004000
GLOBAL,INDEX,IAT=0x573840,0x48c920,0x41b758
GATED=(0x3301c9,0x3301cb,0x3301cd,0x3301ce,0x3301cf,0x3301d1,0x3301d3,0x3301da,0x3301e1,0x3301e6,0x3301ea,0x3301ec,0x3301ee,0x3301f0,0x3301f5)
sha=common.sha

def metadata(image,callers=False):
    common.ENTRIES=ENTRIES;meta=common.metadata(image);meta['required_owners']={};meta['tls_descriptor']=0x70001000;code=_Code(image,'.text')
    for row in meta['sites']:
        ins=code.at(row['instruction']);mem=[op for op in ins.operands if op.type==3 and ins.mnemonic!='lea']
        assert len(mem)==len(row['operands'])
        for op,record in zip(mem,row['operands']):record['fs']=bool(op.mem.segment and ins.reg_name(op.mem.segment)=='fs')
        if ins.mnemonic in('cmp','test'):
            for record in row['operands']:record.update(read=True,write=False)
    assert sum(owner['sites']for owner in meta['owners'].values())==50
    if callers:
        ins=code.at(0x19182a);assert ins.bytes.hex()=='e846a81900'
        meta['sites'].append(dict(instruction=ins.address,bytes=ins.bytes.hex(),operands=[]))
    return meta

def prepare(image,meta,scratch):
    p=scratch/'physical-original';p.mkdir();section=image.section('.text');(p/'text.bin').write_bytes(image.section_bytes(section));(p/'metadata.json').write_text(json.dumps(meta))
    regions={'inputs':(0xff0000,0x20000),'global':(0x573000,0x1000),'index':(0x48c000,0x1000),'iat':(0x41b000,0x1000),'stack':(geometry.STACK,65536)}
    assembly=f'.code32\n.section .original,"ax"\n.incbin "{p/"text.bin"}"\n'
    for name,(_,size)in regions.items():assembly+=f'.section .{name},"aw"\n.space {size}\n'
    assembly+='''.section .stop,"ax"
int3
.section .bridge_data,"aw"
.long -1,0x1000000,0xfffff,0x51
.section .bridge,"ax"
.globl _start
_start:
mov $243,%eax
mov $0x70001000,%ebx
int $0x80
test %eax,%eax
js failed
mov 0x70001000,%eax
shl $3,%eax
or $3,%eax
mov %ax,%fs
fninit
int3
jmp _start
failed:
int3
jmp failed
'''
    (p/'reference.s').write_text(assembly);(p/'link.ld').write_text(f'ENTRY(_start)\nSECTIONS{{.original 0x{section.va:x}:{{*(.original)}} '+''.join(f'.{name} 0x{address:x}:{{*(.{name})}} 'for name,(address,_)in regions.items())+f'.stop 0x{geometry.STOP:x}:{{*(.stop)}} .bridge 0x70000000:{{*(.bridge)}} .bridge_data 0x70001000:{{*(.bridge_data)}}}}\n')
    for command in (['as','--32',str(p/'reference.s'),'-o',str(p/'reference.o')],['ld','-melf_i386','-T',str(p/'link.ld'),str(p/'reference.o'),'-o',str(p/'reference')],['c++','-std=c++20','-O2',str(ROOT/'tools/observe_native_tu15_crt_random.cpp'),'-o',str(p/'observer')]):
        result=subprocess.run(command,text=True,capture_output=True);assert not result.returncode,result.stderr
    return p

def inputs(image):
    rows=[]
    def make(name,entry=0x32c075,tls=False,context=DATA,seed=1,level=0,index=1,fs=None,thread=THREAD,table=TABLE,slot=SLOT,changes=(),**fields):
        n=len(rows);fs=FS+(n%4)if fs is None else fs;banks={}
        def bank(at,size,fill=0):
            lo,hi=at,at+size;overlap=[(p,b)for p,b in banks.items()if p<hi and lo<p+len(b)]
            if overlap:lo=min(lo,*(p for p,b in overlap));hi=max(hi,*(p+len(b)for p,b in overlap))
            raw=bytearray([fill]*(hi-lo))
            for p,b in overlap:raw[p-lo:p-lo+len(b)]=b;del banks[p]
            banks[lo]=raw
        def put(at,value,width=4):
            raw=(value&((1<<(width*8))-1)).to_bytes(width,'little')
            for p,b in banks.items():
                if p<=at and at+width<=p+len(b):b[at-p:at-p+width]=raw;return
            bank(at,width);put(at,value,width)
        bank(fs,0x30);bank(THREAD,0x130);bank(TABLE-4,0x110);bank(SLOT,0x40)
        if context and context<0x40000000:bank(context,0x88,0x53)
        for at,value,width in ((fs+4,table,4),(fs+0x24,level,1),(fs+0x28,thread,4),(THREAD+0x28,int(tls),4),(THREAD+0x12c,0x12345678,4),(INDEX,index,4),(GLOBAL,context,4),((TABLE+index*4)&0xffffffff,slot,4),(SLOT+0xc,context,4),(IAT,struct.unpack('<I',image.read(IAT,4))[0],4)):put(at,value,width)
        if context and context<0x40000000:put(context+0x14,seed)
        for at,value,width in changes:put(at,value,width)
        gp=[0x5c000000+n*31+k*11 for k in range(8)];gp[4]=geometry.SP+(n%4)*4;depth=n%9
        row=dict(id=n,name=name,entry=entry,fs_base=fs,options=0,gp=gp,banks=[dict(address=p,before=b.hex())for p,b in sorted(banks.items())],before_fp=[0x7f|((0,2,3)[n%3]<<8)|((n%4)<<10),((n%8)<<11)|((n*0x100)&0x4700)|(n*3%64),0x1f80|((n%4)<<13)|(0,0x40,0x8000,0x8040)[n%4]|(n*7%64)],live_x87=[struct.pack('<QH',0x8000000000000000+(k<<58),(0x3fff+k%3)|(0x8000 if k%2 else 0)).hex()for k in range(depth)],input_changes=[],**fields);rows.append(row);return row
    seeds=[0,1,2,0xffff,0x10000,0x7fffffff,0x80000000,0xffffffff,0x12345678,0xfedcba98]+[1<<bit for bit in range(32)]+[(~(1<<bit))&0xffffffff for bit in range(32)]+[((0x9e3779b9*(n+1))^0xa5a55a5a)&0xffffffff for n in range(24)]
    for tls in(False,True):
        for seed in seeds:make(f'rand_{tls}_{seed:x}',tls=tls,seed=seed)
        for entry in ENTRIES:
            for context in(0,DATA):
                for level in(0,1,2,3,128,255):make(f'context_{entry:x}_{tls}_{context:x}_{level}',entry=entry,tls=tls,context=context,level=level)
    for index in(0,1,2,31,0x40000001,0xffffffff):make(f'tls_index_{index:x}',tls=True,index=index)
    for offset in(0,1,2,3):make(f'context_alignment_{offset}',context=DATA+offset,seed=0x81726354)
    for flag in(2,0x80000000,0xffffffff):make(f'tls_flag_{flag:x}',tls=True,changes=((THREAD+0x28,flag,4),))
    make('alias_global_holder',context=GLOBAL-0x14,seed=GLOBAL-0x14)
    make('alias_tls_holder',tls=True,context=SLOT-8,seed=SLOT-8)
    make('alias_tls_index',tls=True,context=INDEX-0x14,seed=1)
    make('alias_tls_table',tls=True,context=TABLE+4-0x14,seed=SLOT)
    make('alias_fs_thread_pointer',fs=FS,context=FS+0x14,seed=THREAD)
    make('getter_unreadable_context',entry=0x330179,context=0x40000000)
    make('actual_seed_read_fault',context=0x40000000,actual_read_fault=True)
    make('actual_thread_read_fault',thread=0x40000000,actual_read_fault=True)
    make('actual_tls_table_read_fault',tls=True,table=0x40000000,actual_read_fault=True)
    make('actual_tls_slot_read_fault',tls=True,slot=0x40000000,actual_read_fault=True)
    make('actual_wrapped_holder_read_fault',tls=True,slot=0xfffffff4,actual_read_fault=True)
    bad=make('actual_fs_read_fault',actual_read_fault=True);bad['fs_base']=0x40000000
    for entry in ENTRIES:
        for tls in(False,True):make(f'fault_seed_{entry:x}_{tls}',entry=entry,tls=tls)
    make('fault_seed_fatal',level=2)
    make('fault_seed_allocation',context=0,tls=True)
    make('fault_seed_alias',tls=True,context=SLOT-8,seed=SLOT-8)
    return rows

def base_case(seed):
    excluded={'completed','boundary','after_fp','live_x87_unchanged','pending_x87_temporaries','callee_entries','fs_selector','actual_fs_base','boundary_gp','result_bits','reads','writes','banks','executed_sites','original_instructions','physical_verified'}
    row={k:copy.deepcopy(value)for k,value in seed.items()if k not in excluded};row['banks']=[dict(address=b['address'],before=b['before'])for b in seed['banks']];return row

def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path);parser.add_argument('--report',type=Path,required=True);parser.add_argument('--complete-only',action='store_true');parser.add_argument('--verify',type=Path);parser.add_argument('--callers',action='store_true');args=parser.parse_args();assert sha(NEW_DEFAULT)==NEW_PIN
    assert args.callers or(args.output and not args.output.exists())
    image=Xbe(NEW_DEFAULT);meta=metadata(image,args.callers);scratch=Path(os.environ['HALO2_TEST_SCRATCH']);owned=prepare(image,meta,scratch);declared=inputs(image)
    if args.callers:
        rows=[base_case(next(row for row in declared if row['name']==name))for name in('rand_False_0','rand_False_1','rand_False_ffffffff','rand_True_0','rand_True_1','rand_True_ffffffff')]
        for depth in range(9):
            row=copy.deepcopy(rows[1]);row['name']='caller_live_'+str(depth);row['before_fp'][1]=(depth%8<<11)|0x241;row['live_x87']=[struct.pack('<QH',0x8000000000000000+(n<<58),0x3fff+n%3).hex()for n in range(depth)];rows.append(row)
        for index,row in enumerate(rows):row.update(id=index,entry=0x19182a,caller=True,stop_address=0x19182f)
        rows=geometry.physical(owned,rows,'actual-callers');packets=[]
        for row in rows:
            assert row['completed']and row['live_x87_unchanged']and row['before_fp']==row['after_fp'];assert len(row['callee_entries'])==1
            entry=row['callee_entries'][0];sp=row['gp'][4];assert entry['gp'][4]==sp-4;assert struct.unpack('<I',bytes.fromhex(entry['stack_hex']))==(0x19182f,);assert row['boundary_gp'][4]==sp
            for n in(3,5,6,7):assert row['gp'][n]==row['boundary_gp'][n]
            packets.append(dict(name=row['name'],initial_gp=row['gp'],callee_entry=entry,returned_gp=row['boundary_gp'],before_fp=row['before_fp'],after_fp=row['after_fp'],older_values=len(row['live_x87']),older_values_preserved=True,original_instructions=row['original_instructions']))
        report=dict(passed=True,scope='Actual19182A CALL32C075 and real RET19182F before original clock producer33B590.',profiles=len(rows),actual_calls=len(rows),original_instructions=sum(row['original_instructions']for row in rows),packets=packets,instruction_patches=[],supplied_original_outcomes=False)
    else:
        rows=geometry.physical(owned,declared,'complete');prefix=[]
        if not args.complete_only:
            for seed in(row for row in rows if row['name'].startswith('fault_seed')):
                for phase,events in(('before_read',seed['reads']),('before_write',seed['writes']),('after_write',seed['writes'])):
                    for ordinal in range(len(events)):
                        row=base_case(seed);row.update(id=len(rows)+len(prefix),name=seed['name']+'_'+phase+'_'+str(ordinal),fault=dict(phase=phase,ordinal=ordinal));prefix.append(row)
            for start in range(0,len(prefix),32):rows+=geometry.physical(owned,prefix[start:start+32],'prefix-'+str(start))
        visited=set().union(*(set(row['executed_sites'])for row in rows));uncovered=sorted(row['instruction']for row in meta['sites']if row['instruction']not in visited);assert uncovered==list(GATED)
        payloads={}
        for row in rows:
            assert row['before_fp']==row['after_fp']and row['live_x87_unchanged']and row['actual_fs_base']==row['fs_base']
            for bank in row['banks']:
                for phase in('before','after'):
                    raw=bank.pop(phase);digest=hashlib.sha256(bytes.fromhex(raw)).hexdigest();assert digest not in payloads or payloads[digest]==raw;payloads[digest]=raw;bank[phase+'_ref']=digest
        fixture=dict(image_sha256=NEW_PIN,backend='physical-unchanged-i386-offline',owners=meta['owners'],instruction_patches=[],supplied_original_outcomes=False,bank_format='physical-complete-bank-intern-v1',bank_data=payloads,cases=rows)
        args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_bytes(gzip.compress(json.dumps(fixture,separators=(',',':')).encode(),mtime=0));repeat=None
        if args.verify:repeat=args.output.read_bytes()==args.verify.read_bytes();assert repeat,'Independent original CRT repeat differs'
        report=dict(passed=True,scope=__doc__,owners=meta['owners'],cases=len(rows),complete_calls=sum(row['completed']for row in rows),fault_prefixes=len(prefix),required_owner_prefixes=sum(row.get('boundary',{}).get('phase')=='required_owner'if row.get('boundary')else False for row in rows),actual_faults=sum(row.get('actual_read_fault',False)for row in rows),original_sites=len(visited),uncovered_postallocation_sites=list(map(hex,uncovered)),original_instructions=sum(row['original_instructions']for row in rows),ordered_reads=sum(len(row['reads'])for row in rows),ordered_stores=sum(len(row['writes'])for row in rows),fixture=str(args.output),fixture_bytes=args.output.stat().st_size,fixture_sha256=sha(args.output),repeat_identical=repeat)
    report.update(original_sha256=NEW_PIN,source_hashes={str(Path(__file__).relative_to(ROOT)):sha(__file__),'tools/observe_native_tu15_crt_random.cpp':sha(ROOT/'tools/observe_native_tu15_crt_random.cpp')},owned_scratch=str(scratch),retained_large_artifacts=[],limits=['Initialized ordinary/TLS context plus exact required-owner prefixes only;15 postallocation sites unclosed.','Actual i386 syscall243 supplies declared FS base without engine opcode changes.','No runtime allocator/fatal handler/clock/session outcome is supplied.'])
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({key:value for key,value in report.items()if key in('cases','complete_calls','original_sites','uncovered_postallocation_sites','fixture_bytes','profiles','actual_calls')}))

if __name__=='__main__':main()
