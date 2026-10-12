#!/usr/bin/env python3
"""Complete unchanged16B840 pool allocation over actual and explicit raw banks.

This source-only primitive proof supplies conditional metadata, not allocator
pressure/history or a native image binding. It performs no kernel or GPU call.
The reservation oracle separately proves actual13D370 caller composition.
"""
from pathlib import Path
import hashlib,json,re,sys
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_UNMAPPED,UC_HOOK_MEM_WRITE
from unicorn.x86_const import *
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'tools'))
from differential_source_object_admission_snapshot import Snapshot,PIN,uint,words,packed
sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'));from xbe import Xbe
SYNTH,STACK,STOP=0x20000000,0x200fe000,0x200ff000
def sha(raw):return hashlib.sha256(raw).hexdigest()
def patches(before,after):
 out=[];at=0
 while at<len(before):
  if before[at]==after[at]:at+=1;continue
  start=at
  while at<len(before) and before[at]!=after[at]:at+=1
  out.append(dict(offset=start,before=before[start:at].hex(),after=after[start:at].hex()))
 return out
def main():
 s=Snapshot(ROOT/'analysis/multiplayer-shading/registration-snapshot-restore-20261008');x=Xbe(ROOT/'assets/disc/default.xbe');assert sha(x.data)==PIN
 u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(SYNTH,0x100000);pages={};dirty=set();seen={}
 def page(address):
  at=address&~4095
  if at not in pages:
   raw=s.read(at,4096);u.mem_map(at,4096);u.mem_write(at,raw);pages[at]=raw
 def read(address,size):
  for at in range(address&~4095,((address+size-1)&~4095)+4096,4096):
   if not SYNTH<=at<SYNTH+0x100000:page(at)
  return bytes(u.mem_read(address,size))
 def put(address,raw):
  read(address,len(raw));u.mem_write(address,raw)
  for at in range(address&~4095,((address+len(raw)-1)&~4095)+4096,4096):
   if not SYNTH<=at<SYNTH+0x100000:dirty.add(at)
 def missing(g,access,address,size,value,data):
  for at in range(address&~4095,((address+size-1)&~4095)+4096,4096):page(at)
  return True
 def code(g,pc,size,data):
  if pc==STOP:g.emu_stop();return
  assert read(pc,size)==x.read(pc,size);seen[pc]=size
 def writing(g,access,address,size,value,data):
  if not SYNTH<=address<SYNTH+0x100000:
   for at in range(address&~4095,((address+size-1)&~4095)+4096,4096):dirty.add(at)
 u.hook_add(UC_HOOK_MEM_UNMAPPED,missing);u.hook_add(UC_HOOK_CODE,code);u.hook_add(UC_HOOK_MEM_WRITE,writing)
 ga,gl=(int(v,16)for v in re.search(r'GDT=\s*([0-9a-fA-F]+)\s+([0-9a-fA-F]+)',s.report['registers_before']).groups());read(ga,gl+1)
 u.reg_write(UC_X86_REG_GDTR,(0,ga,gl,0))
 for reg,value in[(UC_X86_REG_CS,8),(UC_X86_REG_SS,16),(UC_X86_REG_DS,16),(UC_X86_REG_ES,16),(UC_X86_REG_FS,32)]:u.reg_write(reg,value)
 manager=uint(s.read(0x4e6464,4))
 pool_addresses={'physical':uint(s.read(manager+0x64,4)),'image':uint(s.read(0x4e6454,4))}
 banks={}
 for name,address in pool_addresses.items():
  header=read(address,0x4c);stride=uint(header,0x24);assert uint(header,0x20)==512 and stride in[24,40]
  rows=uint(header,0x44);bits=uint(header,0x48)
  banks[name]=dict(address=address,stride=stride,rows_address=rows,bits_address=bits,
   header_hex=header.hex(),rows_hex=read(rows,512*stride).hex(),bits_hex=read(bits,64).hex())
 def reset():
  for at in dirty:u.mem_write(at,pages[at])
  dirty.clear()
  for reg in[UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP]:u.reg_write(reg,0)
  u.reg_write(UC_X86_REG_ESP,STACK);u.reg_write(UC_X86_REG_EFLAGS,2);u.reg_write(UC_X86_REG_FPCW,0x023f)
  u.reg_write(UC_X86_REG_FPTAG,0xffff);u.reg_write(UC_X86_REG_FPSW,0);u.reg_write(UC_X86_REG_MXCSR,0x1f80)
 cases=[]
 templates=[('empty',set(),0,[0]),('one',set([0]),1,[0,1]),
  ('sparse',set([0,2,7,64,255,511]),512,[0,1,128,511,512]),
  ('full',set(range(512)),512,[0,128,512]),
  ('last_free',set(range(511)),511,[0,128,511]),
  ('middle_free',set(range(512))-{256},512,[0,255,256,257,512])]
 for name,bank in banks.items():
  inputs=[dict(label='actual retained bank',header=bytes.fromhex(bank['header_hex']),
   rows=bytes.fromhex(bank['rows_hex']),bits=bytes.fromhex(bank['bits_hex']))]
  for label,occupied,high,firsts in templates:
   for first in firsts:
    for salt in[0x8000,0xfffd,0xfffe]:
     header=bytearray.fromhex(bank['header_hex']);rows=bytearray([0xa5])*(512*bank['stride']);bits=bytearray(64)
     header[0x34:0x40]=packed([first,high,len(occupied)]);header[0x40:0x42]=salt.to_bytes(2,'little')
     for index in range(512):
      rows[index*bank['stride']:index*bank['stride']+2]=(0x8123 if index in occupied else 0).to_bytes(2,'little')
      if index in occupied:bits[index>>3]|=1<<(index&7)
     inputs.append(dict(label=f'explicit {label}; next={first}; salt={salt:04X}',header=header,rows=rows,bits=bits))
  for initial in inputs:
   reset();before={key:bytes(initial[key])for key in['header','rows','bits']}
   for key,address in[('header',bank['address']),('rows',bank['rows_address']),('bits',bank['bits_address'])]:put(address,before[key])
   u.reg_write(UC_X86_REG_EDX,bank['address']);u.mem_write(STACK,packed([STOP]));u.emu_start(0x16b840,STOP,count=1000000)
   assert u.reg_read(UC_X86_REG_EIP)==STOP and u.reg_read(UC_X86_REG_ESP)==STACK+4
   after={'header':read(bank['address'],0x4c),'rows':read(bank['rows_address'],512*bank['stride']),'bits':read(bank['bits_address'],64)}
   expected={key:bytearray(value)for key,value in before.items()};header=expected['header'];high=uint(header,0x38);first=uint(header,0x34)
   free=next((i for i in range(first,high)if not(before['bits'][i>>3]&(1<<(i&7)))),high)
   if free>=512:returned=0xffffffff
   else:
    old_salt=int.from_bytes(header[0x40:0x42],'little');returned=(old_salt<<16)|free
    expected['bits'][free>>3]|=1<<(free&7);header[0x3c:0x40]=packed([(uint(header,0x3c)+1)&0xffffffff])
    header[0x34:0x38]=packed([free+1]);header[0x38:0x3c]=packed([max(high,free+1)])
    at=free*bank['stride'];expected['rows'][at:at+bank['stride']]=bytes(bank['stride']);expected['rows'][at:at+2]=old_salt.to_bytes(2,'little')
    following=(old_salt+1)&65535
    if following==65535:following=0x8000
    header[0x40:0x42]=following.to_bytes(2,'little')
   assert u.reg_read(UC_X86_REG_EAX)==returned and all(after[key]==bytes(expected[key])for key in expected)
   cases.append(dict(pool=name,label=initial['label'],source_return_handle=f'{returned:08X}',
    input_patches={key:patches(bytes.fromhex(bank[key+'_hex']),value)for key,value in before.items()},
    exact_output_patches={key:patches(before[key],after[key])for key in before},
    exact_output_sha256={key:sha(after[key])for key in after},kernel_or_gpu_result_supplied=False))
 reset()
 for at,raw in pages.items():assert bytes(u.mem_read(at,4096))==raw
 report=dict(authority=__doc__,source_xbe_sha256=PIN,snapshot_ram_sha256=sha(s.ram),
  snapshot_manifest_sha256=sha((s.directory/'export.json').read_bytes()),tool_sha256=sha(Path(__file__).read_bytes()),
  source_entry='0016B840',abi=dict(pool='EDX',returns='EAX salted handle or NONE; RET'),
  cases=len(cases),actual_retained_cases=2,explicit_conditional_cases=len(cases)-2,banks=banks,records=cases,
  unique_instructions=len(seen),restored_pages_checked=len(pages),fpcw='023F',mxcsr='1F80',services_replaced=[],
  source_instruction_patches=[],native_helper_or_residency_activated=False,original_pressure_or_history_claimed=False)
 path=ROOT/'analysis/multiplayer-shading/source-pool-allocation-original.json';path.write_text(json.dumps(report,indent=2)+'\n')
 print(json.dumps({key:report[key]for key in['cases','actual_retained_cases','explicit_conditional_cases','unique_instructions','restored_pages_checked']},indent=2))
if __name__=='__main__':main()
