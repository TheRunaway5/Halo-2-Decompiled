#!/usr/bin/env python3
"""Unchanged original full PIO CPU owners and observable interruption prefixes.

FIFO words, storage, pointers and timed mutations are declared component inputs.
MMIO writes are observation sinks, never device success or completion results.
No original instruction/callee result is patched or supplied by native code.
"""
import argparse,hashlib,json,os,struct
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_READ,UC_HOOK_MEM_WRITE
from unicorn import x86_const as R


def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
 if a.output.exists():p.error('Preserve original PIO results')
 image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN;code=_Code(image,'DSOUND');body=_trace(code,0x38e0ce,set());assert 0x38e159 in body.code
 cpu=Uc(UC_ARCH_X86,UC_MODE_32);cpu.mem_map(0x10000,0x800000)
 for s in image.sections:cpu.mem_write(s.va,image.section_bytes(s))
 bank,extent=0x1000000,0x50000;cpu.mem_map(bank,extent);cpu.mem_map(0xfe800000,0x3000);cpu.mem_map(0xfe820000,0x3000)
 table0,table1,route0,route1,stack,stop=bank+0x1000,bank+0xa000,bank+0x13000,bank+0x14000,bank+0x40000,bank+0x4ff00
 cpu.mem_write(stop,b'\xf4');read=lambda at,n=4:int.from_bytes(cpu.mem_read(at,n),'little');write=lambda at,v:cpu.mem_write(at,struct.pack('<I',v&0xffffffff))
 state={};executed={};rng=0x913f27ac
 def draw():
  nonlocal rng;rng=(rng*1664525+1013904223)&0xffffffff;return rng
 def apply(kind,pc):
  key=(kind,pc);state['counts'][key]=state['counts'].get(key,0)+1
  for change in state['changes']:
   if change['kind']==kind and change['pc']==pc and change['occurrence']==state['counts'][key]:
    for at,value in change['writes']:
     write(at,value);state['events'].append(dict(kind='input',address=at,value=value))
 def instruction(_,pc,size,__):
  if pc==stop:state['returned']=True;cpu.emu_stop();return
  occurrence=state['visits'].get(pc,0)+1;state['visits'][pc]=occurrence
  if state['interrupt']and pc==state['interrupt'][0]and occurrence==state['interrupt'][1]:state['interrupted']=True;cpu.emu_stop();return
  assert pc in body.code;raw=image.read(pc,size);assert bytes(cpu.mem_read(pc,size))==raw;executed[f'{pc:08x}']=raw.hex();state['pc']=pc;state['instructions'].append(pc);apply('instruction',pc)
 def access(_,kind,at,size,value,__):
  if kind==16:
   if at>=0xfe800000:
    assert at==0xfe820010 and size==4 and state['pc']==0x38e0ce
    n=state['fifo_reads'];values=state['fifo'];assert n<len(values);write(at,values[n]);state['fifo_reads']+=1
   return
  pc=state['pc'];action=at>=0xfe800000
  event=dict(kind='action'if action else'store',pc=pc,address=at,bytes=size,value=value&((1<<(8*size))-1))
  if not action:event['before']=read(at,size)
  state['events'].append(event);apply('store',pc)
 cpu.hook_add(UC_HOOK_CODE,instruction);cpu.hook_add(UC_HOOK_MEM_READ|UC_HOOK_MEM_WRITE,access)
 regs=[getattr(R,'UC_X86_REG_'+name)for name in('EAX','ECX','EDX','EBX','ESP','EBP','ESI','EDI')];rows=[]
 stops=[(0x38e0ce,2),(0x38e0e0,3),(0x38e0ee,4),(0x38e0ff,1),(0x38e136,1),(0x38e140,1),(0x38e159,1),
        (0x38e163,1),(0x38e16d,1),(0x38e180,1),(0x38e184,1),(0x38e19f,1)]
 for index in range(36):
  entry=0x38e0ce if index<12 or 24<=index<31 else 0x38e159;interrupt=None if index<24 else stops[index-24]
  fill=(0,0x5b,0xa5)[index%3];cpu.mem_write(bank,bytes([fill])*extent);cpu.mem_write(stop,b'\xf4')
  fields=[[0x391390,table0],[0x391394,draw()],[0x3913d4,draw()],[0x3913e4,draw()],[0x3913b4,draw()],
          [0x3913c4,draw()],[0x3913a4,draw()],[0x391414,draw()],[0x38f988,route0]]
  # Retain the external decoder sentinel in declared caller data too, so
  # whole-bank native/physical comparison includes every initialized byte.
  # It is observed before execution, outside the unchanged original image.
  fields.append([stop,(fill*0x01010100)|0xf4])
  for route in(route0,route1):
   for offset in(0,8,16,24):fields.append([route+offset,draw()])
  gp=[draw()for _ in range(8)];gp[4]=stack;fields.append([stack,stop if entry==0x38e0ce else draw()]);fields.append([stack+4,stop])
  for at,value in fields:write(at,value)
  fifo=[32]if index%3==0 else[0,4,31,32]if index%3==1 else[0xffffffff]
  if interrupt and interrupt[0]==0x38e0ce:fifo=[0,32]
  changes=[]
  if index%2==0:changes.append(dict(kind='store',pc=0x38e0ee,occurrence=1,writes=[[0x391390,table1]]))
  if index%3==1:changes.append(dict(kind='store',pc=0x38e104,occurrence=1,writes=[[0x3913d4,draw()]]))
  if index%4==2:changes.append(dict(kind='instruction',pc=0x38e172,occurrence=1,writes=[[0x38f988,route1]]))
  if index%4==3:changes.append(dict(kind='instruction',pc=0x38e175,occurrence=1,writes=[[route0+24,draw()]]))
  flags=0x202|(draw()&0xcd5);flags&=~0x100
  for reg,value in zip(regs,gp):cpu.reg_write(reg,value)
  cpu.reg_write(R.UC_X86_REG_EFLAGS,flags);state.clear();state.update(pc=0,counts={},visits={},changes=changes,events=[],instructions=[],interrupt=interrupt,fifo=fifo,fifo_reads=0)
  cpu.emu_start(entry,0,count=10000);assert state.get('returned')or state.get('interrupted')
  expected_flags=cpu.reg_read(R.UC_X86_REG_EFLAGS);mask=0xcd5 if not interrupt or interrupt[0]!=0x38e180 else 0xc5
  rows.append(dict(name=f'pio_{entry:08x}_{index}',entry=entry,fill=fill,initial_memory=fields,gp=gp,eflags=flags,fifo=fifo,changes=changes,interrupt=interrupt,
   returned=state.get('returned',False),end_pc=cpu.reg_read(R.UC_X86_REG_EIP),expected_gp=[cpu.reg_read(r)&0xffffffff for r in regs],expected_eflags=expected_flags,flags_mask=mask,
   fifo_reads=state['fifo_reads'],instructions=state['instructions'],events=state['events']))
 report=dict(scope=__doc__,original_xbe_sha256=NEW_PIN,cases=rows,bank=bank,extent=extent,stop=stop,executed_original_instructions=executed,
  original_instruction_patches=[],substituted_callees=[],source_FIFO_values_and_MMIO_sinks_declared=True,device_completion_or_startup_verified=False,
  tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),retained_large_artifacts=[])
 raw=json.dumps(report,separators=(',',':'))+'\n';assert len(raw)<1024*1024,len(raw);a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(raw)
 print(json.dumps(dict(cases=len(rows),returned=sum(v['returned']for v in rows),interrupted=sum(not v['returned']for v in rows),instruction_sites=len(executed),bytes=len(raw))))


if __name__=='__main__':main()
