#!/usr/bin/env python3
"""Checked original contact-array growth/free lifetime for retained v1458 input.

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
events=[];tracked={}
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
        if stride==0x80 and pointer in(0x10082380,0x10092380):
            header=reg('esi');tracked[pointer]=dict(header=header,size=stride*capacity)
            emit('allocate_return',pointer=pointer,capacity=capacity,stride=stride,header=header,header_words=words(header,3),metadata=metadata(stride*capacity))
            if capacity==16:
                self.enabled=False;PoolAllocation(0x233250)
        return False
class PoolAllocation(Break):
    def stop(self):
        args=words(reg('esp'),3)
        if 0x200<args[1]<=0x800:emit('pool_allocation_after_second_growth',arguments=args,allocator=reg('ecx'))
        return False
class FreeReturn(Break):
    def __init__(self,address,pointer,size):self.pointer=pointer;self.size=size;super().__init__(address,True)
    def stop(self):
        emit('free_return',pointer=self.pointer,size=self.size,metadata=metadata(self.size),payload=raw(self.pointer,self.size).hex())
        self.enabled=False
        return False
class Free(Break):
    def stop(self):
        args=words(reg('esp'),4);ret,pointer,size,tag=args
        if pointer in tracked:
            emit('free_entry',pointer=pointer,size=size,tag=tag,actual_return=ret,header_words=words(tracked[pointer]['header'],3),metadata=metadata(size),payload=raw(pointer,size).hex())
            FreeReturn(ret,pointer,size)
        return False
class RootReturn(Break):
    def stop(self):
        final=[]
        for pointer,record in sorted(tracked.items()):final.append(dict(pointer=pointer,size=record['size'],metadata=metadata(record['size']),payload=raw(pointer,record['size']).hex()))
        emit('root_return',allocations=final)
        gdb.write('CONTACT_ALLOCATION_EVENTS='+json.dumps(events,sort_keys=True)+'\n')
        gdb.execute('dump binary memory HEAP_DUMP 0x10060000 HEAP_END')
        gdb.execute('quit');return True
Growth(0x2e327e);Free(0x149e00);RootReturn(0x1ccaca)
end
continue
'''
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--capture',type=Path,required=True);p.add_argument('--differences',type=Path,required=True);p.add_argument('--report-output',type=Path,required=True);a=p.parse_args();scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();a.capture=a.capture.resolve();a.report_output.mkdir(parents=True,exist_ok=True)
    assert sha(NEW_DEFAULT)==NEW_PIN;d=json.loads((a.capture/'capture.json').read_text());heap=next(r for r in d['regions']if r['address']==0x10060000);replay=scratch/'replay'
    command=[sys.executable,str(ROOT/'tools/replay_live_phantom_hardware_tu15.py'),'--native-frame-semantics','--return-breakpoints','--capture',str(a.capture),'--output',str(replay)]
    reference=subprocess.run(command,capture_output=True,text=True);assert(replay/'output.bin').exists(),reference.stderr[-2000:]
    dump=scratch/'debugger-heap.bin';script=GDB.replace('DEBUGGEE',str(scratch/'debuggee-output.bin')).replace('HEAP_DUMP',str(dump)).replace('HEAP_END',hex(heap['address']+heap['size']))
    commands=scratch/'lifetime.gdb';commands.write_text(script);debugger_command=['gdb','-q','-nx','-batch','-x',str(commands),str(replay/'original-hardware')]
    debugger=subprocess.run(debugger_command,capture_output=True,text=True,timeout=120);log=debugger.stdout+'\nSTDERR:\n'+debugger.stderr;assert len(log.encode())<1024*1024;(a.report_output/'lifetime.log').write_text(log)
    lines=[line for line in debugger.stdout.splitlines()if line.startswith('CONTACT_ALLOCATION_EVENTS=')];assert debugger.returncode==0 and len(lines)==1,(debugger.returncode,log[-3000:]);events=json.loads(lines[0].split('=',1)[1])
    raw=(replay/'output.bin').read_bytes();offset=276
    for region in sorted(d['regions'],key=lambda r:r['address']):
        if region['address']==heap['address']:break
        offset+=region['size']
    original=raw[offset:offset+heap['size']];watched=dump.read_bytes();assert original==watched,'Hardware-debugged heap differs from ordinary original replay'
    final=next(e for e in events if e['kind']=='root_return')['allocations'];freed={e['pointer']:e['payload']for e in events if e['kind']=='free_return'}
    assert all(e['payload']==freed[e['pointer']]for e in final),'Freed contact payload changed before frame return'
    native_before=(a.capture/heap['before_file']).read_bytes();native_after=(a.capture/heap.get('after_file',heap['before_file'])).read_bytes()
    dif=json.loads(a.differences.read_text());coverage=[]
    for first,size in((0x10082380,0x400),(0x10092380,0x800)):
        ranges=[r for r in dif['exact_differences']if first<=int(r['first'],16)<=int(r['last'],16)<first+size]
        start=first-heap['address'];valid=[]
        for n in range(size//0x80):
            p=start+n*0x80;valid.append(dict(index=n,body=struct.unpack_from('<I',native_after,p+0x70)[0],component=struct.unpack_from('<I',native_after,p+0x74)[0],key=struct.unpack_from('<I',native_after,p+0x78)[0],native_valid=native_after[p+0x7d],original_valid=original[p+0x7d]))
        coverage.append(dict(first=first,size=size,ranges=len(ranges),different_bytes=sum(r['bytes']for r in ranges),differences=[dict(first=r['first'],last=r['last'],bytes=r['bytes'],index=(int(r['first'],16)-first)//0x80,offset=hex((int(r['first'],16)-first)%0x80))for r in ranges],records=valid))
    assert sum(r['different_bytes']for r in coverage)==dif['total_different_bytes']==307 and sum(r['ranges']for r in coverage)==73
    invalid_only=all(c['records'][r['index']]['native_valid']==c['records'][r['index']]['original_valid']==0 for c in coverage for r in c['differences']);assert invalid_only
    metadata_equality=[]
    for address in(0x10076098,0x1007635c,0x101663fc,0x1007609c,0x10076360,0x10166400):
        n=address-heap['address'];values=[struct.unpack_from('<I',bank,n)[0]for bank in(native_before,native_after,original)];assert len(set(values))==1;metadata_equality.append(dict(address=address,native_before=values[0],native_after=values[1],original_after=values[2]))
    report=dict(completed=True,image_sha256=NEW_PIN,capture=str(a.capture),capture_metadata_sha256=sha(a.capture/'capture.json'),reference_compare_exit=reference.returncode,debugger_exit=debugger.returncode,checked_heap_bytes=len(original),plain_and_debugged_heap_sha256=hashlib.sha256(original).hexdigest(),events=events,coverage=coverage,all_differences_in_invalid_records=invalid_only,pool_allocations_after_second_growth=sum(e['kind']=='pool_allocation_after_second_growth'for e in events),metadata_equality=metadata_equality,replay_command=command,debugger_command=debugger_command,owned_scratch=str(scratch),source_hashes={str(Path(__file__).relative_to(ROOT)):sha(__file__)},retained_large_artifacts=[],scope='Source-owned allocation lifetime diagnosis only; raw307-byte/73-range gate remains RED and unchanged')
    (a.report_output/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(dict(completed=True,events=len(events),checked_heap_bytes=len(original),different_bytes=307,raw_comparison_passed=False)))
if __name__=='__main__':main()
