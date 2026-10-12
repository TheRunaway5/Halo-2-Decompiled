#!/usr/bin/env python3
"""Execute unchanged renderer storage through its actual process-heap frontier.

Fresh host mmap pages and explicit standalone CPU/stack inputs only. Source
constructs both parameter arrays and publishes its renderer base/context.
No child descriptor, process heap, allocation result, constructor completion
or renderer usability is supplied. The actual first ordinary allocation is
retained as an unresolved CALL, not skipped or replaced.
"""
import argparse,hashlib,json,mmap,struct
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
E=ROOT/'analysis/multiplayer/sandbox-source/physics-renderer-storage-tu15-20261012'
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,required=True);a=ap.parse_args();a.output.mkdir(parents=True,exist_ok=False)
 assert sha(NEW_DEFAULT)==NEW_PIN
 paired_path=E/'correspondence-v1/complete-report.json';assert sha(paired_path)=='59dd620260f89a6ccc4900879f7c39281117feeec6eff042a6722f6c055f83d5'
 pool_path=E.parent/'physics-pool-startup-tu15-20261012/correspondence-v1/complete-report.json'
 paired,pool=[json.loads(p.read_text())for p in (paired_path,pool_path)]
 imap={p:q for d in (paired,pool)for p,q in d['instruction_map'].items()};allowed=set(imap.values())
 image=Xbe(NEW_DEFAULT);code=_Code(image,'.text');stack,stop=0x60000000,0x60200000
 regs=[getattr(R,'UC_X86_REG_'+s)for s in ('EAX','ECX','EDX','EBX','ESP','EBP','ESI','EDI')]
 cases=[];executed=set();count=0
 for token in (0x10000000,0x11003000,0x12007000):
  for flags in (2,0xcd7):
   for shift in (0,4):
    g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0,0x810000)
    for sec in image.sections:g.mem_write(sec.va,image.section_bytes(sec))
    g.mem_map(stack,0x10000);initial=bytes((k*13+7)&255 for k in range(0x10000));g.mem_write(stack,initial)
    g.mem_map(stop,0x1000);g.mem_map(0x80000000,0x1000)
    stores=[];resources=[];backing=[];site=[0];pending=[False];observations=[0];frontiers=[]
    def u(p):return struct.unpack('<I',g.mem_read(p,4))[0]
    def put(p,v):g.mem_write(p,struct.pack('<I',v))
    def abi():return dict(gp=[g.reg_read(r)for r in regs],eflags=g.reg_read(R.UC_X86_REG_EFLAGS),x87_control=g.reg_read(R.UC_X86_REG_FPCW),x87_status=g.reg_read(R.UC_X86_REG_FPSW),mxcsr=g.reg_read(R.UC_X86_REG_MXCSR),xmm=[f'{g.reg_read(getattr(R,f"UC_X86_REG_XMM{i}")):032x}'for i in range(8)])
    def hook(uc,p,n,user):
     sp=g.reg_read(R.UC_X86_REG_ESP)
     if p==0x32a3bb:
      assert u(sp)==0x137483 and u(sp+4)==0x18
      pending[0]=True;g.emu_stop();return
     if p==0x33b3ac:frontiers.append(dict(target=p,abi=abi(),stack=bytes(g.mem_read(sp,20)).hex()))
     if p==0x800000b8:
      assert u(sp)==0x33b3c5
      base,zero,size,kind,protection=[u(sp+4+i*4)for i in range(5)]
      request=[u(base),zero,u(size),kind,protection];assert request==[0,0,0x452e8,0x101000,4]
      n=(request[2]+4095)&~4095;raw=mmap.mmap(-1,n);g.mem_map(token,n);g.mem_write(token,raw[:]);backing.append(raw)
      resources.append(dict(request=request,result=[0,token,n],initial_sha256=hashlib.sha256(raw[:]).hexdigest()))
      put(base,token);put(size,n);g.reg_write(R.UC_X86_REG_EAX,0);g.reg_write(R.UC_X86_REG_ESP,sp+24);g.reg_write(R.UC_X86_REG_EIP,u(sp));return
     assert p in allowed and bytes(g.mem_read(p,n))==image.read(p,n),hex(p)
     site[0]=p;executed.add(p);observations[0]+=1
    def write(uc,access,p,n,value,user):
     if stack<=p<stack+0x10000:return
     stores.append(dict(instruction=site[0],address=p,before=bytes(g.mem_read(p,n)).hex(),written=(value&((1<<(8*n))-1)).to_bytes(n,'little').hex()))
    g.hook_add(UC_HOOK_CODE,hook);g.hook_add(UC_HOOK_MEM_WRITE,write)
    sp=stack+0xf000+shift;put(sp,stop)
    for r,v in zip(regs,(0x11223344,0x22334455,0x33445566,0x44556677,sp,0x66778899,0x778899aa,0x8899aabb)):g.reg_write(r,v)
    g.reg_write(R.UC_X86_REG_EFLAGS,flags);g.reg_write(R.UC_X86_REG_FPCW,0x27f);g.reg_write(R.UC_X86_REG_FPSW,0);g.reg_write(R.UC_X86_REG_FPTAG,0xffff);g.reg_write(R.UC_X86_REG_MXCSR,0x1f80)
    for i in range(8):g.reg_write(getattr(R,f'UC_X86_REG_XMM{i}'),int('102030405060708090a0b0c0d0e0f000',16)+i)
    before=abi()
    try:
     g.emu_start(paired['entries']['00133760'],stop,count=100000);assert pending[0] and g.reg_read(R.UC_X86_REG_EIP)==0x32a3bb
     finals=[dict(address=s.va,bytes=s.vsize,sha256=hashlib.sha256(bytes(g.mem_read(s.va,s.vsize))).hexdigest())for s in image.sections if s.flags&1]
     finals.append(dict(address=token,bytes=len(backing[0]),sha256=hashlib.sha256(bytes(g.mem_read(token,len(backing[0])))).hexdigest()))
     cases.append(dict(token=token,shift=shift,entry_abi=before,exit_abi=abi(),pending_stack=bytes(g.mem_read(g.reg_read(R.UC_X86_REG_ESP),8)).hex(),stores=stores,resources=resources,missing_raw_owner=frontiers[0],final_banks=finals,renderer_base=u(0x51df44),context_base=u(0x556e18)))
     count+=observations[0]
    finally:
     for raw in backing:raw.close()
 listing=a.output/'executed-original.asm';listing.write_text(''.join(f'{p:08x} {code.at(p).bytes.hex()} {code.at(p).mnemonic} {code.at(p).op_str}\n'for p in sorted(executed)))
 d=dict(schema=1,oracle_sha256=sha(__file__),source_sha256=NEW_PIN,correspondence_sha256=sha(paired_path),pool_correspondence_sha256=sha(pool_path),canonical_instruction_map=imap,stack=stack,stop=stop,cases=cases,original_instruction_count=count,executed_sites=len(executed),ordered_writes=sum(len(c['stores'])for c in cases),missing_allocator_target=0x32a3bb,missing_allocator_return=0x137483,instruction_listing_sha256=sha(listing),instruction_patches=[],supplied_source_results=[],complete_renderer_producer_return=False,live_renderer_or_physics_parity=False,boundaries=__doc__)
 p=a.output/'fixture.json';p.write_text(json.dumps(d,sort_keys=True,indent=2)+'\n');print(json.dumps(dict(sha256=sha(p),cases=len(cases),instructions=count,sites=len(executed),stores=d['ordered_writes'])))
if __name__=='__main__':main()
