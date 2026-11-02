#!/usr/bin/env python3
"""Independent physical original four timer CALL/RET8 frames and captured clocks.

Actual caller MOV/PUSH/CALL, full777E0/33B590 and realRET8 execute unchanged.
Only fresh raw monotonic clock operands (or exact journal replay) are external.
"""
import argparse,copy,gzip,hashlib,json,os,struct,subprocess
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
import oracle_native_campaign_session_timers_tu15 as timer
import validate_native_geometry_tu15 as geometry
SITES={0x77ad3:(0x77ad0,0x77ad8),0x77bd3:(0x77bcd,0x77bd8),0x77c1a:(0x77c17,0x77c1f),0x78204:(0x78202,0x78209)}
sha=timer.sha
OBSERVED=timer.OBSERVED|{'caller_reads','argument_reads','return_reads','call_packet','entry_packet','boundary_xmm','boundary_x87','boundary_eflags','core_reads','caller_root_gp','call_gp','entry_gp','entry_stack','entry_eflags','entry_xmm','entry_x87','call_arguments','call_instruction_bytes','entry','gp'}

def metadata(image):
    meta=timer.metadata(image);code=_Code(image,'.text');registers={name:n for n,name in enumerate(('eax','ecx','edx','ebx','esp','ebp','esi','edi'))}
    for site,(first,last)in SITES.items():
        pc=first
        while pc<last:
            ins=code.at(pc);ops=[]
            for op in ins.operands:
                if op.type!=3 or ins.mnemonic=='lea':continue
                b,i=ins.reg_name(op.mem.base),ins.reg_name(op.mem.index)
                ops.append(dict(base=registers[b]if b else-1,index=registers[i]if i else-1,scale=op.mem.scale,displacement=op.mem.disp&0xffffffff,bytes=op.size,read=bool(op.access&1),write=bool(op.access&2)))
            meta['sites'].append(dict(instruction=pc,bytes=ins.bytes.hex(),operands=ops));pc+=ins.size
        assert pc==last and code.at(site).mnemonic=='call' and code.at(site).operands[0].imm==0x777e0
    assert len(meta['sites'])==51
    return meta

def prepare(image,meta,scratch):
    p=scratch/'physical-original';p.mkdir();section=image.section('.text');(p/'text.bin').write_bytes(image.section_bytes(section));(p/'metadata.json').write_text(json.dumps(meta))
    regions={'iat':(0x41b000,0x1000),'records':(0x515000,0x5000),'overrides':(0x51d000,0x1000),'clock':(timer.CLOCK,0x1000),'participants':(0x1000000,0x1000),'stack':(geometry.STACK,65536)}
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
    for command in(['as','--32',str(p/'reference.s'),'-o',str(p/'reference.o')],['ld','-melf_i386','-T',str(p/'link.ld'),str(p/'reference.o'),'-o',str(p/'reference')],['c++','-std=c++20','-O2',str(ROOT/'tools/observe_native_campaign_session_timers_callers_tu15.cpp'),'-o',str(p/'observer')]):
        result=subprocess.run(command,text=True,capture_output=True);assert not result.returncode,result.stderr
    return p

