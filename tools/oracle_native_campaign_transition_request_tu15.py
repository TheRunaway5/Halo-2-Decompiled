#!/usr/bin/env python3
"""Physical unchanged191580 publications/33B590 clock, real5A080 frontier."""
import argparse,contextlib,gzip,hashlib,io,json,os,subprocess
from pathlib import Path
from capstone import CS_AC_READ,CS_AC_WRITE
from capstone.x86 import X86_OP_MEM
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
import oracle_native_campaign_screen_lifecycle_tu15 as common
import oracle_native_campaign_ai_perception_group_link_tu15 as transport
import validate_native_geometry_tu15 as geometry
BASE=0x1000000;OPTIONS=BASE+0x1000;CLOCK=BASE+0xf000;DEST=0x4f9c78
ENTRIES=(0x191580,0x33b590);sha=common.sha
def metadata(image):
    code=_Code(image,'.text');sites=[];owners={};registers={r:n for n,r in enumerate(('eax','ecx','edx','ebx','esp','ebp','esi','edi'))}
    for entry in ENTRIES:
        body=_trace(code,entry,{*ENTRIES,0x5a080});listing=[]
        for pc in sorted(body.code):
            ins=code.at(pc);operands=[];listing.append(f'{pc:x} {ins.bytes.hex()} {ins.mnemonic} {ins.op_str}\n')
            for operand in ins.operands:
                if operand.type!=X86_OP_MEM or ins.mnemonic=='lea':continue
                base=ins.reg_name(operand.mem.base);index=ins.reg_name(operand.mem.index)
                operands.append(dict(base=registers[base]if base else-1,index=registers[index]if index else-1,scale=operand.mem.scale,displacement=operand.mem.disp&0xffffffff,bytes=operand.size,read=bool(operand.access&CS_AC_READ),write=bool(operand.access&CS_AC_WRITE)))
            row=dict(instruction=pc,bytes=ins.bytes.hex(),operands=operands)
            if pc==0x191595:
                assert ins.bytes.hex()=='f3a5';row['repeat_count_register']=1
                # MOVSD's source read precedes its destination store. Operand
                # display order is not an execution-order observation.
                row['operands'].sort(key=lambda op:not op['read'])
            sites.append(row)
        owners[hex(entry)]=dict(sites=len(body.code),listing_sha256=hashlib.sha256(''.join(listing).encode()).hexdigest())
    return dict(sites=sites,owners=owners,stop=geometry.STOP)
def prepare(image,meta,scratch):
    owned=transport.prepare(image,meta,scratch)
    result=subprocess.run(['c++','-std=c++20','-O2',str(ROOT/'tools/observe_native_campaign_transition_request_tu15.cpp'),'-o',str(owned/'observer')],capture_output=True,text=True);assert not result.returncode,result.stderr
    return owned
def inputs():
    rows=[]
    def make(name='publication',options=OPTIONS,private=False,flag=0,state=0,option_flag=0,mode=3,count=2,tick=0x12345678,clock=CLOCK):
        n=len(rows);banks={BASE:bytearray((k*37+11)&255 for k in range(0x10000)),0x4f9000:bytearray((k*19+7)&255 for k in range(0x4000)),0x535000:bytearray(0x2000)}
        def put(p,v,w=4):
            for first,data in banks.items():
                if first<=p and p+w<=first+len(data):data[p-first:p-first+w]=(v&((1<<(8*w))-1)).to_bytes(w,'little');return
            banks[p]=bytearray((v&((1<<(8*w))-1)).to_bytes(w,'little'))
        put(0x535f00,flag,1);put(0x535f04,state);put(0x41b774,clock);put(clock,tick)
        raw=bytearray((k*29+n*13+7)&255 for k in range(0x1118));raw[0:4]=(mode&0xffffffff).to_bytes(4,'little');raw[5]=option_flag;raw[0x264:0x268]=(count&0xffffffff).to_bytes(4,'little')
        if options and not private:
            for i in range(0,len(raw),4):put(options+i,int.from_bytes(raw[i:i+4],'little'))
        gp=[0x5a000000+n*37+i*11 for i in range(8)];gp[4]=geometry.SP-0x2000+n%4*4;address=gp[4]+0x500 if private else options;gp[0]=address
        rows.append(dict(id=n,name=f'{name}_{n}',entry=0x191580,gp=gp,matrix=0,source=0,options=address,private_options=private,options_value=raw.hex(),child_output=0,private_output=False,banks=[dict(address=p,before=data.hex())for p,data in sorted(banks.items())],before_fp=[0x27f|(n%4<<10),(n%8<<11)|0x41,0x1f80|(n%4<<13)|0x8040],live_x87=[]))
    for options in (0,OPTIONS):
        for flag in (0,1,255):
            for state in (0,2,3,8,9):make(options=options,flag=flag,state=state,name='actual_presence_session_gates')
    for private in (False,True):
        for option_flag in (0,1,255):
            for mode in (1,3):
                for count in (-2147483648,-1,0,1,2,3,6,7,2147483647):make(private=private,flag=1,state=3,option_flag=option_flag,mode=mode,count=count,name='actual_private_and_retained_option_guards')
    for source in (DEST-4,DEST-1,DEST,DEST+1,DEST+4,0x4f9c6d):make(options=source,name='forward_stream_overlap')
    for clock in (CLOCK,DEST+8,0x4f9c6d,0x4f9c70):make(options=0,clock=clock,tick=0x87654321,name='clock_publication_alias')
    for kwargs in [dict(name='fault_seed_copy'),dict(name='fault_seed_null',options=0,flag=1,state=3),dict(name='fault_seed_private',private=True,flag=1,state=8,count=7),dict(name='fault_seed_overlap',options=DEST-4)]:make(**kwargs)
    return rows
