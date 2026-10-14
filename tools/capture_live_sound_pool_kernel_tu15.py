"""Observe real kernel pool allocation/capacity while unchanged TU1.5 creates its SDK device.

The owned Complex4627/xemu platform is identified explicitly. This is kernel
reference evidence for that platform, not proof of physical retail-kernel parity.
No guest code, register, memory, allocation or callee return is supplied.
"""
from pathlib import Path
import sys,json,hashlib
sys.path.insert(0,str(Path(__file__).resolve().parent))
from capture_live_sound_environment_tu15 import *

def capture_pool():
 stop=breakpoint(0x387877);gdb.execute('continue');assert reg('eip')==0x387877;stop.delete()
 text=image.section('.text');assert read(text.va,text.rsize)==image.section_bytes(text)
 for listing in sorted((ROOT/'work/sandbox-audio-v10/generated-sdk').glob('*.asm')):
  for row in listing.read_text().splitlines():
   pc,raw,*_=row.split();pc=int(pc,16);raw=bytes.fromhex(raw);assert read(pc,len(raw))==raw==image.read(pc,len(raw))
 allocate=word(0x41b79c);capacity=word(0x41b7ec);parent=word(reg('esp'));assert parent==0x224f2b
 lower=min(allocate,capacity)&~4095;upper=(max(allocate,capacity)+0x800+4095)&~4095;kernel=read(lower,upper-lower)
 (out/'kernel-pool-code.bin').write_bytes(kernel);rows=[];whole=breakpoint(parent)
 for number in range(1,513):
  active=[breakpoint(allocate),breakpoint(capacity)];gdb.execute('continue');pc=reg('eip')
  for b in active:b.delete()
  if pc==parent:break
  assert pc in(allocate,capacity);sp=reg('esp');ret=word(sp);kind='allocate'if pc==allocate else'capacity';args=[word(sp+4+i*4)for i in range(2 if kind=='allocate'else 1)]
  before=abi();header_before=read(args[0]-16,16).hex()if kind=='capacity'else None
  returning=breakpoint(ret);gdb.execute('continue');assert reg('eip')==ret;returning.delete();after=abi();result=reg('eax')
  header_after=read(result-16,16).hex()if kind=='allocate'and result else None
  row=dict(id=number,kind=kind,entry=pc,return_pc=ret,arguments=args,result=result,abi_before=before,abi_after=after,allocation_prefix16_hex=header_after,query_prefix16_hex=header_before)
  rows.append(row);print(kind,number,[hex(a)for a in args],hex(result),flush=True)
 else:raise RuntimeError('Original SDK constructor exceeded pool observation limit')
 whole.delete();assert reg('eax')==0 and word(word(0x52d674)+0x2ab0)
 assert read(lower,len(kernel))==kernel
 report=dict(scope=__doc__,original_sha256=PIN,original_cold_boot=True,source_factory_returned_normally=True,original_factory_result=reg('eax'),kernel_code=dict(address=lower,bytes=len(kernel),sha256=hashlib.sha256(kernel).hexdigest(),allocation_entry=allocate,capacity_entry=capacity),rows=rows,
 platform=dict(bios_path='/mnt/SSD4TB/Games/Xemu/bios/Complex_4627.bin',bios_sha256=hashlib.sha256(Path('/mnt/SSD4TB/Games/Xemu/bios/Complex_4627.bin').read_bytes()).hexdigest(),emulator_path=str(ROOT/'.tools/xemu-app/squashfs-root/usr/bin/xemu'),emulator_sha256=hashlib.sha256((ROOT/'.tools/xemu-app/squashfs-root/usr/bin/xemu').read_bytes()).hexdigest()),
 original_code_patches=[],original_memory_writes=[],original_register_writes=[],supplied_callee_returns=[],physical_retail_kernel_or_pcm_verified=False,tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest())
 (out/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(dict(rows=len(rows),kernel_code=report['kernel_code'],source_factory_returned_normally=True)),flush=True)