def put(row,address,value,width=4):
    banks=row['banks'];raw=(value&((1<<(width*8))-1)).to_bytes(width,'little')
    for b in banks:
        first=b['address'];data=bytearray.fromhex(b['before'])
        if first<=address and address+width<=first+len(data):data[address-first:address-first+width]=raw;b['before']=data.hex();return
    assert not any(b['address']<address+width and address<b['address']+len(b['before'])//2 for b in banks)
    banks.append(dict(address=address,before=raw.hex()));banks.sort(key=lambda b:b['address'])

def inputs():
    original=timer.inputs();names=[f'slot_{slot}_filter_ffffffff'for slot in range(16)]+['state_0','state_2','state_4','cached_1_0','cached_255_ffffffff','cached_128_80000000','inactive_ignores_owner','wrapped_same_slot_0x20000000','wrapped_same_slot_0x80000000']
    profiles=[timer.base_case(next(r for r in original if r['name']==name))for name in names]
    for slot,state in((1,0),(15,4),(0x20000001,2),(0x80000001,0xffffffff)):
        row=timer.base_case(next(r for r in original if r['name']==f'slot_{slot&15}_filter_ffffffff'));row.update(name='idle_final_'+hex(slot),slot=slot)
        put(row,(timer.BASE+slot*0x98+4*0x99c)&0xffffffff,state);profiles.append(row)
    for depth in range(9):
        row=timer.base_case(original[0]);row.update(name='old_fp_depth_'+str(depth));row['before_fp'][1]=(depth%8<<11)|0x241;row['live_x87']=[struct.pack('<QH',0x8000000000000000+(n<<58),0x3fff+n%3).hex()for n in range(depth)];profiles.append(row)
    rows=[]
    def add(seed,site):
        row=copy.deepcopy(seed);n=len(rows);first,last=SITES[site];row.update(id=n,name=f'{site:x}_'+seed['name'],caller_site=site,caller_entry=first,caller_return=last,return_address=last,entry=first,caller=True,call=dict(instruction=site,target=0x777e0,tail=False),stop_address=last)
        row['gp']=[0x5f000000+n*37+k*11 for k in range(8)];row['gp'][4]=geometry.SP+(n%4)*4
        if site in(0x77ad3,0x77c1a):row['gp'][6]=row['slot'];row['record_filter']=0xffffffff
        elif site==0x77bd3:
            row['record_filter']=0xffffffff;row['gp'][7]=0x1000000+(n%4)*16;put(row,row['gp'][7],0xa57953b1);put(row,row['gp'][7]+4,row['slot'])
        else:row['gp'][7]=row['slot'];row['gp'][5]=row['record_filter']
        rows.append(row);return row
    for site in SITES:
        for profile in profiles:add(profile,site)
    for name in('slot_1_filter_0','slot_15_filter_1','slot_1_filter_2','slot_15_filter_3','slot_1_filter_4','unused_filter_0x5','unused_filter_0x80000000','unused_filter_0xfffffffe','wrapped_cached_alias_0x51d04c'):
        add(timer.base_case(next(r for r in original if r['name']==name)),0x78204)
    for slot in(1,15,0x20000001):
        row=timer.base_case(next(r for r in original if r['name']==f'slot_{slot&15}_filter_ffffffff'));row.update(name='selected_idle_final_'+hex(slot),slot=slot,record_filter=4);put(row,(timer.BASE+slot*0x98+4*0x99c)&0xffffffff,0);add(row,0x78204)
    row=add(timer.base_case(next(r for r in original if r['name']=='slot_3_filter_ffffffff')),0x77bd3);row['name']='77bd3_held_source_alias_final';row['gp'][7]=timer.BASE+3*0x98+4*0x99c-4;assert int.from_bytes(bytes.fromhex(next(b for b in row['banks']if b['address']<=row['gp'][7]+4<b['address']+len(b['before'])//2)['before'])[row['gp'][7]+4-0x515f50:row['gp'][7]+8-0x515f50],'little')==3
    return rows

def raw_case(row,payloads=None,replay=False):
    result={k:copy.deepcopy(v)for k,v in row.items()if k not in OBSERVED}
    result['gp']=copy.deepcopy(row.get('caller_root_gp',row.get('gp')));result['entry']=row['caller_entry'];result['banks']=[dict(address=b['address'],before=b.get('before',payloads[b['before_ref']]if payloads else None))for b in row['banks']]
    if replay:result['clock_replay']=copy.deepcopy(row['clock_events'])
    return result

def finish(row):
    packet=row.pop('entry_packet');call=row.pop('call_packet');assert packet is not None and call is not None
    row['caller_root_gp']=row['gp'];row['gp']=packet['gp'];row['entry_gp']=packet['gp'];row['entry']=0x777e0;row['entry_stack']=packet['stack_hex'];row['entry_eflags']=packet['eflags'];row['entry_xmm']=packet['xmm'];row['entry_x87']=row['live_x87'];row['call_gp']=call['gp'];row['call_arguments']=call['arguments'];row['call_instruction_bytes']=call['instruction_bytes'];row['core_reads']=copy.deepcopy(row['reads']);row.pop('callee_entries')
    ret,first,second=struct.unpack('<III',bytes.fromhex(row['entry_stack']));assert(ret,first,second)==(row['return_address'],row['record_filter'],row['slot']);assert call['arguments']==row['entry_stack'][8:]
    assert row['entry_gp'][4]==row['caller_root_gp'][4]-12 and row['call_gp'][4]==row['entry_gp'][4]+4
    for n in(1,2,3,5,6,7):assert row['entry_gp'][n]==row['boundary_gp'][n]
    assert row['before_fp']==row['after_fp']and row['live_x87_unchanged']and row['boundary_x87']==row['live_x87']and row['entry_xmm']==row['boundary_xmm']
    if row['completed']:
        assert row['boundary_gp'][4]==row['entry_gp'][4]+12 and len(row['return_reads'])==1
    else:assert row['boundary']['phase']=='before_return_read'and row['boundary_gp'][4]==row['entry_gp'][4]and not row['return_reads']
    row.pop('clock_replay',None)
    return row

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path);p.add_argument('--report',type=Path,required=True);p.add_argument('--complete-only',action='store_true');p.add_argument('--verify',type=Path);p.add_argument('--source-faults',action='store_true');a=p.parse_args();assert sha(NEW_DEFAULT)==NEW_PIN;assert a.source_faults or(a.output and not a.output.exists())
    image=Xbe(NEW_DEFAULT);meta=metadata(image);scratch=Path(os.environ['HALO2_TEST_SCRATCH']);owned=prepare(image,meta,scratch);declared=inputs();rows=[]
    if a.source_faults:
        declared=[copy.deepcopy(next(r for r in declared if r['caller_site']==0x77bd3))]
        declared[0].update(name='actual_preCALL_source_fault',actual_read_fault=True);declared[0]['gp'][7]=0x40000000
        rows=geometry.physical(owned,declared,'precall-fault');row=rows[0];assert not row['completed']and row['entry_packet']is None and row['call_packet']is None and not row['reads']and not row['writes']and not row['clock_events']
        report=dict(passed=True,scope='Actual77BCD source-memory fault occurs beforeCALL; no prepared native timer invocation exists.',cases=[dict(name=row['name'],boundary=row['boundary'],entry_gp=None,original_instructions=row['original_instructions'],engine_reads=len(row['reads']),engine_stores=len(row['writes']),clocks=len(row['clock_events']))])
    else:
        if a.verify:
            reference=json.loads(gzip.decompress(a.verify.read_bytes()));declared=[raw_case(row,reference['bank_data'],replay=True)for row in reference['cases']]
        for start in range(0,len(declared),32):rows +=geometry.physical(owned,declared[start:start+32],'complete-'+str(start))
        if not a.complete_only and not a.verify:
            cuts=[]
            for site in SITES:
                for stem in('slot_1_filter_ffffffff','idle_final_0xf','cached_255_ffffffff','idle_final_0x20000001'):
                    seed=next(r for r in rows if r['name']==f'{site:x}_{stem}');row=raw_case(seed);row.update(id=len(rows)+len(cuts),name=seed['name']+'_before_return_read',fault=dict(phase='before_return_read',ordinal=0));cuts.append(row)
            for stem in('slot_15_filter_1','unused_filter_0x80000000','selected_idle_final_0xf'):
                seed=next(r for r in rows if r['name']=='78204_'+stem);row=raw_case(seed);row.update(id=len(rows)+len(cuts),name=seed['name']+'_before_return_read',fault=dict(phase='before_return_read',ordinal=0));cuts.append(row)
            seed=next(r for r in rows if r['name']=='77bd3_held_source_alias_final');row=raw_case(seed);row.update(id=len(rows)+len(cuts),name=seed['name']+'_before_return_read',fault=dict(phase='before_return_read',ordinal=0));cuts.append(row)
            for start in range(0,len(cuts),32):rows+=geometry.physical(owned,cuts[start:start+32],'return-cut-'+str(start))
        rows=[finish(row)for row in rows];payloads=timer.intern(rows)
        if a.verify:
            for n,row in enumerate(rows):assert row==reference['cases'][n],('Actual caller replay differs',row['name']);rows[n]={key:row[key]for key in reference['cases'][n]}
        fixture=dict(image_sha256=NEW_PIN,backend='physical-unchanged-i386-offline',owners=meta['owners'],caller_sites={hex(k):dict(entry=v[0],return_address=v[1])for k,v in SITES.items()},instruction_patches=[],supplied_original_outcomes=False,external_input_format='actual-platform-Milliseconds-at-33B595-v1',bank_format='physical-complete-bank-intern-v1',bank_data=payloads,cases=rows)
        a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_bytes(gzip.compress(json.dumps(fixture,separators=(',',':')).encode(),mtime=0));replay=None
        if a.verify:replay=a.output.read_bytes()==a.verify.read_bytes();assert replay,'Actual caller raw-clock replay is not byte identical'
        report=dict(passed=True,cases=len(rows),complete_calls=sum(r['completed']for r in rows),return_read_cuts=sum(not r['completed']for r in rows),original_sites=len(set().union(*(set(r['executed_sites'])for r in rows))),original_instructions=sum(r['original_instructions']for r in rows),core_reads=sum(len(r['core_reads'])for r in rows),caller_reads=sum(len(r['caller_reads'])for r in rows),argument_reads=sum(len(r['argument_reads'])for r in rows),return_reads=sum(len(r['return_reads'])for r in rows),engine_stores=sum(len(r['writes'])for r in rows),external_clock_events=sum(len(r['clock_events'])for r in rows),caller_profiles={hex(site):sum(r['caller_site']==site for r in rows)for site in SITES},fixture=str(a.output),fixture_bytes=a.output.stat().st_size,fixture_sha256=sha(a.output),replay_identical=replay,clock_mode='raw actual-input replay'if a.verify else'fresh actual CLOCK_MONOTONIC milliseconds',idle_nonzero_witnesses=[dict(name=r['name'],slot=r['slot'],actual_eax=r['boundary_gp'][0])for r in rows if r['completed']and'idle_final'in r['name']],instruction_patches=[],supplied_original_outcomes=False)
    report.update(original_sha256=NEW_PIN,source_hashes={str(Path(__file__).relative_to(ROOT)):sha(__file__),'tools/observe_native_campaign_session_timers_callers_tu15.cpp':sha(ROOT/'tools/observe_native_campaign_session_timers_callers_tu15.cpp'),'tools/oracle_native_campaign_session_timers_tu15.py':sha(ROOT/'tools/oracle_native_campaign_session_timers_tu15.py')},owned_scratch=str(scratch),limits=['Actual caller private argument reads are separated and may be omitted only for valid/disjoint prepared-call value profile.','Exact lateRETword reads remain observable after all engine stores/result publication; before_return_read cuts reject that actual fetch.','Arithmetic flags are source-proven dead at each actual continuation and recorded separately; other GP/FP/XMM remain exact.','Fresh-clock byte equality is not claimed; exact replay consumes only captured external operands, never original callee results.','No full77AF0/78020 parent execution, invalid native admission outcome, CPU emulator or World integration is claimed.'])
    a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:report[k]for k in('cases','complete_calls','return_read_cuts','original_sites','fixture_bytes','replay_identical')if k in report}))
if __name__=='__main__':main()
