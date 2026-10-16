#!/usr/bin/env python3
"""Reference-only EP cursor requests and bounded authentic SGE/range construction.

This does not return from38E2E0, configure AC97 or execute DSP. The CPU interval
38E36B..38E3E6 receives declared stack/register inputs and the real descriptors
computed by the earlier whole SDK storage reference. Byte-queue probes use an
explicit empty native queue at the pending offset, not a retail consumer cursor.
"""
import argparse,ctypes,hashlib,json,mmap,struct
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
from unicorn import x86_const as R

ROOT=Path(__file__).resolve().parents[1]
PRIMARY=ROOT/'analysis/asset-setup/mcc-native-20261015/native-startup-45-50/audio-platform-map/xemu-primary'

def probe(base,end,initial):
 memory=bytearray(end-base);read=write=initial;queue=bytearray();rows=[]
 capacity=end-base
 operations=[('write',capacity-7,17),('write',8,29),('read',capacity-11,0),('write',capacity-4,91),('read',capacity,0),('read',1,0),('write',capacity+1,7),('read',0,0),('write',0,0)]
 for kind,size,seed in operations:
  pattern=bytes((i*37+seed)&255 for i in range(size));accepted=size<=(capacity-len(queue)if kind=='write'else len(queue));returned=b''
  if accepted:
   cursor=write if kind=='write'else read;remaining=size;position=0;result=bytearray()
   # Same bounded two-segment/wrap rule as the pinned primary implementation.
   while remaining:
    count=min(end-cursor,remaining);offset=cursor-base
    if kind=='write':memory[offset:offset+count]=pattern[position:position+count]
    else:result.extend(memory[offset:offset+count])
    cursor+=count;position+=count;remaining-=count
    if cursor==end:cursor=base
   if kind=='write':write=cursor;queue.extend(pattern)
   else:
    read=cursor;returned=bytes(result);assert returned==bytes(queue[:size]);del queue[:size]
  rows.append(dict(kind=kind,bytes=size,input_hex=pattern.hex()if kind=='write'else None,accepted=accepted,returned_hex=returned.hex(),read_cursor=read,write_cursor=write,queued=len(queue),ring_hex=memory.hex()))
 return dict(base=base,end=end,initial=initial,empty_native_queue_policy=True,operations=rows)

