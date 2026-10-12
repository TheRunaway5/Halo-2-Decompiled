#!/usr/bin/env python3
"""Original Vista protected join-context and supplied-DLL privilege service.

The executable's own decryptors run. Only actual platform Get/SetLastError and
VirtualAlloc/VirtualFree contracts are resolved; no source code is patched.
The xlive import resolves to the supplied DLL's actual XUserCheckPrivilege.
"""
from pathlib import Path
import hashlib,json,struct,sys
from capstone import Cs,CS_ARCH_X86,CS_MODE_32
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import *
from oracle_vista_pcr_screen import machine,STOP,STACK,u32,w32
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'analysis/frontend/vista-session-flow-audit/local-session-inputs-original'))
from audit import PE,EXE,EXE_SHA,DLL,DLL_SHA
OUT=ROOT/'analysis/frontend/vista-session-flow-audit/native-offline-services'
def execute(e,d,counter):
 u=machine(e.data);u.mem_map(0,0x1000)
 size=(max(s['rva']+max(s['virtual_size'],s['raw_size']) for s in d.sections)+4095)&~4095;u.mem_map(d.base,size)
 for s in d.sections:u.mem_write(d.base+s['rva'],d.data[s['raw']:s['raw']+s['raw_size']])
 api={};next_alloc=0x1c00000;body=[];calls=[];visits=0
 for i,imp in enumerate(e.imports()):
  a=STOP+0x100+i;w32(u,int(imp['iat_va'],16),a);api[a]=imp
 w32(u,0x79b13c,0x105b3240);w32(u,0x107f0ee8,counter)
 def ret(v=0,pop=0):
  sp=u.reg_read(UC_X86_REG_ESP);u.reg_write(UC_X86_REG_EAX,v&0xffffffff);u.reg_write(UC_X86_REG_EIP,u32(u,sp));u.reg_write(UC_X86_REG_ESP,sp+4+pop)
 def hook(_u,pc,n,_):
  nonlocal next_alloc,visits
  visits+=1
  if pc==STOP:u.emu_stop();return
  if pc in (0x6875d3,0x1020b598):ret();return
  if 0x1c00000<=pc<next_alloc:
   raw=bytes(u.mem_read(pc,n));i=next(Cs(CS_ARCH_X86,CS_MODE_32).disasm(raw,pc));body.append(dict(address=hex(pc),bytes=raw.hex(),mnemonic=i.mnemonic,operands=i.op_str))
  if pc==0x105b3240:
   sp=u.reg_read(UC_X86_REG_ESP);calls.append(['XUserCheckPrivilege',u32(u,sp+4),u32(u,sp+8)])
  if pc not in api:return
  imp=api[pc];name=imp['name'];sp=u.reg_read(UC_X86_REG_ESP);args=[u32(u,sp+4+4*i) for i in range(4)]
  if name=='GetLastError':ret();return
  if name=='SetLastError':ret(0,4);return
  if name=='VirtualAlloc':
   size=(args[1]+4095)&~4095;out=args[0] or next_alloc;next_alloc=out+size;u.mem_map(out,size);ret(out,16);return
  if name=='VirtualFree':ret(1,12);return
  raise RuntimeError(f'Unexpected external import {imp}')
 u.hook_add(UC_HOOK_CODE,hook);sp=STACK+0x18000;w32(u,sp,STOP);u.reg_write(UC_X86_REG_ESP,sp);u.reg_write(UC_X86_REG_ECX,0xd712c8)
 u.emu_start(0x61a995,STOP+1,count=3000000);assert u.reg_read(UC_X86_REG_EIP)==STOP
 return dict(kind='privilege',counter=counter,context=u.reg_read(UC_X86_REG_EAX)&255,calls=calls,body=body,instruction_visits=visits)
def map_progress(e,mode):
 u=machine(e.data);path=0x1a00000;result=path+0x400;thunk=STOP+0x800;calls=[];visits=0
 u.mem_write(path,b'scenarios\\multi\\lockout\\lockout\0')
 def hook(_u,pc,n,_):
  nonlocal visits
  visits+=1
  if pc==STOP:u.emu_stop();return
  if pc==0x4655a1:
   sp=u.reg_read(UC_X86_REG_ESP);calls.append(u32(u,sp+4));u.reg_write(UC_X86_REG_EAX,3)
   u.reg_write(UC_X86_REG_EIP,u32(u,sp));u.reg_write(UC_X86_REG_ESP,sp+4)
 u.hook_add(UC_HOOK_CODE,hook)
 u.mem_write(thunk,b'\xd9\x1d'+struct.pack('<I',result)+b'\xe9'+struct.pack('<i',STOP-thunk-11))
 sp=STACK+0x18000;u.mem_write(sp,struct.pack('<3I',thunk,mode,path));u.reg_write(UC_X86_REG_ESP,sp)
 u.emu_start(0x4317d2,STOP+1,count=300000);assert u.reg_read(UC_X86_REG_EIP)==STOP
 return dict(kind='map_progress',mode=mode,raw_file_state=3,progress_bits=u32(u,result),instruction_visits=visits,file_queries=len(calls))
