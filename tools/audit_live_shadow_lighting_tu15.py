#!/usr/bin/env python3
"""Read-only unchanged TU1.5 light-query audit on retained native shadow inputs.

The chosen actual actor and force-query1 are declared diagnostic inputs. No
query result/callee/instruction is replaced. This does not prove a matched
original gameplay caller or complete shadow rendering.
"""
import argparse,hashlib,json,struct
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_INVALID,UcError
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import Xbe
from generate_combat_action_tu15 import CombatCode
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from differential_physics_native_frame_tu15_20261012 import regions,segment


def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--case',type=Path,required=True);p.add_argument('--output',type=Path,required=True)
 p.add_argument('--reference-device-control',type=lambda v:int(v,0));a=p.parse_args();assert not a.output.exists();assert hashlib.sha256(NEW_DEFAULT.read_bytes()).hexdigest()==NEW_PIN
 initial=list(regions(a.case/'before.bin'));abi=json.loads((a.case/'before-abi.json').read_text());owners=json.loads((a.case/'selected-root-cache.json').read_text())['roots']
 image=Xbe(NEW_DEFAULT);code=CombatCode(image);reports=[];a.output.mkdir(parents=True)
 for owner in owners:
  u=Uc(UC_ARCH_X86,UC_MODE_32);mapped=set()
  def mount(address,data):
   pages=[p for p in range(address&~4095,(address+len(data)+4095)&~4095,4096)if p not in mapped]
   start=0
   while start<len(pages):
    end=start+1
    while end<len(pages) and pages[end]==pages[end-1]+4096:end+=1
    u.mem_map(pages[start],(end-start)*4096);start=end
   mapped.update(pages);u.mem_write(address,data)
  for address,data in initial:mount(address,data)
  if a.reference_device_control is not None:
   # Same isolated CPU metadata adapter as the established original model
   # builder oracle. This does not supply a sample, collision or query result.
   # Compare distinct declared values to test the supposedly dead SDK field.
   assert 0x70002000 not in mapped
   mount(0x70002000,bytes(0x3000));u.mem_write(0x411568,struct.pack('<I',0x70002000))
   u.mem_write(0x70003a14,struct.pack('<I',0x70004000));u.mem_write(0x70004004,struct.pack('<I',a.reference_device_control))
  gdt=0x7fff0000;assert gdt not in mapped;mount(gdt,bytes(4096));u.mem_write(gdt+8,segment(0,0x9b)+segment(0,0x93)+segment(abi['fs_base'],0x93));u.reg_write(R.UC_X86_REG_GDTR,(0,gdt,4095,0))
  for r,v in ((R.UC_X86_REG_CS,8),(R.UC_X86_REG_DS,16),(R.UC_X86_REG_ES,16),(R.UC_X86_REG_SS,16),(R.UC_X86_REG_FS,24)):u.reg_write(r,v)
  w=lambda p:int.from_bytes(u.mem_read(p,4),'little')
  cache=w(w(0x515f14)+0x44)+(owner['cache_token']&65535)*256;output=cache+0x30
  gp=abi['gp'].copy();gp[7]=owner['identity'];sp=gp[4];stop=w(sp)
  u.mem_write(sp+4,struct.pack('<II',1,output))
  registers=[R.UC_X86_REG_EAX,R.UC_X86_REG_ECX,R.UC_X86_REG_EDX,R.UC_X86_REG_EBX,R.UC_X86_REG_ESP,R.UC_X86_REG_EBP,R.UC_X86_REG_ESI,R.UC_X86_REG_EDI]
  for r,v in zip(registers,gp):u.reg_write(r,v)
  for n,lanes in enumerate(abi['xmm']):u.reg_write(R.UC_X86_REG_XMM0+n,sum(v<<(32*k)for k,v in enumerate(lanes)))
  for r,v in ((R.UC_X86_REG_EFLAGS,abi['eflags']),(R.UC_X86_REG_FPCW,abi['x87_control']),(R.UC_X86_REG_FPSW,abi['x87_status']),(R.UC_X86_REG_FPTAG,0xffff),(R.UC_X86_REG_MXCSR,abi['mxcsr'])):u.reg_write(r,v)
  trace=[];calls=[];query_status=[];decoded={}
  def observe(cpu,pc,size,_):
   assert bytes(cpu.mem_read(pc,size))==image.read(pc,size)
   trace.append(pc)
   if pc not in decoded:decoded[pc]=code.at(pc)
   i=decoded[pc];assert i is not None,hex(pc)
   if i.mnemonic=='call':calls.append(dict(site=pc,operand=i.op_str))
   if pc in (0xd3e55,0xd3e88):query_status.append(dict(site=pc,value=cpu.reg_read(R.UC_X86_REG_EAX)))
  faults=[]
  def fault(cpu,access,address,size,value,_):
   faults.append(dict(pc=cpu.reg_read(R.UC_X86_REG_EIP),access=access,address=address,size=size,gp=[cpu.reg_read(r)for r in registers]));return False
  u.hook_add(UC_HOOK_CODE,observe);u.hook_add(UC_HOOK_MEM_INVALID,fault)
  error=None
  try:u.emu_start(0xd3d90,stop,count=250000)
  except UcError as exception:error=str(exception)
  completed=u.reg_read(R.UC_X86_REG_EIP)==stop
  r=dict(owner=owner['identity'],entry=0xd3d90,force_query=1,output=output,completed=completed,error=error,faults=faults,
         accepted=bool(u.reg_read(R.UC_X86_REG_EAX)&255)if completed else None,instructions=len(trace),query_status=query_status,calls=calls,
         output_words=list(struct.unpack('<5I',u.mem_read(output,20))))
  reports.append(r);(a.output/f"{owner['identity']}-instructions.json").write_text(json.dumps(trace)+'\n')
 result=dict(scope=__doc__,reference_device_control=a.reference_device_control,original_sha256=NEW_PIN,tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),actors=reports,instruction_patches=[],replaced_services=[],full_goal_complete=False)
 (a.output/'report.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(reports))
 if any(not r['completed']for r in reports):raise RuntimeError('Original query has an unresolved input boundary; retained report is not acceptance')


if __name__=='__main__':main()
