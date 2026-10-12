#!/usr/bin/env python3
"""Original1221B0 raw-bank prefix controls; stop before first XAPI constructor.

Actual owning input plus explicit raw-storage/counter variants only. No handle,
platform status, worker, TLS, resource or final Unit field is supplied.
"""
import argparse,hashlib,json,struct,sys
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
from unicorn.x86_const import *
from oracle_mp_animation_residency_1_1 import ROOT,encoded,sha
sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--source',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=True)
 d=json.loads(a.source.read_text());assert sha(a.source)=='e1c8420ce6fc4abf6bcd9626cb0294a66847594a57fd68c72db21bc064509304'
 s=d['stages'][-1];image=Xbe(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03');assert sha(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03')==d['source_sha256']
 base=0x4e7924;extent=0x25a0;raw=bytes.fromhex(s['before']['async_raw_state']);assert len(raw)==extent
 variants=[('actual_owning_prefix',raw)]
 edges=[(0,0),(1,0xffffffff),(0xffffffff,1),(0x80000000,0x7fffffff),(0x7fffffff,0x80000000),(0x80000001,0xfffffffe)]
 for i,(counter,added)in enumerate(edges):
  b=bytearray([((i+1)*37)&255]*extent);struct.pack_into('<I',b,0x4e7934-base,counter);struct.pack_into('<I',b,0x4e9ec0-base,added);variants.append((f'explicit_raw_storage_and_counters_{i}',bytes(b)))
 cases=[];reached={}
 for name,before in variants:
  u=Uc(UC_ARCH_X86,UC_MODE_32)
  for section in image.sections:
   if section.name!='.text':continue
   lo=section.va&~4095;length=(section.va+section.vsize-lo+4095)&~4095;u.mem_map(lo,length);u.mem_write(section.va,image.section_bytes(section))
  u.mem_map(base&~4095,0x3000);u.mem_write(base,before)
  stack=0xa100800;u.mem_map(0xa100000,4096);u.mem_write(stack,struct.pack('<I',0xa200000));u.mem_map(0xa200000,4096)
  regs=s['registers_before']
  for key,reg in [('eax',UC_X86_REG_EAX),('ecx',UC_X86_REG_ECX),('edx',UC_X86_REG_EDX),('ebx',UC_X86_REG_EBX),('ebp',UC_X86_REG_EBP),('esi',UC_X86_REG_ESI),('edi',UC_X86_REG_EDI)]:u.reg_write(reg,int(regs[key],16))
  u.reg_write(UC_X86_REG_ESP,stack);u.reg_write(UC_X86_REG_EFLAGS,2);stores=[];stop={}
  def code(g,pc,n,data):
   assert bytes(g.mem_read(pc,n))==image.read(pc,n)
   reached[pc]=n
   if pc==0x2dc83a:
    sp=g.reg_read(UC_X86_REG_ESP);stop.update(entry=f'{pc:08x}',args=struct.unpack('<3I',bytes(g.mem_read(sp+4,12))));g.emu_stop()
  def write(g,access,address,n,value,data):
   if base<=address<base+extent:
    assert n==4;stores.append(dict(instruction=f'{g.reg_read(UC_X86_REG_EIP):08x}',address=f'{address:08x}',before=bytes(g.mem_read(address,n)).hex(),written=(value&0xffffffff).to_bytes(4,'little').hex()))
  u.hook_add(UC_HOOK_CODE,code);u.hook_add(UC_HOOK_MEM_WRITE,write);u.emu_start(0x1221b0,0xa200000,count=10000)
  assert stop==dict(entry='002dc83a',args=(0,0,0));after=bytes(u.mem_read(base,extent));assert len(stores)==2703
  assert after[0x4e7934-base:0x4e7938-base]==before[0x4e7934-base:0x4e7938-base]
  assert after[0x4e9ec0-base:0x4e9ec4-base]==before[0x4e9ec0-base:0x4e9ec4-base]
  if name=='actual_owning_prefix':
   assert after.hex()==s['after']['async_raw_state']
   owning=[q for q in d['resource_stores'][d['first_new_store']:]if base<=int(q['address'],16)<base+extent]
   assert [{k:q[k]for k in('instruction','address','before','written')}for q in owning]==stores
  cases.append(dict(name=name,before_hex=before.hex(),after_hex=after.hex(),stores=stores,stop=stop))
 listing=''.join(f'{pc:08x} {image.read(pc,n).hex()}\n'for pc,n in sorted(reached.items()))
 (a.output/'reached.hex').write_text(listing)
 v=dict(schema='animation-async-raw-prefix-controls-v1',tool_sha256=sha(__file__),owning_source_sha256=sha(a.source),source_sha256=d['source_sha256'],cases=cases,reached_instructions=len(reached),initial_execution_registers=s['registers_before'],initial_eflags=2,execution_storage='Fresh explicitly owned Unicorn code/bank/stack provider; actual input first, separately declared raw-storage/counter variants. No platform result, handle or Unit field.',instruction_patches=[],supplied_success=[])
 (a.output/'report.json').write_bytes(encoded(v));print(json.dumps(dict(output=str(a.output/'report.json'),sha256=sha(a.output/'report.json'),cases=len(cases),stores=sum(len(q['stores'])for q in cases))))
if __name__=='__main__':main()
