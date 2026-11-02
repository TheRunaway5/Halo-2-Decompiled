#!/usr/bin/env python3
"""Unchanged TU1.5 part admission through original17D210.

Rejected single-part events return naturally. Accepted cases stop before the
first17DCE0 location-iterator call; no iterator, environment, voice, allocation,
admission or playback result is supplied. Every original opcode is unchanged.
"""
import argparse,hashlib,itertools,json,os,struct
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
from unicorn import x86_const as R

BANK,EXTENT,STACK,STOP=0x1000000,0x10000,0x100e000,0x3000000
TABLE,DEFINITION,EVENTS,BODY,PARTS=[BANK+n*0x1000 for n in range(1,6)]
ENTRY,ACCEPT=0x17d210,0x17d2d0


def initial(row):
 raw=bytearray([row['fill']])*EXTENT
 def put(at,value,size=4):raw[at-BANK:at-BANK+size]=(value&((1<<(size*8))-1)).to_bytes(size,'little')
 put(TABLE+3*16+8,DEFINITION);put(DEFINITION+12,row['count']);put(DEFINITION+24,EVENTS)
 put(BODY+2,row['flags'],2);put(BODY+4,0x12340003);put(BODY+0x5c,row['event'],2)
 for n in range(3):put(EVENTS+n*0x38+0x18,1);put(EVENTS+n*0x38+0x1c,PARTS+n*0x100)
 part=PARTS+row['event']*0x100
 put(part,row['environment'],2);put(part+2,row['selector'],2);put(part+4,row['location'],2);put(part+0x10,row['reference'])
 put(STACK,STOP);put(STACK+4,BODY)
 return raw


def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
 if a.output.exists():p.error('Preserve original admission results')
 image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN;code=_Code(image,'.text');body=_trace(code,ENTRY,set())
 cpu=Uc(UC_ARCH_X86,UC_MODE_32);cpu.mem_map(0x10000,0x800000)
 for sec in image.sections:cpu.mem_write(sec.va,image.section_bytes(sec))
 cpu.mem_map(BANK,EXTENT);cpu.mem_map(STOP,4096);cpu.mem_write(STOP,b'\xf4')
 write=lambda at,value:cpu.mem_write(at,struct.pack('<I',value));regs=[getattr(R,'UC_X86_REG_'+n)for n in ('EAX','ECX','EDX','EBX','ESP','EBP','ESI','EDI')]
 state={};executed={};rng=0x17d210
 def draw():
  nonlocal rng;rng=(rng*1664525+1013904223)&0xffffffff;return rng
 def instruction(_,pc,size,__):
  if pc in (ACCEPT,STOP):state['end']=pc;cpu.emu_stop();return
  assert pc in body.code;raw=bytes(cpu.mem_read(pc,size));assert raw==image.read(pc,size);executed[f'{pc:08x}']=raw.hex();state['trace'].update(struct.pack('<I',pc));state['steps']+=1;state['pc']=pc
 def store(_,kind,at,size,value,__):
  assert STACK-0x100<=at and at+size<=STACK
  before=int.from_bytes(cpu.mem_read(at,size),'little');state['stores'].update(struct.pack('<5I',state['pc'],at,size,before,value&((1<<(8*size))-1)));state['count']+=1
 cpu.hook_add(UC_HOOK_CODE,instruction);cpu.hook_add(UC_HOOK_MEM_WRITE,store)
 controls=list(itertools.product([-32768,-1,0,1,15,16,32767],[0,1,16],[0,1,2,3,65535],[0,32,65535,65503],[0xffffffff,0x56780001]))
 controls +=[(0 if n%2 else 1,[0x7fffffff,0x80000000,0xffffffff,16][n%4],[0,1,2,3][n%4],draw()&65535,draw())for n in range(64)]
 controls +=[(draw()%18-1,draw(),[draw()&65535,1,2][n%3],draw()&65535,draw())for n in range(120)]
 rows=[]
 for n,(location,count,selector,flags,reference)in enumerate(controls):
  row=dict(name=f'part_{n}',location=location,count=count,selector=selector,flags=flags,reference=reference,environment=draw()&65535,event=n%3,fill=[0,0x5b,0xa5][n%3])
  cpu.mem_write(BANK,bytes(initial(row)));write(0x4f0414,TABLE);gp=[draw()for _ in regs];gp[4]=STACK
  for reg,value in zip(regs,gp):cpu.reg_write(reg,value)
  eflags=0x202|(draw()&0xcd5);cpu.reg_write(R.UC_X86_REG_EFLAGS,eflags)
  state.clear();state.update(trace=hashlib.sha256(),stores=hashlib.sha256(),steps=0,count=0,pc=0)
  try:cpu.emu_start(ENTRY,0,count=200)
  except Exception:
   print(json.dumps(dict(failed_input=row,pc=hex(cpu.reg_read(R.UC_X86_REG_EIP)))));raise
  assert state.get('end')in(ACCEPT,STOP)
  row.update(gp=gp,eflags=eflags,selected_for_location_iteration=state['end']==ACCEPT,end_pc=state['end'],expected_gp=[cpu.reg_read(r)&0xffffffff for r in regs],
   expected_eflags=cpu.reg_read(R.UC_X86_REG_EFLAGS),instruction_count=state['steps'],instruction_sha256=state['trace'].hexdigest(),ordered_store_count=state['count'],ordered_store_sha256=state['stores'].hexdigest(),whole_declared_bank_sha256=hashlib.sha256(cpu.mem_read(BANK,EXTENT)).hexdigest())
  rows.append(row)
 report=dict(scope=__doc__,original_xbe_sha256=NEW_PIN,entry=ENTRY,accepted_boundary=ACCEPT,rejected_return_boundary=STOP,bank=BANK,extent=EXTENT,source_tag_table_cell=0x4f0414,
  cases=rows,executed_original_instructions=executed,original_instruction_patches=[],substituted_callees=[],actual_sound_or_environment_result_supplied=False,
  tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),retained_large_artifacts=[])
 raw=json.dumps(report,separators=(',',':'))+'\n';assert len(raw)<1024*1024,len(raw);a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(raw)
 print(json.dumps(dict(cases=len(rows),selected=sum(r['selected_for_location_iteration']for r in rows),natural_rejections=sum(not r['selected_for_location_iteration']for r in rows),instruction_sites=len(executed),bytes=len(raw))))


if __name__=='__main__':main()