def local_owners(e,name):
 u=machine(e.data);path=0x1a00000;visits=0
 selected=name.encode('utf-16le')[:62].ljust(64,b'\0');u.mem_write(path,selected)
 u.mem_write(0x91a590,bytes([0xa5])*0x638);u.mem_write(0x926c80,bytes([0xa5])*0x90)
 def hook(_u,pc,n,_):
  nonlocal visits
  visits+=1
  if pc==STOP:u.emu_stop();return
  if pc==0x6875d3:
   sp=u.reg_read(UC_X86_REG_ESP);u.reg_write(UC_X86_REG_EIP,u32(u,sp));u.reg_write(UC_X86_REG_ESP,sp+4)
 u.hook_add(UC_HOOK_CODE,hook)
 for entry,args in ((0x5b07b2,[0x91abc8]),(0x5e0ce1,[0x91c474,0x91c468]),(0x615024,[path,path])):
  # Actual network-initialized owner is an input to the selected-name event.
  if entry==0x615024:u.mem_write(0x91c45c,b'\1')
  sp=STACK+0x18000;u.mem_write(sp,struct.pack('<'+'I'*(1+len(args)),STOP,*args));u.reg_write(UC_X86_REG_ESP,sp)
  u.emu_start(entry,STOP+1,count=300000);assert u.reg_read(UC_X86_REG_EIP)==STOP
 return dict(kind='local_owners',name=name,selected_name=selected.hex(),names=bytes(u.mem_read(0x91a592,0x60)).hex(),
  optional_metadata=bytes(u.mem_read(0x91a5f2,1))[0],network_busy=bytes(u.mem_read(0x926cfc,1))[0],
  bandwidth=u32(u,0x926d08),peer_owner=bytes(u.mem_read(0x91a590,0x638)).hex(),
  bandwidth_owner=bytes(u.mem_read(0x926c80,0x90)).hex(),instruction_visits=visits)
def allowed_team_mask(e,loaded):
 u=machine(e.data);calls=[];visits=0
 assert u32(u,0x822428)==0xffffffff
 def ret(value=None):
  sp=u.reg_read(UC_X86_REG_ESP)
  if value is not None:u.reg_write(UC_X86_REG_EAX,value)
  u.reg_write(UC_X86_REG_EIP,u32(u,sp));u.reg_write(UC_X86_REG_ESP,sp+4)
 def hook(_u,pc,n,_):
  nonlocal visits
  visits+=1
  if pc==STOP:u.emu_stop();return
  if pc==0x6875d3:ret();return
  if pc in (0x4497ea,0x449813,0x449823):
   calls.append(hex(pc));ret(int(loaded) if pc==0x4497ea else 0)
 u.hook_add(UC_HOOK_CODE,hook);sp=STACK+0x18000;u.mem_write(sp,struct.pack('<2I',STOP,0x1a00000));u.reg_write(UC_X86_REG_ESP,sp)
 u.emu_start(0x5b087b,STOP+1,count=300000);assert u.reg_read(UC_X86_REG_EIP)==STOP
 return dict(kind='allowed_team_mask',world='mainmenu' if loaded else 'unloaded',mask=u.reg_read(UC_X86_REG_EAX)&0xffff,calls=calls,instruction_visits=visits)
def main():
 e=PE(EXE,EXE_SHA);d=PE(DLL,DLL_SHA);rows=[execute(e,d,n)for n in (0,1,15,16,0xffffffff)]
 assert all(r['context']==1 and r['calls']==[['XUserCheckPrivilege',0,254]]for r in rows)
 progress=[map_progress(e,n)for n in (0,1)];assert [r['progress_bits']for r in progress]==[0x3f800000,0x3f800000]
 owners=[local_owners(e,name)for name in ('Player','A\U0001f642 Z','ABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890')]
 assert all(r['network_busy']==0 and r['bandwidth']==0xffffffff and r['optional_metadata']==0 for r in owners)
 masks=[allowed_team_mask(e,n)for n in (False,True)];assert [r['mask']for r in masks]==[0,255]
 OUT.mkdir(parents=True,exist_ok=True)
 report=dict(exe_sha256=EXE_SHA,dll_sha256=DLL_SHA,records=rows,map_progress=progress,local_owners=owners,allowed_team_masks=masks,spans={'protected_entry':e.span(0x61a995,0x61a9c7),'privilege_entry':d.span(0x105b3240,0x105b329f),'privilege_FE':d.span(0x105b3929,0x105b3944),'privilege_result':d.span(0x105b4e87,0x105b4ea2),'context_import':e.span(0x5b07a0,0x5b07b2),'thresholds':e.span(0x5abd62,0x5abd7a),'threshold_value':e.span(0x5abb1a,0x5abb24),'map_progress':e.span(0x430dfa,0x431064),'file_fraction':e.span(0x4657a2,0x46580a),'peer_initialize':e.span(0x5b07b2,0x5b07d7),'bandwidth_initialize':e.span(0x5e0ce1,0x5e0d55),'bandwidth_mode_change':e.span(0x5e0fac,0x5e0fc9),'selected_name':e.span(0x615024,0x6150d3),'peer_names':e.span(0x5b100b,0x5b1031),'allowed_teams':e.span(0x5b087b,0x5b0a09)},boundary=__doc__)
 (OUT/'original.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(dict(cases=len(rows),context=[r['context']for r in rows],instruction_visits=sum(r['instruction_visits']for r in rows))))
if __name__=='__main__':main()
