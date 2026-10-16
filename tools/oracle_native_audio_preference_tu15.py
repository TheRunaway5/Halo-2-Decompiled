#!/usr/bin/env python3
"""Run original386D4A with explicit source speaker-preference inputs.

The actual native policy supplies stereo0 only after opening its native output.
Other source-valid preference bits here are reference-only declared inputs, not
advertised native routes or compressed encoders. Original SDK constructors run;
no EEPROM, SMC/AV-pack value, device result, capability or game BOOL is supplied.
"""
import argparse,hashlib,json,struct
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_EBX,UC_X86_REG_ESP,UC_X86_REG_EBP,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EIP,UC_X86_REG_EFLAGS
GP=[UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_EBX,UC_X86_REG_ESP,UC_X86_REG_EBP,UC_X86_REG_ESI,UC_X86_REG_EDI]
DATA,STACK,STOP=0x2001000,0x2008000,0x2100000

def run(image,flags,executed):
 cpu=Uc(UC_ARCH_X86,UC_MODE_32);cpu.mem_map(0x10000,0x600000)
 for sec in image.sections:
  raw=image.section_bytes(sec)
  if sec.va+len(raw)<=0x610000:cpu.mem_write(sec.va,raw)
 cpu.mem_map(0x2000000,0x10000);cpu.mem_map(STOP,0x1000);cpu.mem_write(DATA,b'\xa5'*0xb4)
 cpu.mem_write(STACK,struct.pack('<I',STOP));cpu.mem_write(0x390be8,struct.pack('<I',flags))
 for i,r in enumerate(GP):cpu.reg_write(r,0x11000000+i*0x111111)
 cpu.reg_write(UC_X86_REG_ECX,DATA);cpu.reg_write(UC_X86_REG_ESP,STACK);cpu.reg_write(UC_X86_REG_EFLAGS,0x202)
 instructions=[];stores=[]
 def hook(m,pc,size,_):
  assert pc not in [0x37da9a,0x33c29d,0x2e101c],'Explicit preference unexpectedly required console settings/hardware'
  raw=bytes(m.mem_read(pc,size));assert image.read(pc,size)==raw;executed[f'{pc:08x}']=raw.hex();instructions.append(pc)
 def write(m,access,address,size,value,_):stores.append([m.reg_read(UC_X86_REG_EIP),address,size,value&((1<<(size*8))-1)])
 cpu.hook_add(UC_HOOK_CODE,hook);cpu.hook_add(UC_HOOK_MEM_WRITE,write);cpu.emu_start(0x386d4a,STOP,count=10000)
 assert cpu.reg_read(UC_X86_REG_EIP)==STOP and cpu.reg_read(UC_X86_REG_ESP)==STACK+4
 assert struct.unpack('<I',cpu.mem_read(0x390be4,4))[0]==flags
 return dict(explicit_flags=flags,native_supported_route=flags==0,returned_gp=[cpu.reg_read(r)&0xffffffff for r in GP],returned_eflags=cpu.reg_read(UC_X86_REG_EFLAGS),data_hex=bytes(cpu.mem_read(DATA,0xb4)).hex(),configuration_hex=bytes(cpu.mem_read(0x390be4,8)).hex(),instructions=len(instructions),source_stores=stores)

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();assert not a.output.exists();x=Xbe(NEW_DEFAULT);assert hashlib.sha256(x.data).hexdigest()==NEW_PIN
 executed={};cases=[run(x,flags,executed)for flags in [0,1,2,0x10000,0x20002,0x30000]];a.output.mkdir(parents=True)
 report=dict(passed=True,scope=__doc__,original_xbe_sha256=NEW_PIN,tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),cases=cases,readonly_regions=[dict(address=p,bytes=n,hex=x.read(p,n).hex())for p,n in [(0x38f8cc,60),(0x38f898,48)]],executed_instruction_bytes=executed,supplied_console_settings_or_hardware_values=[],supplied_game_or_device_results=[],source_sdk_full_factory_completed=False)
 (a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(f'{len(cases)} original explicit preference cases, {len(executed)} unchanged sites; no EEPROM/SMC call')
if __name__=='__main__':main()
