#!/usr/bin/env python3
"""Observe unchanged TU1.5 stock enqueue/cache-pin and queued-pin release.

All stream/cache records are declared raw ABI inputs, including malformed ones.
The original21F260 request and stock codec ret4 execute unchanged. No allocation,
I/O completion, readiness, source voice or device result is supplied.
"""
import argparse,collections,hashlib,json,os,random
from pathlib import Path
from oracle_sound_stream_packets_tu15 import Xbe,NEW_DEFAULT,NEW_PIN,STACK,STOP,REGS
from unicorn import Uc,UcError,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE,UC_HOOK_MEM_INVALID
from unicorn.x86_const import *
STREAM,CHUNK,CACHE,ENTRIES,ALLOCATOR,OWNER,BLOCKS=[0x4000000+n*0x100000 for n in range(7)]
CODEC,VTABLE=0x48b908,0x4633b4
ENTRY_PCS=[0x2b8210,0x2b85f0,0x2e4d60]
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);args=p.parse_args()
 scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True);args.output=args.output.resolve()
 if not args.output.is_relative_to(scratch):raise ValueError('Fixture must remain in its disposable owner')
 args.output.mkdir(parents=True,exist_ok=False);image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN
 assert image.read(CODEC,4)==VTABLE.to_bytes(4,'little') and image.read(VTABLE+4,4)==(0x2e4d60).to_bytes(4,'little')
 assert image.read(0x2e4d60,3)==bytes.fromhex('c20400')
 assert [int.from_bytes(image.read(0x2b8288+n*4,4),'little')for n in range(4)]==[0x2b822e,0x2b822e,0x2b8234,0x2b823b]
 rng=random.Random(0x2b8210);specs=[]
 for flags in range(256):
  for mode in range(8):specs.append(dict(entry=0x2b8210,flags=flags,mode=mode))
 for mode in range(256):
  for flags in [0,1,2,3,8,16,56,128,255]:specs.append(dict(entry=0x2b8210,flags=flags,mode=mode))
 for flags in range(256):
  for pin in [0,1,254,255]:specs.append(dict(entry=0x2b85f0,flags=flags,pin=pin))
 for flags in [0,1,2,3,8,9,0xc1,0xfe,0xff]:
  for alias in ['pin-is-flags','pin-is-mode','pin-is-first-pointer']:
   specs.append(dict(entry=0x2b8210,flags=flags,alias=alias))
 for entry in ENTRY_PCS[:2]:
  for fault in ['stream','chunk','cache','entries','allocator','owner','blocks','codec','handle']:
   for flags in [0,1,2,3,4,7,8,0xc1]:specs.append(dict(entry=entry,flags=flags,fault=fault))
 for n in range(32):specs.append(dict(entry=0x2e4d60,flags=n))
 counts=collections.Counter();sites=collections.Counter();call_counts=collections.Counter();returns=faults=stores_count=stack_count=0
 header=dict(original_xbe_sha256=NEW_PIN,raw_abi_only=True,platform_results_supplied=[],source_stack_stores_observed=True,
  required_entries=ENTRY_PCS,minimum_cases=5500,native_cpu_callees=[0x21f260,0x2e4d60],
  regions=[[0x463000,4096],[0x48b000,4096],[0x50e000,4096],[0x2000000,0x10000]]+[[a,4096]for a in [STREAM,CHUNK,CACHE,ENTRIES,ALLOCATOR,OWNER,BLOCKS]])
 digest=hashlib.sha256();fixture=args.output/'cases.jsonl'
 with fixture.open('wb')as out:
  def emit(v):
   raw=(json.dumps(v,separators=(',',':'))+'\n').encode();out.write(raw);digest.update(raw)
  emit(header)
  for ordinal,spec in enumerate(specs):
   fields=[]
   def w(a,n,v):fields.append([a,n,v&((1<<(n*8))-1)])
   for a in range(STACK-128,STACK+20,4):w(a,4,rng.getrandbits(32))
   for a,v in [(CODEC,VTABLE),(0x50ebdc,CACHE),(0x50ebe4,ALLOCATOR),(CACHE+0x44,ENTRIES),(ALLOCATOR+0x64,OWNER),(ALLOCATOR+0x38,rng.getrandbits(32)),(OWNER+0x44,BLOCKS),(STREAM+0x30,CODEC)]:w(a,4,v)
   for n in range(9):w(VTABLE+n*4,4,int.from_bytes(image.read(VTABLE+n*4,4),'little'))
   for n in range(3):
    w(STREAM+0x1c+n*4,4,CHUNK+n*0x100);w(CHUNK+n*0x100+8,4,(rng.getrandbits(16)<<16)|n)
    w(ENTRIES+n*16+2,1,ordinal&1);w(ENTRIES+n*16+3,1,(ordinal>>1)&1)
    w(ENTRIES+n*16+4,1,spec.get('pin',[0,1,254,255][ordinal%4]));w(ENTRIES+n*16+5,1,rng.randrange(256))
   w(STREAM,1,1);w(STREAM+1,1,spec.get('mode',2));w(STREAM+2,1,spec['flags']);w(STREAM+3,1,rng.randrange(256));w(STACK,4,STOP);w(STACK+4,4,STREAM)
   alias=spec.get('alias')
   if alias:
    at={'pin-is-flags':STREAM-2,'pin-is-mode':STREAM-3,'pin-is-first-pointer':STREAM+0x18}[alias];w(CACHE+0x44,4,at);w(CHUNK+8,4,0)
   gp=[rng.getrandbits(32)for _ in REGS];gp[4]=STACK;gp[6]=STREAM;gp[7]=CHUNK;gp[1]=STREAM
   fault=spec.get('fault')
   if fault=='stream':gp[6]=gp[1]=0
   elif fault=='chunk':gp[7]=0;w(STREAM+0x1c,4,0)
   elif fault=='cache':w(0x50ebdc,4,0)
   elif fault=='entries':w(CACHE+0x44,4,0)
   elif fault=='allocator':w(0x50ebe4,4,0)
   elif fault=='owner':w(ALLOCATOR+0x64,4,0)
   elif fault=='blocks':w(OWNER+0x44,4,0)
   elif fault=='codec':w(STREAM+0x30,4,0)
   elif fault=='handle':w(CHUNK+8,4,0xffffffff)
   g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x800000)
   for section in image.sections:g.mem_write(section.va,image.section_bytes(section))
   text=image.section('.text');g.mem_protect(text.va&~4095,((text.va%4096+text.vsize+4095)//4096)*4096,5)
   for a,n in header['regions']:
    if a>=0x810000:g.mem_map(a,n)
    g.mem_write(a,bytes(n))
   g.mem_map(STOP,4096)
   for a,n,v in fields:g.mem_write(a,v.to_bytes(n,'little'))
   for reg,v in zip(REGS,gp):g.reg_write(reg,v)
   xmm=[rng.getrandbits(128)for _ in range(8)]
   for n,v in enumerate(xmm):g.reg_write(UC_X86_REG_XMM0+n,v)
   fp=[[0x27f,0x37f,0x77f,0xb7f,0xf7f][ordinal%5],[0x1f80,0x3f80,0x5f80,0x7f80,0x9fc0][ordinal%5],(ordinal%8)<<11]
   for reg,v in zip([UC_X86_REG_FPCW,UC_X86_REG_MXCSR,UC_X86_REG_FPSW],fp):g.reg_write(reg,v)
   g.reg_write(UC_X86_REG_EFLAGS,0x202);stores=[];stack_stores=[];calls=[];bad=[];executed=set()
   def u(a,n=4):return int.from_bytes(g.mem_read(a,n),'little')
   def instruction(_g,a,n,_):
    assert image.read(a,n)==bytes(g.mem_read(a,n));executed.add(a)
    if a in header['native_cpu_callees']and a!=spec['entry']:
     sp=g.reg_read(REGS[4]);count=3 if a==0x21f260 else 1
     calls.append(dict(target=a,gp=[g.reg_read(reg)for reg in REGS],**{'return':u(sp)},args=[u(sp+4+k*4)for k in range(count)]));call_counts[f'{a:08x}']+=1
   def write(_g,kind,a,n,v,_):
    try:before=u(a,n)
    except UcError:return
    if 0x2000000<=a<0x2010000:stack_stores.append([a,n,before,v&((1<<(8*n))-1)]);return
    stores.append([g.reg_read(UC_X86_REG_EIP),a,n,before,v&((1<<(8*n))-1)])
   def invalid(_g,kind,a,n,v,_):bad.append([a,n]);return False
   g.hook_add(UC_HOOK_CODE,instruction);g.hook_add(UC_HOOK_MEM_WRITE,write);g.hook_add(UC_HOOK_MEM_INVALID,invalid)
   try:g.emu_start(spec['entry'],STOP,count=10000)
   except UcError:
    if not bad:raise
   if not bad:assert g.reg_read(UC_X86_REG_EIP)==STOP
   assert [g.reg_read(UC_X86_REG_XMM0+n)for n in range(8)]==xmm
   assert [g.reg_read(reg)for reg in [UC_X86_REG_FPCW,UC_X86_REG_MXCSR,UC_X86_REG_FPSW]]==fp
   emit(spec|dict(case=ordinal,fields=fields,gp=gp,out_gp=None if bad else[g.reg_read(reg)for reg in REGS],xmm=[[v>>(k*32)&0xffffffff for k in range(4)]for v in xmm],fp=fp,stores=stores,source_stack_stores=stack_stores,calls=calls,fault=bad[0]if bad else None))
   counts[f'{spec["entry"]:08x}']+=1;sites.update(executed);returns+=not bad;faults+=bool(bad);stores_count+=len(stores);stack_count+=len(stack_stores)
 report=dict(passed=True,scope=__doc__,original_xbe_sha256=NEW_PIN,cases=len(specs),entries=dict(counts),native_cpu_callees=dict(call_counts),returns=returns,faults=faults,ordered_stores=stores_count,source_stack_stores=stack_count,instruction_sites=len(sites),executed_instructions_lower_bound=sum(sites.values()),fixture=str(fixture),fixture_bytes=fixture.stat().st_size,fixture_sha256=digest.hexdigest(),source_instruction_patches=[],platform_results_supplied=[],source_cache_or_voice_admission=False)
 (args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report),flush=True)
if __name__=='__main__':main()
