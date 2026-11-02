#!/usr/bin/env python3
"""Observe unchanged TU1.5 general/requested-datum allocation and record initialization.

Pool/record/bitmap/stack bytes are declared raw ABI inputs, including aliases,
malformed records and both direction flags. No allocation result, resource read,
readiness, voice or platform result is supplied. All CPU instructions execute
unchanged, including16E770's complete16E860 child and general16E6A0 allocation.
"""
import argparse,collections,hashlib,json,os,random
from pathlib import Path
from oracle_sound_stream_packets_tu15 import Xbe,NEW_DEFAULT,NEW_PIN,STACK,STOP,REGS
from unicorn import Uc,UcError,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE,UC_HOOK_MEM_INVALID
from unicorn.x86_const import *
POOL,BITS,ROWS=0x4000000,0x4100000,0x4200000
SEMANTIC={0x16e70e,0x16e71c,0x16e71f,0x16e724,0x16e733,0x16e73a,0x16e740,0x16e743,0x16e74e,0x16e7b2,0x16e7bd,0x16e7c6,0x16e7d1,0x16e86e,0x16e875,0x16e87b,0x16e87e,0x16e88b}
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);args=p.parse_args()
 owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True);args.output=args.output.resolve();assert args.output.is_relative_to(owner);args.output.mkdir(parents=True,exist_ok=False)
 image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN
 specs=[];strides=[0,1,2,3,4,5,7,8,15,16,17,23,24,31,63];salts=[0,1,0x7fff,0x8000,0xfffe,0xffff]
 for df in [False,True]:
  for index in [0,1,15,31,32,63,65535]:
   for stride in strides:
    for salt in salts:specs.append(dict(entry=0x16e770,index=index,stride=stride,salt=salt,df=df,capacity=65536 if index==65535 else 64))
  for capacity in [0,1,2,31,32,64,65535,65536,0x80000000,0xffffffff]:
   for index in [0,1,31,32,63,64,65535]:specs.append(dict(entry=0x16e770,index=index,stride=16,salt=0xffff,df=df,capacity=capacity))
  for occupied in [1,0x7fff,0x8000,0xffff]:
   for stride in [0,1,2,3,16,17,24]:specs.append(dict(entry=0x16e770,index=1,stride=stride,salt=0x8000,df=df,occupied=occupied))
  for stride in strides:
   for next_salt in [0,1,0x7fff,0x8000,0xfffd,0xfffe,0xffff]:specs.append(dict(entry=0x16e860,index=0,stride=stride,df=df,next_salt=next_salt))
  for alias in ['bitmap-is-stride','record-is-count','record-is-stride']:
   for entry in [0x16e770,0x16e860]:
    for stride in [3,16,17]:
     for salt in [0,0x8000,0xffff]:specs.append(dict(entry=entry,index=0,stride=stride,salt=salt,df=df,alias=alias))
  for entry in [0x16e770,0x16e860]:
   for fault in ['pool','rows','bitmap']:
    for stride in [0,1,4,17]:specs.append(dict(entry=entry,index=0,stride=stride,salt=0x8000,df=df,fault=fault))
 for df in [False,True]:
  for cursor in [0,1,15,31,32,63,64,0xffffffff,0x80000000]:
   for high in [0,1,32,64]:
    for stride in [0,1,3,4,16,17,24]:
     for next_salt in [0,0x7fff,0x8000,0xfffe,0xffff]:specs.append(dict(entry=0x16e6a0,index=0,stride=stride,df=df,cursor=cursor,high=high,next_salt=next_salt))
  for alias in ['bitmap-is-stride','record-is-count','record-is-stride']:
   for stride in [0,3,16,17]:
    for next_salt in [0,0xffff]:specs.append(dict(entry=0x16e6a0,index=0,stride=stride,df=df,cursor=0,high=0,next_salt=next_salt,alias=alias))
 counts=collections.Counter();sites=collections.Counter();returns=faults=stores_count=stack_count=backwards=0;digest=hashlib.sha256()
 header=dict(original_xbe_sha256=NEW_PIN,raw_abi_only=True,platform_results_supplied=[],source_stack_stores_observed=True,required_entries=[0x16e6a0,0x16e770,0x16e860],minimum_cases=4000,caller_direction_preserved=True,
  regions=[[0x2000000,0x10000],[POOL,4096],[BITS,0x10000],[ROWS,0x10000]])
 fixture=args.output/'cases.jsonl'
 with fixture.open('wb')as out:
  def emit(row):
   raw=(json.dumps(row,separators=(',',':'))+'\n').encode();out.write(raw);digest.update(raw)
  emit(header)
  for ordinal,spec in enumerate(specs):
   rng=random.Random(0x16e770+ordinal);fields=[]
   def w(at,n,value):fields.append([at,n,value&((1<<(8*n))-1)])
   for at in range(STACK-64,STACK+20,4):w(at,4,rng.getrandbits(32))
   index=spec.get('index',0);stride=spec['stride'];salt=spec.get('salt',0x8000);row_base=ROWS+0x1000;row=(row_base+index*stride)&0xffffffff;bitmap=BITS
   alias=spec.get('alias')
   if alias=='bitmap-is-stride':bitmap=POOL+0x24
   if alias=='record-is-count':row_base=row=POOL+0x3c
   if alias=='record-is-stride':row_base=row=POOL+0x24
   high=spec.get('high',[0,index,index+1,64,0xffffffff,0x80000000][ordinal%6])
   for at,v in [(POOL+0x20,spec.get('capacity',64)),(POOL+0x24,stride),(POOL+0x34,spec.get('cursor',rng.getrandbits(32))),(POOL+0x38,high),(POOL+0x3c,0),(POOL+0x44,row_base),(POOL+0x48,bitmap)]:w(at,4,v)
   w(POOL+0x40,2,spec.get('next_salt',[0,1,0x7fff,0x8000,0xfffd,0xfffe,0xffff][ordinal%7]))
   if ROWS+128<=row and row+128<=ROWS+0x10000:
    for offset in range(-80,84,4):w(row+offset,4,rng.getrandbits(32))
    w(row,2,spec.get('occupied',0))
   if spec['entry']==0x16e6a0 and not alias:
    for candidate in set([spec.get('cursor',0),high]):
     address=row_base+candidate*stride
     if 0<=candidate<=64 and ROWS+128<=address and address+128<=ROWS+0x10000:
      for offset in range(-80,84,4):w(address+offset,4,rng.getrandbits(32))
   if alias=='record-is-count':w(POOL+0x3c,4,0)
   if alias=='record-is-stride'and spec['entry']==0x16e770:
    # A specific allocation sees this occupied WORD and must refuse; the
    # standalone initializer still exercises clearing its stride metadata.
    assert stride!=0
   if bitmap==BITS:
    if spec['entry']==0x16e6a0:
     for n in range(3):w(BITS+n*4,4,[0,0xffffffff,0x55555555,0xaaaaaaaa][ordinal%4])
    else:w(BITS+(index>>5)*4,4,rng.getrandbits(32))
   w(STACK,4,STOP);gp=[rng.getrandbits(32)for _ in REGS];gp[4]=STACK;gp[0]=(salt<<16)|index;gp[2]=POOL;gp[6]=row
   fault=spec.get('fault')
   if fault=='pool':gp[2]=0
   elif fault=='rows':w(POOL+0x44,4,0);gp[6]=0
   elif fault=='bitmap':w(POOL+0x48,4,0)
   xmm=[rng.getrandbits(128)for _ in range(8)];fp=[[0x27f,0x37f,0x77f,0xb7f,0xf7f][ordinal%5],[0x1f80,0x3f80,0x5f80,0x7f80,0x9fc0][ordinal%5],((ordinal%8)<<11)|(ordinal&0x3f)|0x4500]
   flags=0x202|(0x400 if spec['df']else 0);g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x800000)
   for section in image.sections:g.mem_write(section.va,image.section_bytes(section))
   text=image.section('.text');g.mem_protect(text.va&~4095,((text.va%4096+text.vsize+4095)//4096)*4096,5)
   for at,n in header['regions']:g.mem_map(at,n);g.mem_write(at,bytes(n))
   g.mem_map(STOP,4096)
   for at,n,v in fields:g.mem_write(at,v.to_bytes(n,'little'))
   for reg,v in zip(REGS,gp):g.reg_write(reg,v)
   for n,v in enumerate(xmm):g.reg_write(UC_X86_REG_XMM0+n,v)
   for reg,v in zip([UC_X86_REG_FPCW,UC_X86_REG_MXCSR,UC_X86_REG_FPSW],fp):g.reg_write(reg,v)
   g.reg_write(UC_X86_REG_EFLAGS,flags);stores=[];stack_stores=[];bad=[];executed=set()
   def u(at,n=4):return int.from_bytes(g.mem_read(at,n),'little')
   def code(_g,at,n,_):assert image.read(at,n)==bytes(g.mem_read(at,n));executed.add(at)
   def write(_g,kind,at,n,v,_):
    try:before=u(at,n)
    except UcError:return
    pc=g.reg_read(UC_X86_REG_EIP);v&=(1<<(8*n))-1
    if pc in SEMANTIC:stores.append([pc,at,n,before,v])
    elif 0x2000000<=at<0x2010000:stack_stores.append([at,n,before,v])
    else:raise AssertionError(('Unexpected original store',hex(pc),hex(at),n))
   def invalid(_g,kind,at,n,v,_):bad.append([at,n]);return False
   g.hook_add(UC_HOOK_CODE,code);g.hook_add(UC_HOOK_MEM_WRITE,write);g.hook_add(UC_HOOK_MEM_INVALID,invalid)
   try:g.emu_start(spec['entry'],STOP,count=10000)
   except UcError:
    if not bad:raise
   if not bad:assert g.reg_read(UC_X86_REG_EIP)==STOP
   assert (g.reg_read(UC_X86_REG_EFLAGS)&0x400)==(flags&0x400)
   assert [g.reg_read(UC_X86_REG_XMM0+n)for n in range(8)]==xmm
   assert [g.reg_read(reg)for reg in [UC_X86_REG_FPCW,UC_X86_REG_MXCSR,UC_X86_REG_FPSW]]==fp
   emit(spec|dict(case=ordinal,fields=fields,gp=gp,eflags=flags,out_gp=None if bad else[g.reg_read(reg)for reg in REGS],xmm=[[v>>(k*32)&0xffffffff for k in range(4)]for v in xmm],fp=fp,stores=stores,source_stack_stores=stack_stores,fault=bad[0]if bad else None))
   counts[f'{spec["entry"]:08x}']+=1;sites.update(executed);returns+=not bad;faults+=bool(bad);stores_count+=len(stores);stack_count+=len(stack_stores);backwards+=spec['df']
 report=dict(passed=True,scope=__doc__,original_xbe_sha256=NEW_PIN,cases=len(specs),entries=dict(counts),returns=returns,faults=faults,backwards_cases=backwards,ordered_stores=stores_count,source_stack_stores=stack_count,instruction_sites=len(sites),executed_instructions_lower_bound=sum(sites.values()),fixture=str(fixture),fixture_bytes=fixture.stat().st_size,fixture_sha256=digest.hexdigest(),source_instruction_patches=[],supplied_callee_or_platform_results=[],source_cache_or_voice_admission=False)
 (args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report),flush=True)
if __name__=='__main__':main()
