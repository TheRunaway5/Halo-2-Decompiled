#!/usr/bin/env python3
"""Hardware-only lifetime diagnostics for original TU1.5 contact scratch.

Run under run_test.py. Uses unchanged original text and retained failure inputs;
keeps bounded JSON/log diagnostics only, never rewrites comparison exclusions.
"""
import argparse, hashlib, json, os, subprocess, sys
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN

def sha(path):
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()

GDB = r'''
set pagination off
set confirm off
set disable-randomization off
set debuginfod enabled off
run > OUTPUT
python
import gdb,json,struct
events=[]
def reg(name):return int(gdb.parse_and_eval('$'+name))&0xffffffff
def words(p,n):return list(struct.unpack('<'+'I'*n,gdb.selected_inferior().read_memory(p,n*4)))
def pool_metadata():
    pool=words(0x48606c,1)[0];vtable=words(pool,1)[0];size_class=words(pool+0x2b0,1)[0]
    return dict(pool=pool,vtable=vtable,size_class=size_class,free_head_address=pool+0x24+size_class*4,
        free_head=words(pool+0x24+size_class*4,1)[0],in_use_address=pool+0x2e8+size_class*4,
        in_use=words(pool+0x2e8+size_class*4,1)[0],free_count_address=pool+0xf0388+size_class*4,
        free_count=words(pool+0xf0388+size_class*4,1)[0])
def emit(kind,**kw):
    item=dict(kind=kind,pc=reg('eip'),esp=reg('esp'),ebp=reg('ebp'),**kw)
    events.append(item);gdb.write(json.dumps(item,sort_keys=True)+'\n')
class Stop(gdb.Breakpoint):
    def __init__(self,address):super().__init__('*0x%x'%address,type=gdb.BP_HARDWARE_BREAKPOINT)
class Construct(Stop):
    def stop(self):
        src=words(reg('esp')+4,1)[0];dst=reg('ecx')
        emit('copy_uninitialized_private_record',source=src,destination=dst,source_words=words(src,32))
        return False
class Grow(Stop):
    def stop(self):
        array=reg('esi');new=reg('eax')
        emit('array_allocate_return',array=array,array_words=words(array,3),allocated=new,capacity=reg('ebp'),stride=reg('ebx'),heap_prefix=words(new-0x40,20),pool_metadata=pool_metadata())
        return False
class AllocationAfterFree(Stop):
    def stop(self):
        emit('allocator_allocate_after_contact_free',allocator=reg('ecx'),arguments=words(reg('esp'),3))
        return False
class Freed(Stop):
    def stop(self):
        emit('array_free_return',heap_prefix=words(0x10082340,20),heap_words=words(0x10082380,256),pool_metadata=pool_metadata())
        self.enabled=False
        allocator=words(0x48c938,1)[0];vtable=words(allocator,1)[0]
        AllocationAfterFree(words(vtable+0x10,1)[0])
        pool=words(0x48606c,1)[0];pool_vtable=words(pool,1)[0]
        AllocationAfterFree(words(pool_vtable+0x10,1)[0])
        return False
class Free(Stop):
    def stop(self):
        allocator=reg('ecx');vtable=words(allocator,1)[0]
        emit('array_free_call',allocator=allocator,vtable=vtable,vtable_words=words(vtable,12),arguments=words(reg('esp'),3),array_words=words(reg('esp')+0x11c,3),heap_prefix=words(0x10082340,20),heap_words=words(0x10082380,256),pool_metadata=pool_metadata())
        constructor.enabled=False;growth.enabled=False;self.enabled=False
        Freed(0x304aa3)
        return False
class RootReturn(Stop):
    def stop(self):
        emit('root_return',heap_prefix=words(0x10082340,20),heap_words=words(0x10082380,256),pool_metadata=pool_metadata())
        gdb.write('CONTACT_LIFETIME_EVENTS='+json.dumps(events,sort_keys=True)+'\n')
        gdb.execute('quit')
        return True
constructor=Construct(0x303ba0);growth=Grow(0x2e327e);freer=Free(0x304aa0);root=RootReturn(0x1ccaca)
end
continue
'''

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--capture',type=Path,required=True)
    p.add_argument('--report-output',type=Path,required=True)
    a=p.parse_args();scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    a.capture=a.capture.resolve();a.report_output.mkdir(parents=True,exist_ok=True)
    assert sha(NEW_DEFAULT)==NEW_PIN
    replay=scratch/'replay'
    command=[sys.executable,str(ROOT/'tools/replay_live_phantom_hardware_tu15.py'),'--native-frame-semantics','--return-breakpoints','--capture',str(a.capture),'--output',str(replay)]
    comparison=subprocess.run(command,capture_output=True,text=True)
    assert (replay/'original-hardware').exists(),comparison.stderr[-2000:]
    commands=scratch/'contact-lifetime.gdb'
    commands.write_text(GDB.replace('OUTPUT','"'+str(scratch/'debuggee-output.bin')+'"'))
    debugger_command=['gdb','-q','-nx','-batch','-x',str(commands),str(replay/'original-hardware')]
    result=subprocess.run(debugger_command,capture_output=True,text=True,timeout=120)
    log=result.stdout+'\nSTDERR:\n'+result.stderr
    assert len(log.encode())<1024*1024
    (a.report_output/'lifetime.log').write_text(log)
    lines=[line for line in result.stdout.splitlines() if line.startswith('CONTACT_LIFETIME_EVENTS=')]
    assert result.returncode==0 and len(lines)==1,(result.returncode,log[-3000:])
    events=json.loads(lines[0].split('=',1)[1])
    freed=[e for e in events if e['kind']=='array_free_call']
    returned=[e for e in events if e['kind']=='array_free_return']
    root=[e for e in events if e['kind']=='root_return']
    assert len(freed)==len(returned)==len(root)==1
    assert freed[0]['arguments']==[0x10082380,0x400,0x12],freed
    assert returned[0]['heap_words']==root[0]['heap_words']
    assert returned[0]['pool_metadata']==root[0]['pool_metadata']
    assert returned[0]['pool_metadata']['free_head']==0x10082380
    differences=json.loads((a.report_output.parent/'lower0-byte-difference-diagnostic/differences.json').read_text())['exact_differences']
    assert len(differences)==22 and sum(e['bytes'] for e in differences)==95
    coverage=[dict(first=e['first'],last=e['last'],bytes=e['bytes'],record=(int(e['first'],16)-0x10082380)//0x80,
        record_offset=hex((int(e['first'],16)-0x10082380)%0x80))for e in differences]
    assert all(0x10082380<=int(e['first'],16)<=int(e['last'],16)<0x10082780 for e in differences)
    report=dict(completed=True,image_sha256=NEW_PIN,capture=str(a.capture),capture_metadata_sha256=sha(a.capture/'capture.json'),events=events,difference_coverage=coverage,reference_compare_exit=comparison.returncode,debugger_exit=result.returncode,
        replay_command=command,debugger_command=debugger_command,source_hashes={str(Path(__file__).relative_to(ROOT)):sha(__file__)},retained_large_artifacts=[],
        scope='Unchanged physical original contact-record copy, growth, free and subsequent same-allocator allocation diagnostics. Raw-byte qualification remains unchanged.')
    (a.report_output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(completed=True,events=len(events),allocations_after_free=sum(e['kind']=='allocator_allocate_after_contact_free' for e in events),reference_compare_exit=comparison.returncode)))

if __name__=='__main__':main()
