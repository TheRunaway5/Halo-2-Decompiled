#!/usr/bin/env python3
"""Run unchanged TU1.5 heap construction and clients on fresh native pages.

The sole platform import adapts actual anonymous host storage. Explicit caller
ABI/argument/payload inputs are retained. No captured heap, prepared allocator,
source call replacement, mode transition or patched instruction is supplied.
"""
import argparse,ctypes,hashlib,json,mmap,struct
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from research_physics_main_heap_tu15_20261012 import WORLD,WORLD_PIN,POOL,POOL_PIN,sha
REPORT=ROOT/'analysis/multiplayer/sandbox-source/physics-main-heap-tu15-20261012/correspondence-v1/complete-report.json'
REPORT_PIN='579a526dba8c9fb6e090b78c79c906487935b1d74b9c4b8ef601b22d71277de4'
GP=[getattr(R,'UC_X86_REG_'+s)for s in ('EAX','ECX','EDX','EBX','ESP','EBP','ESI','EDI')]
STACK,SP,STOP=0x2000000,0x200f000,0x3000000
SIZES=(1,8,9,16,17,32,33,48,49,64,65,96,97,128,129,160,161,192,193,256,257,320,321,512,513,1024,1025,2048,2049,4096,4097,8192)
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,required=True);a=ap.parse_args()
 assert sha(NEW_DEFAULT)==NEW_PIN and sha(REPORT)==REPORT_PIN and sha(WORLD)==WORLD_PIN and sha(POOL)==POOL_PIN
 paired=json.loads(REPORT.read_text());world=json.loads(WORLD.read_text());pool=json.loads(POOL.read_text())
 mapping={int(p,16):q for p,q in world['entries'].items()};mapping.update({int(p,16):q for p,q in paired['entries'].items()})
 imap={p:q for d in (world,pool,paired)for p,q in d['instruction_map'].items()};instructions=set(imap.values())
 globals=[v for k,v in sorted(paired['globals'].items())];sequences=[];executed=set();counts={}
 a.output.mkdir(parents=True,exist_ok=False);(a.output/'storage').mkdir()
 image=Xbe(NEW_DEFAULT)
 def persist(raw):
  name='storage/'+hashlib.sha256(raw).hexdigest()+'.bin';p=a.output/name
  if not p.exists():p.write_bytes(raw)
  return name
 for layout,first in enumerate((0x10000000,0x11003000,0x12007000)):
  g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0,0x810000)
  for sec in image.sections:g.mem_write(sec.va,image.section_bytes(sec))
  g.mem_map(STACK,0x10000);g.mem_map(STOP,0x1000);g.mem_map(0x80000000,0x1000)
  backing={};resources=[];stores=[];rows=[];site=[0];next_token=[first];frontiers=[]
  def u(p):return struct.unpack('<I',g.mem_read(p,4))[0]
  def put(p,v):g.mem_write(p,struct.pack('<I',v))
  def abi():return dict(gp=[g.reg_read(r)for r in GP],eflags=g.reg_read(R.UC_X86_REG_EFLAGS),
   mxcsr=g.reg_read(R.UC_X86_REG_MXCSR),x87_control=g.reg_read(R.UC_X86_REG_FPCW),x87_status=g.reg_read(R.UC_X86_REG_FPSW),
   xmm=[f'{g.reg_read(getattr(R,f"UC_X86_REG_XMM{i}")):032x}'for i in range(8)])
  def hook(uc,p,n,user):
   if p==0x800000b8:
    sp=uc.reg_read(R.UC_X86_REG_ESP);assert u(sp)==0x33b3c5
    bp,zero,np,flags,protection=[u(sp+4+i*4)for i in range(5)]
    request=[u(bp),zero,u(np),flags,protection];assert request==[0,0,0x106470,0x101000,4]
    size=(request[2]+4095)&~4095;token=(next_token[0]+4095)&~4095
    raw=mmap.mmap(-1,size,prot=mmap.PROT_READ|mmap.PROT_WRITE)
    uc.mem_map_ptr(token,size,3,ctypes.addressof(ctypes.c_ubyte.from_buffer(raw)))
    backing[token]=raw;next_token[0]=token+size
    resources.append(dict(allocation=True,request=request,result=[0,token,size],entry_abi=abi(),initial=persist(raw[:])))
    put(bp,token);put(np,size);uc.reg_write(R.UC_X86_REG_EAX,0);uc.reg_write(R.UC_X86_REG_ESP,sp+24)
    uc.reg_write(R.UC_X86_REG_EIP,u(sp));return
   if p==0x33b3ac:frontiers.append(abi())
   assert p in instructions and bytes(uc.mem_read(p,n))==image.read(p,n),hex(p)
   executed.add(p);counts[p]=counts.get(p,0)+1;site[0]=p
  def write(uc,access,p,n,value,user):
   if STACK<=p<STACK+0x10000:return
   assert any(b<=p and p+n<=b+len(raw)for b,raw in backing.items()) or any(b<=p and p+n<=b+4 for b in globals),hex(p)
   stores.append(dict(instruction=site[0],address=p,before=bytes(uc.mem_read(p,n)).hex(),written=(value&((1<<(8*n))-1)).to_bytes(n,'little').hex()))
  g.hook_add(UC_HOOK_CODE,hook);g.hook_add(UC_HOOK_MEM_WRITE,write)
  def run(entry,kind,regs=None,args=(),inputs=()):
   values=[0x11223344,0x22334455,0x33445566,0x44556677,SP,0x66778899,0x778899aa,0x8899aabb]
   for k,v in (regs or {}).items():values[k]=v
   for r,v in zip(GP,values):g.reg_write(r,v)
   g.reg_write(R.UC_X86_REG_EFLAGS,2);g.reg_write(R.UC_X86_REG_FPCW,0x27f);g.reg_write(R.UC_X86_REG_FPSW,0)
   g.reg_write(R.UC_X86_REG_FPTAG,0xffff);g.reg_write(R.UC_X86_REG_MXCSR,0x1f80)
   for i in range(8):g.reg_write(getattr(R,f'UC_X86_REG_XMM{i}'),int('102030405060708090a0b0c0d0e0f000',16)+i)
   put(SP,STOP)
   for i,v in enumerate(args):put(SP+4+4*i,v)
   for inp in inputs:g.mem_write(inp['address'],bytes.fromhex(inp['bytes']))
   before=abi();begin=len(stores);rb=len(resources)
   g.emu_start(mapping[entry],STOP,count=2000000);assert g.reg_read(R.UC_X86_REG_EIP)==STOP,kind
   rows.append(dict(entry=entry,kind=kind,arguments=args,inputs=inputs,entry_abi=before,exit_abi=abi(),
    stores=stores[begin:],resource_begin=rb,resource_end=len(resources),globals=[u(p)for p in globals],live_allocations=len(backing)))
   return g.reg_read(R.UC_X86_REG_EAX)
  try:
   initial=[bytes(g.mem_read(p,4)).hex()for p in globals]
   raw=run(0x1482a0,'fresh_raw_main_heap');base=u(paired['globals']['00480d90'])
   assert base==raw+0x1000 and u(base-8)==0x105470 and u(base-4)==0x1000
   run(0x148190,'construct_fixed_and_size_class_heaps')
   fixed=u(paired['globals']['00480d9c']);heap=u(paired['globals']['00480da0'])
   assert fixed==base and heap==base+0x15040 and u(paired['globals']['00480d98'])==1
   allocations=[]
   for n in SIZES:
    p=run(0x22e950,'size_class_boundary_allocate',{1:heap},(n,0x13572468));assert p
    allocations.append((p,n))
   assert run(0x22e950,'oversize_declined',{1:heap},(8193,0))==0
   # Real page exhaustion, with no output pointer supplied to the source.
   for _ in range(150):
    p=run(0x22e950,'page_allocate_until_exhaustion',{1:heap},(8192,0))
    if not p:break
    allocations.append((p,8192))
   else:raise AssertionError('No source heap exhaustion')
   for i,(p,n) in enumerate(allocations[::2]):
    payload=bytes((i+k+17)&255 for k in range(n))
    run(0x22eb20,'free_alternate_size_class',{1:heap},(p,n,0x24681357),(dict(address=p,bytes=payload.hex()),))
   replacements=[]
   for p,n in allocations[::2]:
    q=run(0x22e950,'reuse_freed_size_class',{1:heap},(n,0));assert q;replacements.append((q,n))
   assert run(0x22e950,'still_page_exhausted',{1:heap},(8192,0))==0
   for p,n in allocations[1::2]+replacements:run(0x22eb20,'retire_size_class',{1:heap},(p,n,0))
   # The constructor really selected mode 1. Exercise facade tail-dispatch and
   # source address-range free dispatch, without writing a fake mode 2.
   fixed_alloc=[]
   for i in range(100):
    n=(16,33,256,1024,4096)[i%5]
    p=run(0x148a10,'fixed_facade_allocate',args=(n,0))
    if not p:break
    fixed_alloc.append((p,n))
   else:raise AssertionError('No fixed arena exhaustion')
   for p,n in fixed_alloc[::2]:run(0x148a70,'fixed_facade_free_hole',args=(p,n,0))
   for _ in range(8):
    p=run(0x148a10,'fixed_facade_refill_holes',args=(33,0));assert p
    fixed_alloc.append((p,33))
   live=fixed_alloc[1:len(fixed_alloc)-8:2]+fixed_alloc[-8:]
   for p,n in reversed(live):run(0x148a70,'fixed_facade_retire',args=(p,n,0))
   run(0x148a70,'null_free_is_noop',args=(0,8192,0x24681357))
   final=[dict(address=p,bytes=len(raw),storage=persist(bytes(g.mem_read(p,len(raw)))))for p,raw in backing.items()]
   sequences.append(dict(name=f'fresh-main-heap-layout-{layout}',first_token=first,initial_globals=initial,producer_frontier=frontiers[0],resources=resources,steps=rows,final_storage=final))
  finally:
   for raw in backing.values():raw.close()
 report=dict(schema=1,tool_sha256=sha(__file__),original_sha256=NEW_PIN,correspondence_sha256=REPORT_PIN,
  world_correspondence_sha256=WORLD_PIN,pool_correspondence_sha256=POOL_PIN,canonical_instruction_map=imap,
  stack=STACK,stack_bytes=0x10000,stop=STOP,globals=globals,sequences=sequences,
  executed_sites=sorted(executed),instruction_count=sum(counts.values()),instruction_counts={f'{p:08x}':v for p,v in counts.items()},
  source_calls_replaced=False,captured_heap=False,live_world_startup=False,
  declared_native_abi='Anonymous host mmap, 32-bit owned tokens, stdcall preserves GP/FP/flags except EAX/ESP; no Xbox kernel-clobber claim')
 (a.output/'fixture.json').write_text(json.dumps(report,sort_keys=True,indent=2)+'\n')
 print(json.dumps(dict(sha256=sha(a.output/'fixture.json'),steps=sum(len(s['steps'])for s in sequences),instructions=sum(counts.values()),stores=sum(len(r['stores'])for s in sequences for r in s['steps']),executed_sites=len(executed))))
if __name__=='__main__':main()