def main():
 parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--storage-reference',type=Path,required=True);parser.add_argument('--output',type=Path,required=True);a=parser.parse_args();assert not a.output.exists()
 image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN
 reference=json.loads(a.storage_reference.read_text());assert reference['passed']and reference['original_xbe_sha256']==NEW_PIN
 assert not reference['instruction_patches']and not reference['supplied_game_or_sdk_results']
 storage=reference['cases'][0];assert storage['through_storage']and len(storage['dma_allocations'])==16
 cpu=Uc(UC_ARCH_X86,UC_MODE_32)
 for address,size in [(0x10000,0x600000),(0x2000000,0x20000),(0x2100000,4096),(0xfe800000,0x60000)]:cpu.mem_map(address,size)
 for section in image.sections:
  raw=image.section_bytes(section)
  if section.va+len(raw)<=0x610000:cpu.mem_write(section.va,raw)
 for row in storage['regions']:cpu.mem_write(row['address'],bytes.fromhex(row['hex']))
 pages=[]
 for row in storage['dma_allocations']:
  raw=mmap.mmap(-1,row['capacity'],flags=mmap.MAP_PRIVATE|mmap.MAP_ANONYMOUS,prot=mmap.PROT_READ|mmap.PROT_WRITE)
  raw[:]=bytes.fromhex(row['hex']);native=ctypes.addressof(ctypes.c_char.from_buffer(raw));assert native>0xffffffff
  cpu.mem_map_ptr(row['source'],row['capacity'],3,native);pages.append((row['source'],row['capacity'],raw))
 regs=[getattr(R,'UC_X86_REG_'+n)for n in ['EAX','ECX','EDX','EBX','ESP','EBP','ESI','EDI']]
 gp=[0x11000000+i*0x111111 for i in range(8)];gp[3]=0;gp[5]=0x2009000;gp[4]=gp[5]-0x24
 for reg,value in zip(regs,gp):cpu.reg_write(reg,value)
 cpu.reg_write(R.UC_X86_REG_EFLAGS,0x202)
 instructions=[];stores=[];executed={}
 def hook(m,pc,size,_):
  raw=bytes(m.mem_read(pc,size));assert raw==image.read(pc,size);assert executed.setdefault(f'{pc:08x}',raw.hex())==raw.hex();instructions.append(pc)
 def write(m,access,address,size,value,_):stores.append([m.reg_read(R.UC_X86_REG_EIP),address,size,value&((1<<(8*size))-1)])
 cpu.hook_add(UC_HOOK_CODE,hook);cpu.hook_add(UC_HOOK_MEM_WRITE,write)
 try:
  cpu.emu_start(0x38e36b,0x38e3e6,count=10000);assert cpu.reg_read(R.UC_X86_REG_EIP)==0x38e3e6
  data=next(r for r in storage['dma_allocations']if r['source_descriptor']==0x391370)
  table=next(r for r in storage['dma_allocations']if r['source_descriptor']==0x391400)
  entries=[list(struct.unpack('<2I',cpu.mem_read(table['source']+i*8,8)))for i in range(6)]
  assert entries==[[data['physical']+i*4096,0]for i in range(6)]
  ranges=[[struct.unpack('<I',cpu.mem_read(p,4))[0]for p in [0xfe804024+16*i,0xfe804028+16*i]]for i in range(2)]
  assert ranges==[[0,0x2000],[0x2000,0x6000]]
  layout=dict(entry=0x38e36b,end_exclusive=0x38e3e6,declared_input_gp=gp,instructions=instructions.copy(),source_stores=stores.copy(),returned_gp=[cpu.reg_read(r)for r in regs],returned_eflags=cpu.reg_read(R.UC_X86_REG_EFLAGS),data_descriptor={k:v for k,v in data.items()if k!='hex'},table_descriptor={k:v for k,v in table.items()if k!='hex'},page_entries=entries,ranges=ranges)
  instructions.clear();stores.clear();cpu.mem_write(0x2008000,struct.pack('<I',0x2100000));cpu.reg_write(R.UC_X86_REG_ESP,0x2008000)
  cpu.emu_start(0x38e75b,0x2100000,count=10000);assert cpu.reg_read(R.UC_X86_REG_EIP)==0x2100000
  cursor_stores=[row for row in stores if row[1]in [0xfe80402c,0xfe80403c]]
  assert cursor_stores==[[0x38e7c4,0xfe80402c,4,0x600],[0x38e7ca,0xfe80403c,4,0x3800]]
  code=_Code(image,'DSOUND');a.output.mkdir(parents=True)
  for entry in [0x38e75b,0x38e2e0]:
   body=_trace(code,entry,{0x387a32,0x38f5e2,0x38f6f4,0x38f66a,0x38f692,0x38851e,0x38f5ef})
   (a.output/f'original-{entry:08x}.asm').write_text(''.join(f'{pc:08x} {code.at(pc).bytes.hex()} {code.at(pc).mnemonic} {code.at(pc).op_str}\n'for pc in sorted(body.code)))
  manifest=json.loads((PRIMARY/'manifest.json').read_text())
  for row in manifest['files']:assert hashlib.sha256((PRIMARY/row['file']).read_bytes()).hexdigest()==row['sha256']
  report=dict(passed=True,scope=__doc__,original_xbe_sha256=NEW_PIN,tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),storage_reference_sha256=hashlib.sha256(a.storage_reference.read_bytes()).hexdigest(),layout_fragment=layout,pending_cursor_stores=cursor_stores,unowned_scheduler_stores=[r for r in stores if r[1]>=0xfe800000 and r[1]not in [0xfe80402c,0xfe80403c]],cursor_source_instructions=instructions,circular_native_byte_probes=[probe(0,0x2000,0x600),probe(0x2000,0x6000,0x3800)],executed_instruction_bytes=executed,primary_commit=manifest['commit'],primary_files=manifest['files'],instruction_patches=[],whole38e2e0_returned=False,native_dsp_produced_bytes=0,source_frontend_or_ac97_configured=False,source_producer_ready=False)
  (a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(f'EP output: {len(entries)} authentic SGE records; {len(layout["instructions"])} original layout instructions; 2 pending cursors; no DSP producer')
 finally:
  for source,size,raw in pages:cpu.mem_unmap(source,size);raw.close()
if __name__=='__main__':main()
