#!/usr/bin/env python3
"""Independent unchanged-i386 timer retirement with actual platform clock operands.

Fresh CLOCK_MONOTONIC samples are published at real33B595 before its unchanged
volatile load. Exact repeats replay only those captured external operands; fresh
clock repetitions make structural/state assertions without timestamp equality.
No original instruction, callee result, session outcome or default now is supplied.
"""
import argparse,copy,gzip,hashlib,json,os,struct,subprocess
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
import oracle_native_campaign_screen_lifecycle_tu15 as common
import validate_native_geometry_tu15 as geometry
ENTRIES=(0x777e0,0x33b590)
IAT,CLOCK,FLAG,CACHED,BASE=0x41b774,0x60212000,0x51d048,0x51d04c,0x515f70
sha=common.sha
OBSERVED={'completed','boundary','after_fp','live_x87_unchanged','pending_x87_temporaries','callee_entries','clock_events','boundary_gp','result_bits','reads','writes','banks','executed_sites','original_instructions','physical_verified','clock_replay'}

def metadata(image,callers=False):
    common.ENTRIES=ENTRIES;meta=common.metadata(image);meta['required_owners']={};code=_Code(image,'.text')
    for row in meta['sites']:
        if code.at(row['instruction']).mnemonic in('cmp','test'):
            for op in row['operands']:op.update(read=True,write=False)
    assert sum(row['sites']for row in meta['owners'].values())==38
    assert code.at(0x33b590).bytes.hex()=='a174b74100' and code.at(0x33b595).bytes.hex()=='8b00'
    if callers:
        common.ENTRIES=(0x77ab0,);extra=common.metadata(image)
        assert sum(row['sites']for row in extra['owners'].values())==17
        meta['sites']+=extra['sites'];meta['owners'].update(extra['owners'])
    return meta

def prepare(image,meta,scratch):
    p=scratch/'physical-original';p.mkdir();section=image.section('.text');(p/'text.bin').write_bytes(image.section_bytes(section));(p/'metadata.json').write_text(json.dumps(meta))
    regions={'iat':(0x41b000,0x1000),'records':(0x515000,0x5000),'overrides':(0x51d000,0x1000),'clock':(CLOCK,0x1000),'stack':(geometry.STACK,65536)}
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
    for command in (['as','--32',str(p/'reference.s'),'-o',str(p/'reference.o')],['ld','-melf_i386','-T',str(p/'link.ld'),str(p/'reference.o'),'-o',str(p/'reference')],['c++','-std=c++20','-O2',str(ROOT/'tools/observe_native_campaign_session_timers_tu15.cpp'),'-o',str(p/'observer')]):
        result=subprocess.run(command,text=True,capture_output=True);assert not result.returncode,result.stderr
    return p

