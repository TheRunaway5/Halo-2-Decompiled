#!/usr/bin/env python3
"""Repeat declared original PIO CPU cases on physical i386, without opcode patches.

FIFO words, caller storage and timed input changes are declared inputs. MMIO
addresses have ordinary host backing used only as write observation sinks;
this is not device completion, playback, or Xbox kernel/hardware acceptance.
"""
import argparse,ctypes,hashlib,json,os,struct,subprocess
from pathlib import Path
from original_audio_cpu import OriginalAudioCpu
from oracle_smg_firing_1_1 import Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN


def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--reference',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
 if a.output.exists():p.error('Preserve physical PIO results')
 scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True);ref=json.loads(a.reference.read_text());image=Xbe(NEW_DEFAULT)
 assert hashlib.sha256(image.data).hexdigest()==NEW_PIN==ref['original_xbe_sha256'];code=_Code(image,'DSOUND')
 layout=scratch/'ptrace-layout.c';layout.write_text('#include <stddef.h>\n#include <sys/user.h>\n_Static_assert(offsetof(struct user,u_debugreg)==848,"Unsupported ptrace ABI");\nint main(void){return 0;}\n')
 subprocess.run(['cc',str(layout),'-o',str(scratch/'ptrace-layout')],check=True);subprocess.run([str(scratch/'ptrace-layout')],check=True)
 host=OriginalAudioCpu(image,scratch/'original32.elf',0x27f,0x1f80,extra_declared_regions=((0xfe800000,0x3000),(0xfe820000,0x3000)),source_bank_bytes=ref['extent'])
 aliases=dict(eax='rax',ecx='rcx',edx='rdx',ebx='rbx',esp='rsp',ebp='rbp',esi='rsi',edi='rdi');names=tuple(aliases.values());visited=set();total_stores=total_actions=total_instructions=0
 try:
  host.trace(6,host.debug_offset+7*8,0)
  def write(address,data):
   # The XBE's DSOUND section includes these mutable pointer globals. Admit
   # only these declared data extents, never an original instruction byte.
   if any(low<=address and address+len(data)<=high for low,high in ((0x391390,0x391490),(0x38f988,0x38f98c))):
    assert all(address+len(data)<=int(pc,16) or address>=int(pc,16)+len(bytes.fromhex(raw))for pc,raw in ref['executed_original_instructions'].items())
    assert os.pwrite(host.memory_fd,data,address)==len(data)
   else:host.write(address,data)
  for row in ref['cases']:
   regions={ref['bank']:bytearray([row['fill']])*ref['extent'],0x391390:bytearray(0x100),0x38f988:bytearray(4)}
   def put_expected(address,value,size=4):
    base=next(at for at,data in regions.items()if at<=address and address+size<=at+len(data));regions[base][address-base:address-base+size]=value.to_bytes(size,'little')
   for address,value in row['initial_memory']:put_expected(address,value)
   for address,data in regions.items():write(address,bytes(data))
   # The external return address is observed before execution, never patched
   # into the original image. All source opcodes remain exactly authenticated.
   host.prepare(row['entry'],row['gp'][4],dict(zip(names,row['gp'])))
   regs=host.registers();regs.eflags=row['eflags'];host.trace(13,0,ctypes.addressof(regs))
   instructions=[];events=[];visits={};counts={};fifo=0
   def changes(kind,pc):
    key=(kind,pc);counts[key]=counts.get(key,0)+1
    for change in row['changes']:
     if (change['kind'],change['pc'],change['occurrence'])==(kind,pc,counts[key]):
      for address,value in change['writes']:
       write(address,struct.pack('<I',value));events.append(dict(kind='input',address=address,value=value))
   for _ in range(10000):
    pc=regs.rip
    if pc==ref['stop']:break
    visits[pc]=visits.get(pc,0)+1
    if row['interrupt']==[pc,visits[pc]]:break
    ins=code.at(pc);assert ins and ins.bytes.hex()==ref['executed_original_instructions'][f'{pc:08x}']
    assert host.read(pc,len(ins.bytes))==ins.bytes
    instructions.append(pc);visited.add(pc);changes('instruction',pc)
    if pc==0x38e0ce:
     host.write(0xfe820010,struct.pack('<I',row['fifo'][fifo]));fifo+=1
    def register(name):return 0 if not name else getattr(regs,aliases[name])&0xffffffff
    pending=[]
    for op in ins.operands:
     if op.type==3 and op.access&2:
      mem=op.mem;address=(register(ins.reg_name(mem.base))+register(ins.reg_name(mem.index))*mem.scale+mem.disp)&0xffffffff
      pending.append((address,op.size))
    if ins.mnemonic=='push':pending.append(((regs.rsp-4)&0xffffffff,4))
    before={address:int.from_bytes(host.read(address,size),'little')for address,size in pending if address<0xfe800000}
    host.trace(9);regs=host.wait_stop()
    for address,size in pending:
     action=address>=0xfe800000;event=dict(kind='action'if action else'store',pc=pc,address=address,bytes=size,value=int.from_bytes(host.read(address,size),'little'))
     if not action:event['before']=before[address]
     events.append(event);changes('store',pc)
   else:raise RuntimeError('Original physical PIO instruction budget')
   name=row['name'];assert regs.rip==row['end_pc'],(name,'return/interruption boundary')
   assert [getattr(regs,r)&0xffffffff for r in names]==row['expected_gp'],(name,'GP registers')
   assert (regs.eflags&row['flags_mask'])==(row['expected_eflags']&row['flags_mask']),(name,'defined flags')
   assert instructions==row['instructions']and events==row['events']and fifo==row['fifo_reads'],(name,'ordered instruction/input/store events')
   for event in row['events']:
    if event['kind']!='action':put_expected(event['address'],event['value'],event.get('bytes',4))
   assert all(host.read(at,len(data))==bytes(data)for at,data in regions.items()),(name,'whole declared memory')
   total_stores+=sum(e['kind']=='store'for e in events);total_actions+=sum(e['kind']=='action'for e in events);total_instructions+=len(instructions)
 finally:host.close()
 assert visited=={int(v,16)for v in ref['executed_original_instructions']}
 out=dict(passed=True,cases=len(ref['cases']),natural_returns=sum(r['returned']for r in ref['cases']),interruption_prefixes=sum(not r['returned']for r in ref['cases']),
  original_instruction_sites=len(visited),executed_instructions=total_instructions,ordered_source_stores=total_stores,ordered_MMIO_sink_writes=total_actions,
  original_xbe_sha256=NEW_PIN,reference_sha256=hashlib.sha256(a.reference.read_bytes()).hexdigest(),tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
  CPU_owner_sha256=hashlib.sha256(Path(__file__).with_name('original_audio_cpu.py').read_bytes()).hexdigest(),backend='Physical Linux x86-64 child, CS23 i386 compatibility, PTRACE_SINGLESTEP',
  original_instruction_patches=[],substituted_callees=[],declared_source_FIFO_and_MMIO_sink_backing=True,device_startup_completion_or_playback_verified=False,retained_large_artifacts=[])
 a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(out,indent=2)+'\n');print(json.dumps(out))


if __name__=='__main__':main()
