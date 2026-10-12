#!/usr/bin/env python3
"""Execute unchanged 1A0E70 -> SetFilePointer to its actual kernel boundary.

Read callbacks supply a nonNULL pointer to a zero high DWORD. Independent
wrapper controls distinguish that from its NULL/sign-extension branch.
No seek status, file position, kernel return or I/O completion is supplied.
"""
from pathlib import Path
import hashlib,json,re,sys
from unicorn import Uc,UcError,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_UNMAPPED,UC_HOOK_MEM_WRITE
from unicorn.x86_const import *
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'tools'))
from differential_source_object_admission_snapshot import Snapshot,PIN,uint,words,packed
sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'));from xbe import Xbe
SYNTH,STACK,STOP,TASK,HIGH=0x20000000,0x200fe000,0x200ff000,0x20004000,0x20003000
def sha(raw):return hashlib.sha256(raw).hexdigest()
def main():
 s=Snapshot(ROOT/'analysis/multiplayer-shading/registration-snapshot-restore-20261008');x=Xbe(ROOT/'assets/disc/default.xbe');assert sha(x.data)==PIN
 u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(SYNTH,0x100000);pages={};dirty=set();seen={};events=[];boundaries=[]
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
  try:
   for at in range(address&~4095,((address+size-1)&~4095)+4096,4096):page(at)
   return True
  except ValueError as error:
   boundaries.append(dict(kind='uncaptured memory',pc=f'{g.reg_read(UC_X86_REG_EIP):08X}',address=f'{address:08X}',reason=str(error)));g.emu_stop();return False
 def code(g,pc,size,data):
  if pc==STOP:g.emu_stop();return
  try:original=x.read(pc,size)
  except (ValueError,KeyError):
   st=words(read(g.reg_read(UC_X86_REG_ESP)+4,20))
   record=dict(kind='actual kernel entry; not executed',entry=f'{pc:08X}',stack=st)
   if pc==nt_set:record['requested_position']=words(read(st[2],8))
   boundaries.append(record);g.emu_stop();return
  assert read(pc,size)==original;seen[pc]=size
  if pc==0x2d2488:
   st=words(read(g.reg_read(UC_X86_REG_ESP)+4,16))
   events.append(dict(entry='002D2488',file=st[0],low=st[1],high_pointer=f'{st[2]:08X}',
    high_value=uint(read(st[2],4)) if st[2] else None,method=st[3]))
 def writing(g,access,address,size,value,data):
  if not SYNTH<=address<SYNTH+0x100000:
   for at in range(address&~4095,((address+size-1)&~4095)+4096,4096):dirty.add(at)
 u.hook_add(UC_HOOK_MEM_UNMAPPED,missing);u.hook_add(UC_HOOK_CODE,code);u.hook_add(UC_HOOK_MEM_WRITE,writing)
 ga,gl=(int(v,16)for v in re.search(r'GDT=\s*([0-9a-fA-F]+)\s+([0-9a-fA-F]+)',s.report['registers_before']).groups());read(ga,gl+1)
 u.reg_write(UC_X86_REG_GDTR,(0,ga,gl,0))
 for reg,value in[(UC_X86_REG_CS,8),(UC_X86_REG_SS,16),(UC_X86_REG_DS,16),(UC_X86_REG_ES,16),(UC_X86_REG_FS,32)]:u.reg_write(reg,value)
 nt_set=uint(s.read(0x411584,4));assert nt_set>=0x80000000
 assert uint(x.read(0x411584,4))==0x800000e2
 exports_path=ROOT/'tools/reference/halo2-native-reference/scripts/vendor/xbox_kernel_exports.py'
 assert re.search(r'226:\s*"NtSetInformationFile"',exports_path.read_text())
 file=uint(s.read(0x557c90+uint(s.read(0x55aca8,4))*0x804,4));assert file==76
 def reset():
  for at in dirty:u.mem_write(at,pages[at])
  dirty.clear();events.clear();boundaries.clear()
  for reg in[UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP]:u.reg_write(reg,0)
  u.reg_write(UC_X86_REG_ESP,STACK);u.reg_write(UC_X86_REG_EFLAGS,2);u.reg_write(UC_X86_REG_FPCW,0x023f)
  u.reg_write(UC_X86_REG_FPTAG,0xffff);u.reg_write(UC_X86_REG_FPSW,0);u.reg_write(UC_X86_REG_MXCSR,0x1f80)
  u.mem_write(STACK-256,bytes(512))
 records=[];offsets=[0,1,0x1ff,0x200,0x7ffffffe,0x7fffffff,0x80000000,0x80000001,0xfffffffe,0xffffffff]
 for kind in['read_callback','nonnull_zero_high','null_high']:
  for offset in offsets:
   reset()
   if kind=='read_callback':
    task=[file,0x20005000,64,offset,0,0,0,0,0,0]
    u.mem_write(TASK,packed(task));u.mem_write(STACK,packed([STOP,TASK]));entry=0x1a0e70
   else:
    u.mem_write(HIGH,packed([0]));pointer=HIGH if kind=='nonnull_zero_high' else 0
    u.mem_write(STACK,packed([STOP,file,offset,pointer,0]));entry=0x2d2488
   try:u.emu_start(entry,STOP,count=1000000)
   except UcError:
    if not boundaries:raise
   assert len(events)==1 and events[0]['low']==offset and events[0]['method']==0
   if kind=='null_high' and offset>=0x80000000:
    assert not boundaries and u.reg_read(UC_X86_REG_EIP)==STOP and u.reg_read(UC_X86_REG_EAX)==0xffffffff
    outcome=dict(kind='complete title rejection before kernel',eax='FFFFFFFF')
   else:
    assert len(boundaries)==1 and boundaries[0]['entry']==f'{nt_set:08X}'
    assert boundaries[0]['stack'][0]==file and boundaries[0]['stack'][3:]==[8,14]
    assert boundaries[0]['requested_position']==[offset,0]
    if kind!='null_high':assert events[0]['high_pointer']!='00000000' and events[0]['high_value']==0
    outcome=boundaries[0]
   if kind=='read_callback':assert words(read(TASK,40))==task
   records.append(dict(kind=kind,offset=f'{offset:08X}',wrapper_input=events[0],outcome=outcome,
    kernel_outcome_supplied=False,file_current_position_result_claimed=False))
 reset()
 for at,raw in pages.items():assert bytes(u.mem_read(at,4096))==raw
 report=dict(authority=__doc__,source_xbe_sha256=PIN,snapshot_ram_sha256=sha(s.ram),
  snapshot_manifest_sha256=sha((s.directory/'export.json').read_bytes()),tool_sha256=sha(Path(__file__).read_bytes()),
  nt_set_iat='00411584',actual_nt_set_entry=f'{nt_set:08X}',actual_file_token=file,
  source_kernel_import='800000E2',source_kernel_export_ordinal=226,
  export_name_evidence=dict(path=str(exports_path.relative_to(ROOT)),sha256=sha(exports_path.read_bytes())),
  cases=len(records),whole_read_callback_cases=10,explicit_wrapper_controls=20,
  source_null_pointer_highbit_rejections=4,records=records,unique_instructions=len(seen),
  restored_pages_checked=len(pages),fpcw='023F',mxcsr='1F80',services_replaced=[],instruction_patches=[],
  negative_original_seek_status_supplied=False,original_file_position_or_completion_claimed=False)
 path=ROOT/'analysis/multiplayer-shading/source-async-seek-wrapper-original.json';path.write_text(json.dumps(report,indent=2)+'\n')
 print(json.dumps({key:report[key]for key in['cases','whole_read_callback_cases','explicit_wrapper_controls','source_null_pointer_highbit_rejections','unique_instructions','restored_pages_checked','actual_nt_set_entry']},indent=2))
if __name__=='__main__':main()