def intern_banks(row,payloads):
    for bank in row['banks']:
        for phase in ('before','after'):
            raw=bank.pop(phase);digest=hashlib.sha256(bytes.fromhex(raw)).hexdigest()
            assert digest not in payloads or payloads[digest]==raw
            payloads[digest]=raw;bank[phase+'_ref']=digest
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--report',type=Path,required=True);p.add_argument('--verify',action='store_true');a=p.parse_args();assert a.verify or not a.output.exists();assert sha(NEW_DEFAULT)==NEW_PIN
    image=Xbe(NEW_DEFAULT);meta=metadata(image);owned=prepare(image,meta,Path(os.environ['HALO2_TEST_SCRATCH']));rows=geometry.physical(owned,inputs(),'complete');specifications=[]
    for suffix in ('inputs','observed'):(owned/('complete-'+suffix+'.json')).unlink()
    for seed in(r for r in rows if r['name'].startswith('fault_seed')):
        for phase,events in (('before_read',seed['reads']),('before_write',seed['writes']),('after_write',seed['writes'])):
            # Every copy position has independently executed read and committed
            # store cuts. Rejected store cuts cover both ends and interior.
            ordinals=range(len(events))if phase!='before_write'else sorted({0,1,2,31,len(events)//2,len(events)-1})
            for n in ordinals:
                if n<0 or n>=len(events):continue
                specifications.append((seed['id'],phase,n))
        specifications.append((seed['id'],'clock_refresh',0))
    visited=set().union(*(set(r['executed_sites'])for r in rows));read_count=sum(len(r['reads'])for r in rows);write_count=sum(len(r['writes'])for r in rows);payloads={}
    for row in rows:intern_banks(row,payloads)
    # Independently execute every cut in bounded batches. Never feed complete
    # prior traces back into the observer as unused declared input, or retain
    # duplicate prefix event arrays after their literal prefix is verified.
    excluded={'after_xmm0_bits','iterator_after','completed','boundary','after_fp','live_x87_unchanged','boundary_gp','reads','writes','banks','executed_sites','original_instructions','physical_verified','clock_events'}
    first_prefix=len(rows)
    for start in range(0,len(specifications),32):
        batch=[]
        for index,(seed_id,phase,n)in enumerate(specifications[start:start+32]):
            seed=rows[seed_id];row={k:v for k,v in seed.items()if k not in excluded};row['banks']=[dict(address=b['address'],before=payloads[b['before_ref']])for b in seed['banks']]
            row.update(id=first_prefix+start+index,name=seed['name']+'_'+phase+'_'+str(n),fault=dict(phase=phase,ordinal=n),trace_prefix_seed=seed_id);batch.append(row)
        label='prefix-'+str(start)
        with contextlib.redirect_stdout(io.StringIO()):batch=geometry.physical(owned,batch,label)
        for suffix in ('inputs','observed'):(owned/(label+'-'+suffix+'.json')).unlink()
        for row in batch:
            seed=rows[row['trace_prefix_seed']];read_count+=len(row['reads']);write_count+=len(row['writes']);visited.update(row['executed_sites'])
            for field in ('reads','writes'):
                assert row[field]==seed[field][:len(row[field])],(row['name'],field)
                row[field+'_prefix']=dict(seed=seed['id'],count=len(row[field]));del row[field]
            intern_banks(row,payloads)
        rows+=batch
    fixture=dict(image_sha256=NEW_PIN,backend='physical-unchanged-i386-offline',owners=meta['owners'],instruction_patches=[],supplied_original_outcomes=False,event_format='exact-original-prefix-v1',bank_format='physical-complete-bank-intern-v1',bank_data=payloads,cases=rows);encoded=gzip.compress(json.dumps(fixture,separators=(',',':')).encode(),mtime=0)
    if a.verify:assert a.output.read_bytes()==encoded,'Transition publication independent physical repeat differs'
    else:a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_bytes(encoded)
    report=dict(passed=True,owners=meta['owners'],cases=len(rows),complete_calls=sum(r['completed']for r in rows),required_owner_boundaries=sum(r['boundary']is not None and r['boundary']['phase']=='required_owner'for r in rows),fault_prefixes=len(specifications),reference_batch_size=32,lossless_prefix_events_verified=True,original_sites_executed=len(visited),uncovered_sites=[hex(s['instruction'])for s in meta['sites']if s['instruction']not in visited],ordered_reads=read_count,ordered_stores=write_count,unchanged_instructions=sum(r['original_instructions']for r in rows),fixture=str(a.output),fixture_bytes=len(encoded),fixture_sha256=sha(a.output),required_reference_inputs=[dict(path=str(a.output),bytes=len(encoded),reason='Canonical independent physical reference; literal prefixes and banks are losslessly interned')],retained_disposable_large_artifacts=[],source_hashes={str(Path(__file__).relative_to(ROOT)):sha(__file__),'tools/observe_native_campaign_transition_request_tu15.cpp':sha(ROOT/'tools/observe_native_campaign_transition_request_tu15.cpp')});a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))
if __name__=='__main__':main()
