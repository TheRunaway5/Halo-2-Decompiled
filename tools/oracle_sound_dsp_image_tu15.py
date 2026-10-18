#!/usr/bin/env python3
"""Decode the owned TU1.5 DSP resource with unchanged original CPU routines.

Explicit mutable byte-buffer ABI, with the original image's descriptor/seed
table and encrypted effect payloads. No device, allocation, DMA, DSP execution,
hardware address relocation or platform return is supplied.
"""
import argparse,hashlib,json,struct
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_READ,UC_HOOK_MEM_WRITE,UC_MEM_WRITE
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False)
 x=Xbe(NEW_DEFAULT);assert hashlib.sha256(x.data).hexdigest()==NEW_PIN
 raw=x.section_bytes(next(s for s in x.sections if s.name=='DSPImage'));words=lambda at:struct.unpack_from('<I',raw,at)[0]
 metadata=0x818+4*(words(0x804)+words(0x80c));count=words(metadata);table_size=(count-1)*32+40;table=metadata+table_size
 assert count==15 and 0x818<table<len(raw) and table+count*8<=len(raw)
 g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x600000)
 for s in x.sections:g.mem_write(s.va,x.section_bytes(s))
 base,stack,stop=0x1000000,0x10f0000,0x10fe000;g.mem_map(base,0x100000)
 buffer,context=base+0x1000,base+0x9000
 g.mem_write(buffer,raw)
 gp_names=('EAX','ECX','EDX','EBX','ESP','EBP','ESI','EDI')
 gp=lambda:[g.reg_read(getattr(R,'UC_X86_REG_'+n))for n in gp_names]
 xmm=lambda:[list(struct.unpack('<4I',g.reg_read(getattr(R,f'UC_X86_REG_XMM{i}')).to_bytes(16,'little')))for i in range(8)]
 recording=False;initial={};store_hash=hashlib.sha256();store_count=0;executed=set();cases=[]
 def ins(machine,pc,size,user):
  if pc not in executed:assert bytes(machine.mem_read(pc,size))==x.read(pc,size);executed.add(pc)
 def access(machine,kind,address,size,value,user):
  nonlocal store_count
  if not recording:return
  before=bytes(machine.mem_read(address,size))
  for i,b in enumerate(before):initial.setdefault(address+i,b)
  if kind==UC_MEM_WRITE:
   written=(value&((1<<(8*size))-1)).to_bytes(size,'little')
   store_hash.update(struct.pack('<3I',machine.reg_read(R.UC_X86_REG_EIP),address,size));store_hash.update(before);store_hash.update(written);store_count+=1
 g.hook_add(UC_HOOK_CODE,ins);g.hook_add(UC_HOOK_MEM_READ|UC_HOOK_MEM_WRITE,access)
 def banks():
  out={};start=None;last=None;data=bytearray()
  for at,b in sorted(initial.items()):
   if last is not None and at!=last+1:out[f'{start:08x}']=data.hex();data=bytearray();start=None
   if start is None:start=at
   data.append(b);last=at
  if start is not None:out[f'{start:08x}']=data.hex()
  return out
 def run(name,entry,args,end=stop):
  nonlocal recording,store_hash,store_count
  initial.clear();store_hash=hashlib.sha256();store_count=0;g.mem_write(stack,struct.pack('<'+'I'*(1+len(args)),stop,*args))
  for reg,v in((R.UC_X86_REG_ESP,stack),(R.UC_X86_REG_EFLAGS,2),(R.UC_X86_REG_FPCW,0x027f),(R.UC_X86_REG_FPSW,0),(R.UC_X86_REG_FPTAG,0xffff),(R.UC_X86_REG_MXCSR,0x1f80)):g.reg_write(reg,v)
  before_gp,before_xmm=gp(),xmm();recording=True
  try:g.emu_start(entry,end,count=5000000)
  finally:recording=False
  assert g.reg_read(R.UC_X86_REG_EIP)==end
  before=banks();after={at:bytes(g.mem_read(int(at,16),len(value)//2)).hex()for at,value in before.items()}
  row=dict(name=name,entry=entry,gp=before_gp,xmm=before_xmm,gp_after=gp(),xmm_after=xmm(),eflags_after=g.reg_read(R.UC_X86_REG_EFLAGS),defined_flags_mask=0x801 if end==0x38e09a else 0xcc5,before=before,after=after,store_sha256=store_hash.hexdigest(),store_count=store_count,store_hash_encoding='LE32 pc,address,extent followed by before and written bytes in execution order',external_boundary=[]if end==stop else[end],external_targets=[]if end==stop else[end])
  filename=f'{len(cases):03d}-{name}.json';(a.output/filename).write_text(json.dumps(row,separators=(',',':'))+'\n');cases.append(dict(file=filename))
 run('initialize',0x38dfd2,(context,))
 # Stop after the actual F6 EB / IMUL BL. Its intermediate AX and retained
 # upper registers are observable even though later code consumes only AL.
 run('multiply_prefix',0x38dfff,(context,buffer+table,count*8,buffer+table,0),0x38e09a)
 run('seed_table',0x38dfff,(context,buffer+table,count*8,buffer+table,0))
 effects=[];position=0x818
 for i in range(count):
  size=words(metadata+8+i*32+4);assert size and position+size<=metadata
  run(f'effect_{i:02d}',0x38dfff,(buffer+table+i*8,buffer+position,size,buffer+position,0))
  decoded=bytes(g.mem_read(buffer+position,size));(a.output/f'effect-{i:02d}.bin').write_bytes(decoded)
  effects.append(dict(index=i,section_offset=position,bytes=size,sha256=hashlib.sha256(decoded).hexdigest(),first64_hex=decoded[:64].hex(),descriptor32_hex=raw[metadata+8+i*32:metadata+40+i*32].hex()))
  position+=size
 # The original loop decodes only each descriptor's byte extent. Do not
 # replace that with the container's aggregate program/data word counts.
 assert position<=metadata
 decoded_resource=bytes(g.mem_read(buffer,len(raw)))
 (a.output/'decoded-resource.bin').write_bytes(decoded_resource)
 report=dict(suite='audio_dsp',scope=__doc__,xbe_sha256=NEW_PIN,dsp_section_sha256=hashlib.sha256(raw).hexdigest(),metadata_offset=metadata,seed_table_offset=table,effects=effects,cases=cases,instruction_patches=[],supplied_external_results=[],external_targets=[],executed_instructions=[f'{v:08x}'for v in sorted(executed)],dsp_execution_verified=False)
 report['decoded_resource_sha256']=hashlib.sha256(decoded_resource).hexdigest()
 (a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(f'PASS original DSP decoder: {len(cases)} calls, {count} actual payloads, {position-0x818} decoded bytes, {len(executed)} unchanged instructions; no supplied returns')
 print('Effect0',effects[0])
if __name__=='__main__':main()
