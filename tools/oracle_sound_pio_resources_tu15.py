#!/usr/bin/env python3
"""Original bounded PIO CPU prefixes; explicit offline hardware inputs only.

No vendor object is executed. Native8-byte SGE effect expectations are a
separately named vendor dataflow contract, not an emulated Xbox/DSP proof.
"""
import argparse,hashlib,json,struct
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
from unicorn import x86_const as R

STACK=0x2018000
GP=[getattr(R,'UC_X86_REG_'+s) for s in ['EAX','ECX','EDX','EBX','ESP','EBP','ESI','EDI']]
def run(image,storage,fill):
 cpu=Uc(UC_ARCH_X86,UC_MODE_32)
 for p,n in [(0x10000,0x600000),(0x2000000,0x20000),(0xfe800000,0x3000),(0xfe820000,0x3000)]:cpu.mem_map(p,n)
 for section in image.sections:
  raw=image.section_bytes(section)
  if section.va+len(raw)<=0x610000:cpu.mem_write(section.va,raw)
 for row in storage['regions']:cpu.mem_write(row['address'],bytes.fromhex(row['hex']))
 for row in storage['dma_allocations']:
  cpu.mem_map(row['source'],row['capacity']);cpu.mem_write(row['source'],bytes([fill])*row['capacity'])
 cpu.mem_write(0x2000000,bytes([fill])*0x20000)
 cpu.mem_write(STACK,struct.pack('<I',0x38ecb5));cpu.mem_write(0xfe820010,struct.pack('<I',32))
 gp=storage['returned_gp'].copy();gp[4]=STACK;gp[1]=storage['frontier']['source_subobject'];gp[6]=gp[1]
 for r,v in zip(GP,gp):cpu.reg_write(r,v)
 cpu.reg_write(R.UC_X86_REG_EFLAGS,0x202)
 instructions=[];stores=[];original={}
 def hook(m,pc,n,_):
  b=bytes(m.mem_read(pc,n));assert b==image.read(pc,n);instructions.append(pc);original[f'{pc:08x}']=b.hex()
 def store(m,access,p,n,value,_):stores.append([m.reg_read(R.UC_X86_REG_EIP),p,n,value&((1<<(8*n))-1)])
 cpu.hook_add(UC_HOOK_CODE,hook);cpu.hook_add(UC_HOOK_MEM_WRITE,store)
 def result(entry,end,initial):
  return dict(entry=entry,end_exclusive=end,initial_gp=initial,instructions=instructions.copy(),stores=stores.copy(),returned_gp=[cpu.reg_read(r)&0xffffffff for r in GP],returned_eflags=cpu.reg_read(R.UC_X86_REG_EFLAGS),stack_hex=bytes(cpu.mem_read(0x2010000,0x10000)).hex(),dma_regions=[dict(source=r['source'],hex=bytes(cpu.mem_read(r['source'],r['capacity'])).hex()) for r in storage['dma_allocations']])
 cpu.emu_start(0x38e0ce,0x38e140,count=10000);prefix=result(0x38e0ce,0x38e140,gp)
 assert len([s for s in stores if s[1]<0xfe800000])==257
 assert len([s for s in stores if s[1]>=0xfe800000])==6
 # Only the offline original reference executes these five request MOVs.
 # The native component starts separately at its declared804 boundary.
 instructions.clear();stores.clear();cpu.emu_start(0x38e140,0x38e159,count=1000)
 prior=stores.copy();assert len(prior)==5
 initial=[cpu.reg_read(r)&0xffffffff for r in GP];instructions.clear();stores.clear()
 cpu.emu_start(0x38e159,0x38e16d,count=1000);component=result(0x38e159,0x38e16d,initial)
 assert len(instructions)==3 and len(stores)==2
 return dict(fill=fill,prefix=prefix,unowned_reference_only_tracking_stores=prior,input_component=component,executed_original_bytes=original)
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--storage-reference',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
 image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN
 storage=next(c for c in json.loads(a.storage_reference.read_text())['cases'] if c['through_storage'])
 cases=[run(image,storage,f) for f in [0,0x5a,0xa5]]
 result=dict(passed=True,schema='H2_ORIGINAL_PIO_PREFIX_V1',original_xbe_sha256=NEW_PIN,tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),storage_reference_sha256=hashlib.sha256(a.storage_reference.read_bytes()).hexdigest(),cases=cases,offline_declared_fifo_payload_capacity=32,native_fifo_or_hardware_result=False,instruction_patches=[],whole38e0ce_returned=False,whole38e8c9_returned=False,initializer45_complete=False)
 a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(dict(cases=len(cases),prefix_instructions=len(cases[0]['prefix']['instructions']),prefix_stores=len(cases[0]['prefix']['stores']),input_instructions=len(cases[0]['input_component']['instructions']))))
if __name__=='__main__':main()
