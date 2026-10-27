#!/usr/bin/env python3
"""Checked original contact-array growth/free lifetime for an explicitly selected retained input.

Hardware breakpoints observe genuine original calls and return addresses.
The full watched heap must match ordinary ptrace original execution. No raw
bank/ELF/debugger output survives run_test.py scratch cleanup.
"""
import argparse,hashlib,json,os,struct,subprocess,sys
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
def sha(path):return hashlib.file_digest(Path(path).open('rb'),'sha256').hexdigest()
GDB=r'''
set pagination off
set confirm off
set disable-randomization off
set debuginfod enabled off
run > DEBUGGEE
python
import gdb,json,struct
events=[];tracked={};copy_break=None
def reg(n):return int(gdb.parse_and_eval('$'+n))&0xffffffff
def raw(p,n):return bytes(gdb.selected_inferior().read_memory(p,n))
def words(p,n):return list(struct.unpack('<'+'I'*n,raw(p,n*4)))
def metadata(size):
    pool=words(0x48606c,1)[0];index=words(pool+0x2b0+((size-1)>>10)*4,1)[0]
    return dict(pool=pool,size_class=index,head_address=pool+0x24+index*4,head=words(pool+0x24+index*4,1)[0],
        in_use_address=pool+0x2e8+index*4,in_use=words(pool+0x2e8+index*4,1)[0],
        free_address=pool+0xf0388+index*4,free=words(pool+0xf0388+index*4,1)[0])
def emit(kind,**kw):
    event=dict(kind=kind,pc=reg('eip'),esp=reg('esp'),**kw);events.append(event);gdb.write(json.dumps(event,sort_keys=True)+'\n')
class Break(gdb.Breakpoint):
    def __init__(self,address,temporary=False):super().__init__('*0x%x'%address,type=gdb.BP_HARDWARE_BREAKPOINT,temporary=temporary)
class Growth(Break):
    def stop(self):
        pointer=reg('eax');stride=reg('ebx');capacity=reg('ebp')
        if stride==0x80 and HEAP_FIRST<=pointer<HEAP_LIMIT:
            header=reg('esi');tracked[pointer]=dict(header=header,size=stride*capacity)
            emit('allocate_return',pointer=pointer,capacity=capacity,stride=stride,header=header,header_words=words(header,3),metadata=metadata(stride*capacity))

        return False
class CopyRecord(Break):
    def stop(self):
        destination=reg('ecx');source=words(reg('esp')+4,1)[0]
        record_index=None
        for pointer,record in tracked.items():
            if pointer<=destination<pointer+record['size']:record_index=(destination-pointer)//0x80;break
        emit('source_record_copy_entry',source=source,destination=destination,record_index=record_index,source_payload=raw(source,0x80).hex())
        return False
class PrivateAccess(gdb.Breakpoint):
    def __init__(self,address):
        self.address=address
        super().__init__('*('+{1:'unsigned char',2:'unsigned short',4:'unsigned int'}[SELECTED_WIDTH]+'*)0x%x'%address,type=gdb.BP_WATCHPOINT,wp_class=gdb.WP_ACCESS)
    def stop(self):
        emit('selected_private_lane_access',address=self.address,width=SELECTED_WIDTH,value=int.from_bytes(raw(self.address,SELECTED_WIDTH),'little'),gp={n:reg(n)for n in('eax','ecx','edx','ebx','ebp','esi','edi')})
        return False
class ConsumerGate(Break):
    def stop(self):
        record=(reg('esi')+reg('eax'))&0xffffffff if reg('eip')==0x304858 else reg('eax')
        emit('valid_contact_consumer_gate',record=record,valid=raw(record+0x7d,1)[0],payload=raw(record,0x80).hex(),gp={n:reg(n)for n in('eax','ecx','edx','ebx','esi','edi')})
        return False
class FreeReturn(Break):
    def __init__(self,address,pointer,size):self.pointer=pointer;self.size=size;super().__init__(address,True)
    def stop(self):
        emit('free_return',pointer=self.pointer,size=self.size,metadata=metadata(self.size),payload=raw(self.pointer,self.size).hex())
        self.enabled=False
        copy_break.enabled=True
        return False
class Free(Break):
    def stop(self):
        args=words(reg('esp'),4);ret,pointer,size,tag=args
        if pointer in tracked:
            emit('free_entry',pointer=pointer,size=size,tag=tag,actual_return=ret,header_words=words(tracked[pointer]['header'],3),metadata=metadata(size),payload=raw(pointer,size).hex())
            copy_break.enabled=False;FreeReturn(ret,pointer,size)
        return False
class RootReturn(Break):
    def stop(self):
        final=[]
        for pointer,record in sorted(tracked.items()):final.append(dict(pointer=pointer,size=record['size'],metadata=metadata(record['size']),payload=raw(pointer,record['size']).hex()))
        emit('root_return',allocations=final)
        gdb.write('CONTACT_ALLOCATION_EVENTS='+json.dumps(events,sort_keys=True)+'\n')
        gdb.execute('dump binary memory HEAP_DUMP 0x10060000 HEAP_END')
        gdb.execute('quit');return True
growth=Growth(0x2e327e);freer=Free(0x149e00);RootReturn(0x1ccaca);copy_break=AUXILIARY_BREAK
end
continue
'''
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--capture',type=Path,required=True);p.add_argument('--differences',type=Path,required=True);p.add_argument('--report-output',type=Path,required=True);p.add_argument('--selected-read-address',type=lambda s:int(s,0),required=True);p.add_argument('--watch-access',action='store_true');p.add_argument('--reuse-replay',action='store_true');p.add_argument('--selected-width',type=int,choices=(1,2,4),default=4);p.add_argument('--consumer-gate',type=lambda s:int(s,0),choices=(0x304858,0x3049a0));a=p.parse_args();scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();a.capture=a.capture.resolve();a.report_output.mkdir(parents=True,exist_ok=True)
    assert sha(NEW_DEFAULT)==NEW_PIN;d=json.loads((a.capture/'capture.json').read_text());heap=next(r for r in d['regions']if r['address']==0x10060000);replay=scratch/'replay'
    command=[sys.executable,str(ROOT/'tools/replay_live_phantom_hardware_tu15.py'),'--native-frame-semantics','--return-breakpoints','--capture',str(a.capture),'--output',str(replay)]
    # Reuse is restricted to this same owned scratch and authenticated exact
    # input/output files. It preserves the real original comparison result.
    input_digest=hashlib.sha256()
    for region in sorted(d['regions'],key=lambda r:r['address']):
        input_digest.update(struct.pack('<II',region['address'],region['size']))
        input_digest.update(bytes.fromhex(sha(a.capture/region['before_file'])))
        input_digest.update(bytes.fromhex(sha(a.capture/region.get('after_file',region['before_file']))))
    input_pin=dict(capture=str(a.capture),metadata=sha(a.capture/'capture.json'),banks=input_digest.hexdigest(),replayer=sha(ROOT/'tools/replay_live_phantom_hardware_tu15.py'),observer=sha(ROOT/'tools/live_physics_hardware_return_tu15.cpp'),original=NEW_PIN)
    reuse_pin=replay/'diagnostic-reuse-pin.json'
    if a.reuse_replay:
        prior=json.loads(reuse_pin.read_text());assert prior['input']==input_pin,'Original diagnostic reuse inputs changed'
        assert sha(replay/'output.bin')==prior['output_sha256'] and sha(replay/'original-hardware')==prior['executable_sha256'],'Original diagnostic reuse artifact changed'
        reference=subprocess.CompletedProcess(command,prior['actual_reference_exit'])
    else:
        reference=subprocess.run(command,capture_output=True,text=True);assert(replay/'output.bin').exists(),reference.stderr[-2000:]
        reuse_pin.write_text(json.dumps(dict(input=input_pin,actual_reference_exit=reference.returncode,output_sha256=sha(replay/'output.bin'),executable_sha256=sha(replay/'original-hardware')))+'\n')
    dump=scratch/'debugger-heap.bin';script=GDB.replace('DEBUGGEE',str(scratch/'debuggee-output.bin')).replace('HEAP_DUMP',str(dump)).replace('HEAP_END',hex(heap['address']+heap['size'])).replace('HEAP_FIRST',hex(heap['address'])).replace('HEAP_LIMIT',hex(heap['address']+heap['size'])).replace('SELECTED_READ_ADDRESS',hex(a.selected_read_address)).replace('SELECTED_WIDTH',str(a.selected_width)).replace('AUXILIARY_BREAK','ConsumerGate('+hex(a.consumer_gate)+')'if a.consumer_gate else 'PrivateAccess('+hex(a.selected_read_address)+')'if a.watch_access else 'CopyRecord(0x303ba0)')
    commands=scratch/'lifetime.gdb';commands.write_text(script);debugger_command=['gdb','-q','-nx','-batch','-x',str(commands),str(replay/'original-hardware')]
    debugger=subprocess.run(debugger_command,capture_output=True,text=True,timeout=120);log=debugger.stdout+'\nSTDERR:\n'+debugger.stderr;assert len(log.encode())<1024*1024;(a.report_output/'lifetime.log').write_text(log)
    lines=[line for line in debugger.stdout.splitlines()if line.startswith('CONTACT_ALLOCATION_EVENTS=')];assert debugger.returncode==0 and len(lines)==1,(debugger.returncode,log[-3000:]);events=json.loads(lines[0].split('=',1)[1])
    raw=(replay/'output.bin').read_bytes();offset=276
    for region in sorted(d['regions'],key=lambda r:r['address']):
        if region['address']==heap['address']:break
        offset+=region['size']
    original=raw[offset:offset+heap['size']];watched=dump.read_bytes();assert original==watched,'Hardware-debugged heap differs from ordinary original replay'
    final=next(e for e in events if e['kind']=='root_return')['allocations'];freed={e['pointer']:e['payload']for e in events if e['kind']=='free_return'}
    assert all(e['payload']==freed[e['pointer']]for e in final),'Freed contact payload changed after its LAST free before frame return'
    native_before=(a.capture/heap['before_file']).read_bytes();native_after=(a.capture/heap.get('after_file',heap['before_file'])).read_bytes()
    dif=json.loads(a.differences.read_text());coverage=[]
    for allocation in final:
        first,size=allocation['pointer'],allocation['size']
        ranges=[r for r in dif['exact_differences']if first<=int(r['first'],16)<=int(r['last'],16)<first+size]
        start=first-heap['address'];valid=[]
        for n in range(size//0x80):
            p=start+n*0x80;valid.append(dict(index=n,body=struct.unpack_from('<I',native_after,p+0x70)[0],component=struct.unpack_from('<I',native_after,p+0x74)[0],key=struct.unpack_from('<I',native_after,p+0x78)[0],native_valid=native_after[p+0x7d],original_valid=original[p+0x7d]))
        coverage.append(dict(first=first,size=size,ranges=len(ranges),different_bytes=sum(r['bytes']for r in ranges),differences=[dict(first=r['first'],last=r['last'],bytes=r['bytes'],index=(int(r['first'],16)-first)//0x80,offset=hex((int(r['first'],16)-first)%0x80))for r in ranges],records=valid))
    assert sum(r['different_bytes']for r in coverage)==dif['total_different_bytes'] and sum(r['ranges']for r in coverage)==dif['different_ranges']
    invalid_only=all(c['records'][r['index']]['native_valid']==c['records'][r['index']]['original_valid']==0 for c in coverage for r in c['differences'])
    metadata_equality=[]
    for address in sorted({event['metadata'][name]for event in final for name in('head_address','in_use_address','free_address')}):
        n=address-heap['address'];values=[struct.unpack_from('<I',bank,n)[0]for bank in(native_before,native_after,original)];assert values[1]==values[2],'Native/original final allocator metadata differs';metadata_equality.append(dict(address=address,native_before=values[0],native_after=values[1],original_after=values[2]))
    report=dict(completed=True,image_sha256=NEW_PIN,capture=str(a.capture),capture_metadata_sha256=sha(a.capture/'capture.json'),reference_compare_exit=reference.returncode,reference_output_reused=a.reuse_replay,reference_input_pin=input_pin,debugger_exit=debugger.returncode,checked_heap_bytes=len(original),plain_and_debugged_heap_sha256=hashlib.sha256(original).hexdigest(),events=events,coverage=coverage,all_differences_in_invalid_records=invalid_only,allocation_returns=sum(e['kind']=='allocate_return'for e in events),free_returns=sum(e['kind']=='free_return'for e in events),selected_private_lane_accesses=sum(e['kind']=='selected_private_lane_access'for e in events),selected_read_address=a.selected_read_address,selected_width=a.selected_width,consumer_gate=a.consumer_gate,observation_mode='consumer_gate'if a.consumer_gate else 'access_watch'if a.watch_access else 'record_copy',metadata_equality=metadata_equality,replay_command=command,debugger_command=debugger_command,owned_scratch=str(scratch),source_hashes={str(Path(__file__).relative_to(ROOT)):sha(__file__)},temporary_allocated_bytes_before_owner_cleanup=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file()),retained_large_artifacts=[],scope='Original selected capture allocation/copy/free or one selected lane access-watch/consumer gate diagnosis only. Existing strict raw gate remains RED and unchanged; no paired native lifetime or complete semantic read closure claimed.')
    (a.report_output/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(dict(completed=True,events=len(events),checked_heap_bytes=len(original),different_bytes=dif['total_different_bytes'],raw_comparison_passed=False)))
if __name__=='__main__':main()
