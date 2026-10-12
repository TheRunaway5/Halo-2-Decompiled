#!/usr/bin/env python3
"""Original fresh context/controller owners and native selected-profile layout.

This does not certify native OS adapter selection or reproduce Windows RNG
identity histories. Clocks and XUser/input/reference services are explicit
source inputs. Source profile publication and all source field writes execute.
"""
from pathlib import Path
import hashlib,importlib.util,json,struct,sys
ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'analysis/frontend/vista-session-flow-audit/native-offline-owner'
sys.path.insert(0,str(OUT.parent/'local-session-inputs-original'))
from audit import PE,EXE,EXE_SHA,DLL,DLL_SHA
from oracle_native_vista_offline_services import execute as privilege,map_progress,local_owners
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
spec=importlib.util.spec_from_file_location('controller_owner',OUT.parent/'controller-user-owner/audit.py')
prior=importlib.util.module_from_spec(spec);spec.loader.exec_module(prior)
STOP=0x3000000;STACK=0x2000000;CONTEXT=0x2100000

def machine(p):
 u=Uc(UC_ARCH_X86,UC_MODE_32)
 size=(max(s['rva']+max(s['virtual_size'],s['raw_size']) for s in p.sections)+4095)&~4095
 u.mem_map(p.base,size)
 for s in p.sections:u.mem_write(p.base+s['rva'],p.data[s['raw']:s['raw']+s['raw_size']])
 for a,n in [(STACK,0x20000),(CONTEXT,0x10000),(STOP,0x1000)]:u.mem_map(a,n)
 return u

def ret(u,value=0,high=None):
 s=u.reg_read(UC_X86_REG_ESP);pc=struct.unpack('<I',u.mem_read(s,4))[0]
 u.reg_write(UC_X86_REG_EAX,value&0xffffffff)
 if high is not None:u.reg_write(UC_X86_REG_EDX,high&0xffffffff)
 u.reg_write(UC_X86_REG_ESP,s+4);u.reg_write(UC_X86_REG_EIP,pc)

def context(p,poison,clock):
 u=machine(p);before=bytearray((i*37+poison)&255 for i in range(0x218));before[0]=0
 u.mem_write(CONTEXT,bytes(before));u.mem_write(STACK+0x10000,struct.pack('<I',STOP));u.reg_write(UC_X86_REG_ESP,STACK+0x10000);u.reg_write(UC_X86_REG_ECX,CONTEXT);calls=[]
 def hook(m,pc,n,_):
  if pc==STOP:m.emu_stop()
  elif pc==0x104f4cf0:calls.append('qpc_ms');ret(m,clock,clock>>32)
  elif pc==0x1027dd00:
   s=m.reg_read(UC_X86_REG_ESP);dest,value,length=struct.unpack('<3I',m.mem_read(s+4,12));m.mem_write(dest,bytes([value&255])*length);ret(m,dest)
 u.hook_add(UC_HOOK_CODE,hook);u.emu_start(0x100c4ec0,STOP+1,count=10000)
 assert u.reg_read(UC_X86_REG_EIP)==STOP and calls==['qpc_ms']
 return dict(kind='context',clock=clock,before=before.hex(),after=bytes(u.mem_read(CONTEXT,0x218)).hex())

def main():
 OUT.mkdir(parents=True,exist_ok=True);e=PE(EXE,EXE_SHA);d=PE(DLL,DLL_SHA);rows=[]
 for poison in (0,17,165,255):
  for clock in (0,0xffffffff,0x123456789abcdef0):rows.append(context(d,poison,clock))
 for poison in (0,165):
  o=prior.Oracle(EXE.read_bytes());before=bytes([poison])*0x4a38;o.machine.mem_write(prior.CONTROLLERS,before)
  assert o.call(0x608608)['returned']
  rows.append(dict(kind='controllers',before=before.hex(),after=prior.raw(o.machine,prior.CONTROLLERS,0x4a38).hex()))
 # Fresh native selected configurations use cleared input/account owner bytes.
 # The PC active-user read likewise sees a freshly allocated cleared B8 bank.
 for name in ('Player','A\U0001f642 Z','ABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890'):
  for team in (-1,0,7,16):
   o=prior.Oracle(EXE.read_bytes(),xuid=bytes.fromhex('1234567800000000'))
   assert o.call(0x608608)['returned'];o.machine.mem_write(prior.USERS,bytes(4*0xb8));prior.set32(o.machine,0xd712c8+8,0)
   cfg=bytearray(0x90);text=name.encode('utf-16le')[:62];cfg[:len(text)]=text
   cfg[0x40:0x50]=bytes([10,3,7,10,1,2,3,4,5,0,0,0,0,0,0,0]);cfg[0x7c]=team&255
   profile=bytearray(0x1208);profile[8:8+64]=cfg[:64];profile[0x1148:0x1158]=cfg[0x40:0x50]
   o.machine.mem_write(prior.INPUT,bytes(profile));prior.set32(o.machine,prior.CONTROLLERS+0x1220,team)
   assert o.call(0x6087bf,0,prior.INPUT,0,0)['returned']
   payload=prior.raw(o.machine,prior.USERS+0x10,0x84)
   assert payload==cfg[:0x84],(name,team,payload.hex(),cfg[:0x84].hex())
   rows.append(dict(kind='profile',name=name,team=team,configuration=cfg.hex(),payload=payload.hex()))
 for counter in (0,1,15,16,0xffffffff):
  record=privilege(e,d,counter);rows.append(dict(kind='privilege',counter=counter,context=record['context']))
 rows += [map_progress(e,n)for n in (0,1)]
 for name in ('Player','A\U0001f642 Z','ABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890'):
  record=local_owners(e,name)
  rows.append({key:record[key]for key in ('kind','name','selected_name','names','optional_metadata','network_busy','bandwidth')})
 fixture=ROOT/'tests/fixtures/native_vista_session_original.jsonl';fixture.write_text(''.join(json.dumps(r,separators=(',',':'))+'\n' for r in rows))
 report=dict(exe_sha256=EXE_SHA,dll_sha256=DLL_SHA,fixture_sha256=hashlib.sha256(fixture.read_bytes()).hexdigest(),cases=len(rows),spans={'context':d.span(0x100c4ec0,0x100c5007),'controllers':e.span(0x608608,0x608675),'profile_payload':e.span(0x60696f,0x606b13)},boundary=__doc__)
 (OUT/'evidence.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(dict(cases=len(rows),fixture_sha256=report['fixture_sha256'])))
if __name__=='__main__':main()
