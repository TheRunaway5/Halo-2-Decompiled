#!/usr/bin/env python3
"""Join original GP auxiliary output and VP input through descriptor391410.

Descriptor/context values are declared resource inputs. Original CPU table
append and page-publication stores execute unpatched into write-only MMIO
sinks. No allocation/kernel/device-read/result/clock is supplied or certified.
"""
import argparse,hashlib,json,os,struct
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_READ,UC_HOOK_MEM_WRITE

from unicorn import x86_const as R


def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
 if a.output.exists():p.error('Preserve original auxiliary page evidence')
 image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN;code=_Code(image,'DSOUND');parent=_trace(code,0x38e0ce,set())
 assert all(v in parent.code for v in(0x38e159,0x38e163,0x38e168,0x38e16d))
 gp=image.read(0x390c18,0x5cc);words=struct.unpack('<371I',gp)
 assert [words[i]for i in(0x14,0x16,0x18,0x1c)]==[0x1560,0xb000,640,0x49e2]
 cpu=Uc(UC_ARCH_X86,UC_MODE_32);cpu.mem_map(0x10000,0x800000)
 for s in image.sections:cpu.mem_write(s.va,image.section_bytes(s))
 bank=0x1000000;cpu.mem_map(bank,0x10000);cpu.mem_map(0xfe820000,0x1000);cpu.mem_map(0xfe802000,0x1000)
 ctx,table,out,stack,stop=bank+0x1000,bank+0x3000,bank+0x7000,bank+0xe000,bank+0xff00
 cpu.mem_write(stop,b'\xf4');read=lambda at:int.from_bytes(cpu.mem_read(at,4),'little');write=lambda at,v:cpu.mem_write(at,struct.pack('<I',v&0xffffffff))
 state={};instructions={}
 def hook(_,pc,size,__):
  if pc==state['stop']:state['complete']=True;cpu.emu_stop();return
  raw=image.read(pc,size);assert bytes(cpu.mem_read(pc,size))==raw;instructions[f'{pc:08x}']=raw.hex();state['pc']=pc
 def access(_,kind,address,size,value,__):
  if address>=0xfe800000:
   assert kind!=16,'No APU read is supplied';state['pio'].append([state['pc'],address,size,value])
 cpu.hook_add(UC_HOOK_CODE,hook);cpu.hook_add(UC_HOOK_MEM_READ|UC_HOOK_MEM_WRITE,access);rows=[]
 for source,physical in((0x1200000,0x6000000),(0x2300000,0x20000000),(0x3400000,0x7ffff000)):
  cpu.mem_write(ctx,bytes(0x24));cpu.mem_write(table,bytes([0xa5])*0x1060);write(ctx,524);write(ctx+4,513);write(0x391430,table)
  cpu.mem_write(0x391410,struct.pack('<4I',source,physical,4096,0));cpu.mem_write(stack,struct.pack('<4I',stop,0x391410,1,out));write(out,0xa5a5a5a5)
  cpu.reg_write(R.UC_X86_REG_ECX,ctx);cpu.reg_write(R.UC_X86_REG_ESP,stack);cpu.reg_write(R.UC_X86_REG_EFLAGS,2);state.clear();state.update(stop=stop,pio=[])
  cpu.emu_start(0x38de07,0,count=1000);assert state.get('complete')and read(out)==0xb000 and read(ctx+4)==512
  assert [read(table+11*8),read(table+11*8+4)]==[physical,0];assert state['pio']==[[0x38de4d,0xfe8020d4,4,12]];append=list(state['pio'])
  state.clear();state.update(stop=0x38e16d,pio=[]);cpu.emu_start(0x38e159,0,count=10);assert state.get('complete')
  assert state['pio']==[[0x38e159,0xfe820804,4,0x7ff],[0x38e168,0xfe820808,4,physical]]
  rows.append(dict(declared_source_base=source,declared_physical_page=physical,original_GP_append_offset=read(out),original_GP_SGE=[read(table+11*8),read(table+11*8+4)],original_GP_count_write=append,original_VP_page_writes=state['pio']))
 report=dict(scope=__doc__,original_xbe_sha256=NEW_PIN,original_GP_program_sha256=hashlib.sha256(gp).hexdigest(),original_GP_auxiliary=dict(source_mix_alias=0x1560,first_bin=11,words=640,destination=0xb000,control=0x49e2,format=2),
  source_descriptor=0x391410,GP_page_index=11,VP_page_index=0x7ff,cases=rows,executed_original_instructions=instructions,original_instruction_patches=[],device_reads_or_success_results_supplied=[],physical_DMA_alignment_or_clock_verified=False,
  tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),retained_large_artifacts=[])
 a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(report,indent=2)+'\n');print('PASS original GP B000/page11 and VP page7FF share descriptor391410 in3 declared resource profiles; no hardware acceptance.')


if __name__=='__main__':main()
