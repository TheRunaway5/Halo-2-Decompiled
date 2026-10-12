#!/usr/bin/env python3
"""Unchanged TU1.5 physical stage and partitions on actual anonymous pages."""
import argparse,ctypes,hashlib,json,mmap,struct
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import ROOT,Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from research_physics_physical_storage_tu15_20261012 import sha
REPORT=ROOT/'analysis/multiplayer/sandbox-source/physics-physical-storage-tu15-20261012/correspondence-v1/complete-report.json'
PIN='4e81e7a5fd05edae4d24a35152896df4191aed827da55ba1d87b559bcb8e540f'
GP=[getattr(R,'UC_X86_REG_'+s)for s in ('EAX','ECX','EDX','EBX','ESP','EBP','ESI','EDI')]
STACK,SP,STOP=0x2000000,0x200f000,0x3000000
PHYSICAL,PHYSICAL_BYTES=0x80061000,0x30e4000

def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,required=True);a=ap.parse_args()
 assert sha(REPORT)==PIN and sha(NEW_DEFAULT)==NEW_PIN;d=json.loads(REPORT.read_text());mapping={int(p,16):q for p,q in d['entries'].items()}
 imap=d['instruction_map'];instructions=set(imap.values());amap={int(p,16):q for p,q in d['address_map'].items()}
 globals=[amap.get(p,p)for p in (0x4ed9f0,0x4ed9f4,0x4ed9f8,0x4ed9fc,0x4eda10,0x5098b0,0x5098b4,0x5098b8)]
 a.output.mkdir(parents=True,exist_ok=False);(a.output/'storage').mkdir();image=Xbe(NEW_DEFAULT);sequences=[];executed=set();counts={}
 def persist(raw):
  name='storage/'+hashlib.sha256(raw).hexdigest()+'.bin';p=a.output/name
  if not p.exists():p.write_bytes(raw)
  return name
 for case in range(3):
  g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0,0x810000)
  for sec in image.sections:g.mem_write(sec.va,image.section_bytes(sec))
  g.mem_map(STACK,0x10000);g.mem_map(STOP,0x1000);g.mem_map(0x80000000,0x1000)
  backing={};resources=[];stores=[];rows=[];site=[0]
  def u(p):return struct.unpack('<I',g.mem_read(p,4))[0]
  def put(p,v):g.mem_write(p,struct.pack('<I',v))
  def abi():return dict(gp=[g.reg_read(r)for r in GP],eflags=g.reg_read(R.UC_X86_REG_EFLAGS),mxcsr=g.reg_read(R.UC_X86_REG_MXCSR),
   x87_control=g.reg_read(R.UC_X86_REG_FPCW),x87_status=g.reg_read(R.UC_X86_REG_FPSW),xmm=[f'{g.reg_read(getattr(R,f"UC_X86_REG_XMM{i}")):032x}'for i in range(8)])
  def hook(uc,p,n,user):
   if p in (0x800000a6,0x800000b6):
    sp=uc.reg_read(R.UC_X86_REG_ESP);frontier=abi()
    if p==0x800000a6:
     assert u(sp)==0x2db489;q=[u(sp+4+4*i)for i in range(5)]
     assert q==[PHYSICAL_BYTES,0x61000,0x3144fff,0,4] and not backing
     raw=mmap.mmap(-1,PHYSICAL_BYTES,prot=mmap.PROT_READ|mmap.PROT_WRITE)
     uc.mem_map_ptr(PHYSICAL,PHYSICAL_BYTES,3,ctypes.addressof(ctypes.c_ubyte.from_buffer(raw)));backing[PHYSICAL]=raw
     result=PHYSICAL;initial=persist(raw[:]);uc.reg_write(R.UC_X86_REG_EAX,result);pop=24
    else:
     q=[u(sp+4+4*i)for i in range(3)]+[0,0];address,size,prot=q[:3]
     assert size>0 and size%4096==0 and address%4096==0 and prot==4
     assert PHYSICAL<=address and address+size<=PHYSICAL+PHYSICAL_BYTES
     host=ctypes.addressof(ctypes.c_ubyte.from_buffer(backing[PHYSICAL]))+(address-PHYSICAL)
     libc=ctypes.CDLL(None,use_errno=True);libc.mprotect.argtypes=(ctypes.c_void_p,ctypes.c_size_t,ctypes.c_int)
     assert libc.mprotect(host,size,mmap.PROT_READ|mmap.PROT_WRITE)==0
     result=uc.reg_read(R.UC_X86_REG_EAX);initial=None;pop=16
    resources.append(dict(allocation=p==0x800000a6,request=q,result=result,entry_abi=frontier,initial=initial))
    uc.reg_write(R.UC_X86_REG_ESP,sp+pop);uc.reg_write(R.UC_X86_REG_EIP,u(sp));return
   assert p in instructions and bytes(uc.mem_read(p,n))==image.read(p,n),hex(p)
   executed.add(p);counts[p]=counts.get(p,0)+1;site[0]=p
  def write(uc,access,p,n,value,user):
   if STACK<=p<STACK+0x10000:return
   assert any(b<=p and p+n<=b+4 for b in globals)or amap.get(0x4ed9f0,0x4ed9f0)<=p<amap.get(0x4ed9f0,0x4ed9f0)+52,hex(p)
   stores.append(dict(instruction=site[0],address=p,before=bytes(uc.mem_read(p,n)).hex(),written=(value&((1<<(8*n))-1)).to_bytes(n,'little').hex()))
  g.hook_add(UC_HOOK_CODE,hook);g.hook_add(UC_HOOK_MEM_WRITE,write)
  def run(entry,kind,regs=None,args=()):
   values=[0x11223344,0x22334455,0x33445566,0x44556677,SP,0x66778899,0x778899aa,0x8899aabb]
   for k,v in (regs or {}).items():values[k]=v
   for r,v in zip(GP,values):g.reg_write(r,v)
   g.reg_write(R.UC_X86_REG_EFLAGS,2);g.reg_write(R.UC_X86_REG_FPCW,0x27f);g.reg_write(R.UC_X86_REG_FPSW,0)
   g.reg_write(R.UC_X86_REG_FPTAG,0xffff);g.reg_write(R.UC_X86_REG_MXCSR,0x1f80)
   for i in range(8):g.reg_write(getattr(R,f'UC_X86_REG_XMM{i}'),int('102030405060708090a0b0c0d0e0f000',16)+i)
   put(SP,STOP)
   for i,v in enumerate(args):put(SP+4+4*i,v)
   before=abi();begin=len(stores);rb=len(resources);g.emu_start(mapping[entry],STOP,count=2000000)
   assert g.reg_read(R.UC_X86_REG_EIP)==STOP,kind
   rows.append(dict(entry=entry,kind=kind,arguments=args,inputs=[],entry_abi=before,exit_abi=abi(),stores=stores[begin:],
    resource_begin=rb,resource_end=len(resources),globals=[u(p)for p in globals],live_allocations=len(backing)))
   return g.reg_read(R.UC_X86_REG_EAX)
  try:
   initial=[bytes(g.mem_read(p,4)).hex()for p in globals]
   assert run(0x12cb70,'fresh_physical_stage')==PHYSICAL
   assert run(0x217020,'actual_arena_partition_request',{0:0x40000,1:0x3be000})==0x82d47000
   if case==0:requests=((0,0),(1,0),(0x31,0x12),(0x1000,0x2000),(0x1111,0x2222))
   elif case==1:requests=((0x40000,0x3be000),)*10
   else:requests=((0x2000000,0),(0x10000,1),(1,0x2000),(0x100000,0))
   for x,y in requests:run(0x217020,'declared_partition_request',{0:x,1:y})
   # Real exhaustion retains the original published pointer/size behavior.
   for _ in range(32):
    if not run(0x217020,'partition_until_exhaustion',{0:0x40000,1:0x3be000}):break
   else:raise AssertionError('No original partition exhaustion')
   assert run(0x217020,'oversized_partition_declined',{0:0x4000000,1:0})==0
   run(0x2d7a1a,'zero_length_protection_wrapper',args=(PHYSICAL,0,4))
   final=[dict(address=p,bytes=len(raw),storage=persist(bytes(g.mem_read(p,len(raw)))))for p,raw in backing.items()]
   sequences.append(dict(name=f'fresh-physical-stage-{case}',initial_globals=initial,resources=resources,steps=rows,final_storage=final))
  finally:
   for raw in backing.values():raw.close()
 f=dict(schema=1,tool_sha256=sha(__file__),original_sha256=NEW_PIN,correspondence_sha256=PIN,canonical_instruction_map=imap,
  stack=STACK,stack_bytes=0x10000,stop=STOP,globals=globals,sequences=sequences,executed_sites=sorted(executed),instruction_count=sum(counts.values()),
  captured_storage=False,source_calls_replaced=False,live_world_startup=False)
 (a.output/'fixture.json').write_text(json.dumps(f,sort_keys=True,indent=2)+'\n');print(json.dumps(dict(sha256=sha(a.output/'fixture.json'),steps=sum(len(s['steps'])for s in sequences),stores=sum(len(r['stores'])for s in sequences for r in s['steps']),instructions=f['instruction_count'],sites=len(executed))))
if __name__=='__main__':main()