def inputs():
    rows=[]
    def make(name,record_filter=0xffffffff,slot=0,states=(3,3,3,3,3),override=0,cached=0xa5379bdf,clock_available=True,clock_cell=CLOCK,changes=(),**fields):
        n=len(rows);banks={0x515f50:bytearray([0xa5]*0x3100),FLAG:bytearray([override,0x53,0x79,0xb1]),CACHED:bytearray(struct.pack('<I',cached)),IAT:bytearray(struct.pack('<I',clock_cell)),CLOCK:bytearray(4)}
        def put(at,value,width=4):
            raw=(value&((1<<(width*8))-1)).to_bytes(width,'little');overlap=[(p,b)for p,b in banks.items()if p<at+width and at<p+len(b)]
            if len(overlap)==1 and overlap[0][0]<=at and at+width<=overlap[0][0]+len(overlap[0][1]):p,b=overlap[0];b[at-p:at-p+width]=raw;return
            lo=min([at]+[p for p,b in overlap]);hi=max([at+width]+[p+len(b)for p,b in overlap]);data=bytearray(hi-lo)
            for p,b in overlap:data[p-lo:p-lo+len(b)]=b;del banks[p]
            data[at-lo:at-lo+width]=raw;banks[lo]=data
        for ordinal,state in enumerate(states):
            if record_filter!=0xffffffff and record_filter!=ordinal:continue
            at=(BASE+slot*0x98+ordinal*0x99c)&0xffffffff
            if not fields.get('actual_read_fault',False):put(at,state);put((at+4)&0xffffffff,0x93700000+ordinal)
        for at,value,width in changes:put(at,value,width)
        gp=[0x5e000000+n*37+k*11 for k in range(8)];gp[4]=geometry.SP+(n%4)*4;depth=n%9
        rows.append(dict(id=n,name=name,entry=0x777e0,record_filter=record_filter&0xffffffff,slot=slot&0xffffffff,clock_available=clock_available,clock_cell=clock_cell,gp=gp,banks=[dict(address=p,before=b.hex())for p,b in sorted(banks.items())],before_fp=[0x7f|((0,2,3)[n%3]<<8)|((n%4)<<10),((n%8)<<11)|((n*0x100)&0x4700)|(n*3%64),0x1f80|((n%4)<<13)|(0,0x40,0x8000,0x8040)[n%4]|(n*7%64)],live_x87=[struct.pack('<QH',0x8000000000000000+(k<<58),(0x3fff+k%3)|(0x8000 if k%2 else 0)).hex()for k in range(depth)],**fields));return rows[-1]
    for slot in range(16):
        for selected in(0xffffffff,0,1,2,3,4):make(f'slot_{slot}_filter_{selected:x}',slot=slot,record_filter=selected)
    for selected in(5,6,15,0x7fffffff,0x80000000,0xfffffffe):make('unused_filter_'+hex(selected),record_filter=selected,slot=0x87654321,clock_available=False,clock_cell=0x40000000)
    for state in(-0x80000000,-1,0,1,2,4,5,0x7fffffff):make('state_'+str(state),states=(state,)*5,clock_available=False)
    for ordinal in range(5):
        states=[0,1,2,4,5];states[ordinal]=3;make('one_active_'+str(ordinal),states=states)
    for override in(1,2,0x80,0xff):
        for cached in(0,1,0x7fffffff,0x80000000,0xffffffff):make(f'cached_{override}_{cached:x}',override=override,cached=cached)
    make('missing_clock_owner',clock_available=False)
    make('forged_clock_owner',clock_cell=CLOCK+4)
    make('forged_unmapped_clock_owner',clock_cell=0x40000000)
    make('cached_ignores_missing_owner',override=1,clock_available=False)
    make('cached_ignores_forged_unused_iat',override=1,clock_cell=0x40000000)
    make('inactive_ignores_owner',states=(0,1,2,4,5),clock_available=False,clock_cell=0x40000000)
    # Defined wrap arithmetic coverage; no fake live import bindings are admitted.
    modulus=1<<29;inverse=pow(19,-1,modulus)
    for target,ordinal in((CACHED,1),(FLAG,0)):
        delta=(target-BASE-ordinal*0x99c)&0xffffffff;assert delta%8==0
        slot=((delta//8)*inverse)%modulus
        make('wrapped_cached_alias_'+hex(target),slot=slot,record_filter=ordinal,override=1,changes=((target,3,4),))
    for slot in(0x80000000,0x20000000):make('wrapped_same_slot_'+hex(slot),slot=slot)
    for slot in(0x1000000,0x1000001):make('actual_state_fault_'+hex(slot),slot=slot,record_filter=0,actual_read_fault=True)
    make('fault_seed_live')
    make('fault_seed_cached',override=0x80)
    make('fault_seed_filtered',record_filter=4)
    make('fault_seed_missing',clock_available=False)
    make('fault_seed_alias_cached',slot=((CACHED-BASE-0x99c)//8*inverse)%modulus,record_filter=1,override=1,changes=((CACHED,3,4),))
    return rows

def base_case(seed,payloads=None,replay=False):
    row={k:copy.deepcopy(v)for k,v in seed.items()if k not in OBSERVED}
    row['banks']=[dict(address=b['address'],before=b.get('before',payloads[b['before_ref']]if payloads else None))for b in seed['banks']]
    if replay:row['clock_replay']=copy.deepcopy(seed['clock_events'])
    return row

def intern(rows):
    payloads={}
    for row in rows:
        row.pop('clock_replay',None)
        for bank in row['banks']:
            for phase in('before','after'):
                raw=bank.pop(phase);digest=hashlib.sha256(bytes.fromhex(raw)).hexdigest();assert digest not in payloads or payloads[digest]==raw;payloads[digest]=raw;bank[phase+'_ref']=digest
    return payloads

def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path);parser.add_argument('--report',type=Path,required=True);parser.add_argument('--complete-only',action='store_true');parser.add_argument('--verify',type=Path);parser.add_argument('--fresh-verify',type=Path);parser.add_argument('--callers',action='store_true');args=parser.parse_args();assert sha(NEW_DEFAULT)==NEW_PIN
    assert args.callers or args.fresh_verify or(args.output and not args.output.exists())
    image=Xbe(NEW_DEFAULT);meta=metadata(image,args.callers);scratch=Path(os.environ['HALO2_TEST_SCRATCH']);owned=prepare(image,meta,scratch);declared=inputs();prefix=[];repeat=None
    if args.callers:
        names=['slot_0_filter_ffffffff','slot_15_filter_ffffffff','cached_255_ffffffff','state_4','missing_clock_owner']
        selected=[base_case(next(row for row in declared if row['name']==name))for name in names]
        for slot in range(16):
            row=base_case(next(r for r in declared if r['name']==f'slot_{slot}_filter_ffffffff'));row.update(name='actual_slot_'+str(slot));selected.append(row)
        for depth in range(9):
            row=copy.deepcopy(selected[0]);row.update(name='caller_live_'+str(depth));row['before_fp'][1]=(depth%8<<11)|0x241;row['live_x87']=[struct.pack('<QH',0x8000000000000000+(n<<58),0x3fff+n%3).hex()for n in range(depth)];selected.append(row)
        for index,row in enumerate(selected):row.update(id=index,caller=True,entry=0x77ad0,stop_address=0x77ad8);row['gp'][6]=row['slot']
        # Genuine full original parent witnesses all80 clock samples across16slots.
        full=base_case(declared[0]);full.update(id=len(selected),name='full_77AB0_80_real_clock_reads',entry=0x77ab0,caller=True,install_root_stack=True,record_filter=0x97,slot=0)
        records=next(b for b in full['banks']if b['address']==0x515f50);raw=bytearray.fromhex(records['before'])
        for slot in range(16):
            for ordinal in range(5):
                at=BASE+slot*0x98+ordinal*0x99c-records['address'];raw[at:at+4]=struct.pack('<I',3)
        records['before']=raw.hex();full['banks'].append(dict(address=0x51d018,before='01000000'+('53'*4)));selected.append(full)
        rows=geometry.physical(owned,selected,'actual-callers');packets=[]
        for row in rows:
            assert row['live_x87_unchanged']and row['before_fp']==row['after_fp']
            for entry in row['callee_entries']:
                assert struct.unpack('<III',bytes.fromhex(entry['stack_hex']))[0:2]==(0x77ad8,0xffffffff)
            if row['completed']:
                if row['name'].startswith('full_'):assert len(row['callee_entries'])==16 and row['boundary_gp'][4]==row['gp'][4]+8
                else:
                    assert len(row['callee_entries'])==1;entry=row['callee_entries'][0];assert entry['gp'][4]==row['gp'][4]-12 and row['boundary_gp'][4]==row['gp'][4]
                    for n in(3,5,6,7):assert row['boundary_gp'][n]==entry['gp'][n]
            packets.append(dict(name=row['name'],completed=row['completed'],boundary=row['boundary'],initial_gp=row['gp'],record_filter=row['record_filter'],slot=row['slot'],callee_entries=row['callee_entries'],returned_gp=row['boundary_gp'],before_fp=row['before_fp'],after_fp=row['after_fp'],older_values=len(row['live_x87']),older_values_preserved=True,clock_events=row['clock_events'],original_instructions=row['original_instructions']))
        report=dict(passed=True,scope='Unchanged77AD0 PUSH/PUSH/CALL777E0 and trueRET8 to77AD8, plus complete unchanged77AB0 parent with80 genuine fresh clock samples; no native parent binding claimed.',profiles=len(rows),actual_calls=sum(len(r['callee_entries'])for r in rows),original_instructions=sum(r['original_instructions']for r in rows),packets=packets,instruction_patches=[],supplied_original_outcomes=False)
    elif args.fresh_verify:
        reference=json.loads(gzip.decompress(args.fresh_verify.read_bytes()));seeds=[r for r in reference['cases']if r['completed']and r['clock_events']][:16];rows=geometry.physical(owned,[base_case(r,reference['bank_data'])for r in seeds],'fresh-clock-repeat')
        for old,row in zip(seeds,rows):
            assert row['completed']and row['before_fp']==row['after_fp']and row['live_x87_unchanged']
            assert [tuple(e[k]for k in('instruction','cell','engine_reads_before','engine_stores_before','phase'))for e in old['clock_events']]==[tuple(e[k]for k in('instruction','cell','engine_reads_before','engine_stores_before','phase'))for e in row['clock_events']]
            assert [(e['instruction'],e['address'],e['bytes'],e['stores_before'])for e in old['reads']]==[(e['instruction'],e['address'],e['bytes'],e['stores_before'])for e in row['reads']]
            assert [(e['instruction'],e['address'],len(e['written']))for e in old['writes']]==[(e['instruction'],e['address'],len(e['written']))for e in row['writes']]
        report=dict(passed=True,scope='Fresh real platform samples: identical control flow/read-store addresses/phase and FP; timestamp/cell bytes deliberately not required equal.',profiles=len(rows),fresh_clock_events=sum(len(r['clock_events'])for r in rows),fresh_values=[r['clock_events'][0]['value']for r in rows],reference=str(args.fresh_verify),reference_sha256=sha(args.fresh_verify))
    else:
        if args.verify:
            reference=json.loads(gzip.decompress(args.verify.read_bytes()));declared=[base_case(r,reference['bank_data'],replay=True)for r in reference['cases']]
        rows=[]
        for start in range(0,len(declared),32):rows+=geometry.physical(owned,declared[start:start+32],'complete-'+str(start))
        if not args.complete_only and not args.verify:
            for seed in(row for row in rows if row['name'].startswith('fault_seed')):
                for phase,events in(('before_read',seed['reads']),('before_write',seed['writes']),('after_write',seed['writes']),('clock_refresh',seed['clock_events'])):
                    for ordinal in range(len(events)):
                        row=base_case(seed);row.update(id=len(rows)+len(prefix),name=seed['name']+'_'+phase+'_'+str(ordinal),fault=dict(phase=phase,ordinal=ordinal));prefix.append(row)
            for start in range(0,len(prefix),32):rows+=geometry.physical(owned,prefix[start:start+32],'prefix-'+str(start))
        visited=set().union(*(set(row['executed_sites'])for row in rows));assert len(visited)==38
        for row in rows:assert row['before_fp']==row['after_fp']and row['live_x87_unchanged']
        payloads=intern(rows)
        if args.verify:
            # Preserve canonical JSON key order only after independently checking
            # every actual observed field; raw sample replay never supplies state.
            for index,row in enumerate(rows):
                assert row==reference['cases'][index],('Observed replay differs',row['id'])
                rows[index]={key:row[key]for key in reference['cases'][index]}
        fixture=dict(image_sha256=NEW_PIN,backend='physical-unchanged-i386-offline',owners=meta['owners'],instruction_patches=[],supplied_original_outcomes=False,external_input_format='actual-platform-Milliseconds-at-33B595-v1',bank_format='physical-complete-bank-intern-v1',bank_data=payloads,cases=rows)
        args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_bytes(gzip.compress(json.dumps(fixture,separators=(',',':')).encode(),mtime=0))
        if args.verify:repeat=args.output.read_bytes()==args.verify.read_bytes();assert repeat,'Replay of raw external clock operands differs from independent original reference'
        report=dict(passed=True,scope=__doc__,owners=meta['owners'],cases=len(rows),complete_calls=sum(row['completed']for row in rows),required_clock_owners=sum(row['boundary']is not None and row['boundary']['phase']=='required_clock_owner'for row in rows),fault_prefixes=sum('fault'in r for r in rows),actual_faults=sum(row.get('actual_read_fault',False)for row in rows),original_sites=len(visited),uncovered_sites=[],original_instructions=sum(row['original_instructions']for row in rows),ordered_reads=sum(len(row['reads'])for row in rows),ordered_stores=sum(len(row['writes'])for row in rows),external_clock_publications=sum(len(row['clock_events'])for row in rows),fixture=str(args.output),fixture_bytes=args.output.stat().st_size,fixture_sha256=sha(args.output),replay_identical=repeat,clock_mode='replay of actual captured raw external inputs'if args.verify else'fresh actual CLOCK_MONOTONIC uint32 milliseconds')
    report.update(original_sha256=NEW_PIN,source_hashes={str(Path(__file__).relative_to(ROOT)):sha(__file__),'tools/observe_native_campaign_session_timers_tu15.cpp':sha(ROOT/'tools/observe_native_campaign_session_timers_tu15.cpp')},owned_scratch=str(scratch),retained_large_artifacts=[],limits=['Real original imported clock binding is explicitly rebound to the same dedicated mutable cell60212000 used by nativeWorld; no instruction patch.','Missing/forged live-clock owner is a stop-before-input/MOV dependency boundary, not an original engine error return.','Fresh samples are nondeterministic. Byte-identical repeat is exclusively an explicit raw-input journal replay; fresh repetition checks structure.','Ordinary retained RAM/disjoint private caller stack, DF0; wrapped source aliases are coverage profiles, no device-memory alias promise.','No World clock producer integration, native77AB0 parent or full completion/session outcome is claimed.'])
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({key:value for key,value in report.items()if key in('cases','complete_calls','original_sites','fixture_bytes','profiles','actual_calls','external_clock_publications','replay_identical')}))
if __name__=='__main__':main()
