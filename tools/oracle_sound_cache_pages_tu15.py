#!/usr/bin/env python3
"""Observe unchanged TU1.5 page allocation, eviction and complete datum deletion.

Only raw ABI/pool/list/cache/stack inputs are declared. All CPU callees and
the actual sound busy/delete callbacks execute unchanged. No page handle,
busy answer, eviction, loaded byte, resource read or platform result is supplied.
The stack probe executes as called by13FE50; it is not a global CRT proof.
"""
import argparse,collections,hashlib,json,os,random
from pathlib import Path
from oracle_sound_stream_packets_tu15 import Xbe,NEW_DEFAULT,NEW_PIN,STACK,STOP,REGS
from unicorn import Uc,UcError,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE,UC_HOOK_MEM_INVALID
from unicorn.x86_const import *
from capstone import Cs,CS_ARCH_X86,CS_MODE_32

ALLOC,POOL,BITS,ROWS,CACHE,CBITS,CROWS,CHUNKS,WINDOWS=[0x4000000+n*0x100000 for n in range(9)]
ENTRIES=[0x13fe00,0x13fe50,0x140310,0x16e8a0,0x21f3f0,0x21f420]
CALLBACKS=[0x21f3b0,0x21f3f0]

def specifications():
 specs=[]
 for tick in [0,150,300,1000,0xffffffff]:
  for first in [0,149,150,151,299,300,301,1000,0xffffffff]:
   for second in [0,149,150,151,299,300,301,1000,0xffffffff]:
    specs.append(dict(entry=0x13fe00,tick=tick,first=first,second=second))
 for df in [False,True]:
  for policy in [0,1,2,0xffffffff]:
   for count in [0,1,2,4,7,8]:
    for layout in ['packed','gapped','tail','full']:
     for threshold in [0,1,8,-1]:
      specs.append(dict(entry=0x13fe50,df=df,policy=policy,count=count,layout=layout,threshold=threshold))
  for index in [0,1,2,7,15,31,32,63]:
   for stride in [0,1,2,3,4,7,16,17,24,31]:
    for debug in [False,True]:specs.append(dict(entry=0x16e8a0,df=df,index=index,stride=stride,debug=debug))
  for count in [1,2,4,7]:
   for index in range(count):
    for callback in [False,True]:
     specs.append(dict(entry=0x140310,df=df,count=count,index=index,callback=callback))
  for index in [0,1,7,31,32,63]:
   for debug in [False,True]:specs.append(dict(entry=0x21f3f0,df=df,index=index,debug=debug))
 # These drive the actual 256-slot rolling-window wrap and a full pool's
 # oldest-record eviction when a free physical gap still needs metadata.
 for policy in [0,1,2]:
  for count in [255,256,257,300]:
   specs.append(dict(entry=0x13fe50,policy=policy,count=count,layout='packed',need=count+1,capacity=count+1,total=count+8,callbacks=False,tick=10000))
  for threshold in [-10,0,1,8,9,100,0x7fffffff]:
   for need in [0,1,9,32,0xffffffff,0x80000000]:
    specs.append(dict(entry=0x13fe50,policy=policy,count=4,layout='gapped',need=need,threshold=threshold))
 for shift in [0,1,4,9,12,31,32,0xffffffff]:
  for size in [0,1,511,512,513,0x3fffffff,0x7fffffff,0x80000000,0xffffffff]:
   specs.append(dict(entry=0x13fe50,count=4,layout='gapped',shift=shift,size=size))
 for entry in ENTRIES:
  for fault in ['pool','rows','bitmap','allocator','stack']:
   specs.append(dict(entry=entry,count=2,index=1,fault=fault))
 for alias in ['bitmap-is-stride','record-is-count','record-is-stride']:
  for df in [False,True]:
   for stride in [0,3,16,17]:specs.append(dict(entry=0x16e8a0,index=0,stride=stride,df=df,debug=True,alias=alias))
 rng=random.Random(0x140242)
 for _ in range(300):
  specs.append(dict(entry=0x13fe50,count=rng.randrange(1,12),layout=rng.choice(['packed','gapped','tail','full']),need=rng.randrange(1,24),
   threshold=rng.choice([-1,0,1,2,7,8,9]),policy=rng.choice([0,1,2,0xffffffff]),tick=rng.choice([0,50,1000,0xffffffff]),callbacks=bool(rng.randrange(2))))
 for df in [False,True]:
  for size in [0,1,35,36,511,512,513,4095,4096,4097,0x40000024,0x80000201,0xffffffff]:
   for count in [0,1,4,8]:
    for layout in ['packed','full']:specs.append(dict(entry=0x21f420,df=df,count=count,layout=layout,shift=9,size=size))
 for policy in [0,1,2]:
  for count in [1,2,4,8]:
   for need in [0,1,2,3,9]:
    for callbacks in [False,True]:
     for tick in [300,1000]:specs.append(dict(entry=0x13fe50,policy=policy,count=count,capacity=count,layout='packed',need=need,callbacks=callbacks,tick=tick))
 return specs

def request_specifications():
 specs=[]
 for df in [False,True]:
  for request in range(256):
   for present,loaded in [(False,False),(True,False),(True,True)]:
    for locks in [0,255]:specs.append(dict(entry=0x21f260,df=df,request=request,present=present,loaded=loaded,locks=locks,count=1))
  for entry in [0x128970,0x1293f0]:
   for present,loaded in [(False,False),(True,False),(True,True)]:
    for held in range(8):
     for selector in [-1,0,1]:specs.append(dict(entry=entry,df=df,present=present,loaded=loaded,held=held,selector=selector,count=1))
  for entry in [0x21f260,0x128970,0x1293f0]:
   for fault in ['pool','rows','bitmap','allocator','stack','tags','binding']:
    specs.append(dict(entry=entry,df=df,count=1,present=False,fault=fault,request=2))
 return specs

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--request-chain',action='store_true');args=p.parse_args()
 owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True);args.output=args.output.resolve()
 if not args.output.is_relative_to(owner):raise ValueError('Original fixture must stay in its disposable owner')
 args.output.mkdir(parents=True,exist_ok=False);image=Xbe(NEW_DEFAULT)
 assert hashlib.sha256(image.data).hexdigest()==NEW_PIN
 regions=[[0x50e000,4096],[0x2000000,0x10000]]+[[base,0x10000 if base in [ROWS,CROWS,CHUNKS]else 4096]for base in [ALLOC,POOL,BITS,ROWS,CACHE,CBITS,CROWS,CHUNKS,WINDOWS]]
 header=dict(original_xbe_sha256=NEW_PIN,raw_abi_only=True,platform_results_supplied=[],source_stack_stores_observed=True,
  required_entries=ENTRIES,minimum_cases=2500,caller_direction_preserved=True,native_cpu_callees=CALLBACKS,required_boundary_callees=[0x1a3b70],regions=regions)
 if args.request_chain:
  regions.extend([[0x4f0000,4096],[0x52d000,4096],[0x4900000,0x10000],[0x4a00000,0x10000],[0x4b00000,0x10000],[0x4c00000,0x10000]])
  header.update(required_entries=[0x21f260,0x128970,0x1293f0],native_cpu_callees=CALLBACKS+[0x21f420],required_boundary_callees=[0x1a3b70,0x2dbf74],minimum_cases=3000)
 counts=collections.Counter();sites=collections.Counter();branches=collections.Counter();total=collections.Counter();digest=hashlib.sha256()
 fixture=args.output/'cases.jsonl'
 with fixture.open('wb')as out:
  def emit(row):
   raw=(json.dumps(row,separators=(',',':'))+'\n').encode();out.write(raw);digest.update(raw)
  emit(header)
  for ordinal,spec in enumerate(request_specifications()if args.request_chain else specifications()):
   rng=random.Random(0x13fe50+ordinal);fields=[]
   def w(at,n,value):fields.append([at,n,value&((1<<(8*n))-1)])
   count=spec.get('count',4);entry=spec['entry'];index=spec.get('index',ordinal%max(1,count));stride=spec.get('stride',24)
   capacity=spec.get('capacity',max(count,8) if spec.get('layout')=='full'else max(count+1,8));tick=spec.get('tick',1000)
   salts=[0x8000+(n%32767)for n in range(max(64,count))];handle=(salts[index]<<16)|index
   rows=ROWS+0x1000;crows=CROWS+0x1000;bitmap=BITS
   alias=spec.get('alias')
   if alias=='bitmap-is-stride':bitmap=POOL+0x24
   if alias=='record-is-count':rows=POOL+0x3c
   if alias=='record-is-stride':rows=POOL+0x24
   for at,v in [(0x50ebdc,CACHE),(0x50ebe0,0x9000000),(0x50ebe4,ALLOC),(POOL+0x20,capacity),(POOL+0x24,stride),(POOL+0x34,0),(POOL+0x38,max(count,index+1)),(POOL+0x3c,count),(POOL+0x44,rows),(POOL+0x48,bitmap),
                (CACHE+0x20,max(count,index+1,64)),(CACHE+0x24,16),(CACHE+0x34,0),(CACHE+0x38,max(count,index+1)),(CACHE+0x3c,max(count,index+1)),(CACHE+0x44,crows),(CACHE+0x48,CBITS)]:w(at,4,v)
   debug=spec.get('debug',not spec.get('df',False)and ordinal%3==0)
   w(POOL+0x40,2,[0x8000,0xfffd,0xfffe,0xffff][ordinal%4]);w(POOL+0x2a,1,8 if debug else 0)
   w(CACHE+0x2a,1,8 if debug else 0);w(CACHE+0x40,2,0x8000)
   nodes=range(count)if entry not in [0x16e8a0,0x21f3f0]else range(index+1)
   bits={};cbits={}
   for n in nodes:
    h=(salts[n]<<16)|n;record=rows+n*stride;cached=crows+n*16;chunk=CHUNKS+0x1000+n*12
    # Datum-deletion cases include debug poisoning over populated surrounding
    # bytes, with odd strides and the actual backwards string direction.
    if entry==0x16e8a0 and not alias:
     for k in range(0,max(stride,4),4):w(record+k,4,rng.getrandbits(32))
    w(record,2,salts[n]);w(cached,2,salts[n]);w(cached+2,1,0 if ordinal%17==0 else 1)
    w(cached+4,1,1 if ordinal%11==0 else 0);w(cached+5,1,255 if ordinal%13==0 else 0)
    w(cached+12,4,chunk);w(chunk+8,4,h)
    if entry!=0x16e8a0:
     start=n*3 if spec.get('layout')=='gapped'else n+(4 if spec.get('layout')=='tail'else 0)
     for at,v in [(record+4,1),(record+8,start),(record+12,((salts[n+1]<<16)|(n+1))if n+1<count else 0xffffffff),(record+16,((salts[n-1]<<16)|(n-1))if n else 0xffffffff),(record+20,[10,50,200,990][n%4])]:w(at,4,v)
    bits[n>>5]=bits.get(n>>5,0)|(1<<(n&31));cbits[n>>5]=cbits.get(n>>5,0)|(1<<(n&31))
   if bitmap==BITS:
    for n,v in bits.items():w(BITS+n*4,4,v)
   for n,v in cbits.items():w(CBITS+n*4,4,v)
   if alias=='record-is-count':w(POOL+0x3c,4,0)
   if alias=='record-is-stride':w(POOL+0x24,4,stride)
   callbacks=spec.get('callbacks',True);delete=spec.get('callback',True)
   for at,v in [(ALLOC+0x20,0x21f3f0 if callbacks and delete else 0),(ALLOC+0x24,0x21f3b0 if callbacks else 0),(ALLOC+0x2c,spec.get('policy',ordinal%3)),(ALLOC+0x30,spec.get('total',max(16,count*3+8)if spec.get('layout')=='gapped'else max(16,count+8))),(ALLOC+0x34,spec.get('shift',0)),(ALLOC+0x38,tick),
                (ALLOC+0x3c,(salts[0]<<16)if count else 0xffffffff),(ALLOC+0x40,((salts[count-1]<<16)|(count-1))if count else 0xffffffff),(ALLOC+0x64,POOL)]:w(at,4,v)
   # The source's clamped index8 aliases +64, the real pool pointer. It is
   # not an independently initialized ninth limit DWORD.
   for n in range(8):w(ALLOC+0x44+n*4,4,[0x7fffffff,0x7fffffff,0x7fffffff,0x7fffffff,0,1,4,8,32][(ordinal+n)%9]if ordinal%19==0 else 0x7fffffff)
   for at in range(STACK-64,STACK+24,4):w(at,4,rng.getrandbits(32))
   w(STACK,4,STOP);w(STACK+4,4,ALLOC);w(STACK+8,4,spec.get('size',spec.get('need',1+ordinal%12)));w(STACK+12,4,spec.get('threshold',0))
   gp=[rng.getrandbits(32)for _ in REGS];gp[4]=STACK
   if entry==0x13fe00:
    gp[1]=ALLOC;gp[6]=WINDOWS+0x100;gp[7]=WINDOWS+0x200
    w(WINDOWS+0x104,4,spec.get('first',0));w(WINDOWS+0x204,4,spec.get('second',0))
    w(WINDOWS+0x10c,4,[0,1,0x7fffffff,0x80000000,0xffffffff][ordinal%5]);w(WINDOWS+0x20c,4,[0,1,0x7fffffff,0x80000000,0xffffffff][(ordinal//5)%5])
   elif entry==0x140310:gp[7]=ALLOC;gp[3]=handle
   elif entry==0x16e8a0:gp[2]=POOL;gp[0]=handle
   elif entry==0x21f3f0:w(STACK+4,4,handle)
   elif entry==0x21f420:
    gp[3]=CHUNKS+0x9000;w(gp[3],4,rng.getrandbits(32));w(gp[3]+4,4,spec.get('size',513));w(gp[3]+8,4,0xffffffff);w(STACK+4,4,0xabcd1234)
   if args.request_chain:
    w(ALLOC+0x34,4,9)
    binding=0x4900000;tags=0x4a00000;definition=0x4a08000;gestalt=0x4b00000;ranges=0x4b08000;permutations=0x4c00000
    chunk=CHUNKS+0x1000;datum=0xabcd0000;selector=spec.get('selector',0)
    for at,v in [(0x4f0414,tags),(tags+8,definition),(0x52d664,gestalt),(gestalt+0x24,ranges),(gestalt+0x2c,permutations),(gestalt+0x44,chunk),
                 (binding+12,datum),(chunk,0x80001000),(chunk+4,513),(chunk+8,0x80000000 if spec.get('present')else 0xffffffff),(CROWS+0x1000+12,chunk)]:w(at,4,v)
    w(definition+8,2,0);w(ranges+8,2,0);w(permutations+12,2,0)
    w(binding+4,1,spec.get('held',0));w(binding+0x9c,1,selector);w(binding+0x9d,1,0);w(binding+0x9e,2,0)
    w(CROWS+0x1000+2,1,int(spec.get('loaded',False)));w(CROWS+0x1000+4,1,spec.get('locks',0))
    if entry==0x21f260:
     w(STACK+4,4,datum);w(STACK+8,4,chunk);w(STACK+12,4,spec['request'])
    elif entry==0x128970:w(STACK+4,4,binding)
    else:gp[0]=binding
    if not spec.get('present'):
     for pool in [POOL,CACHE]:
      w(pool+0x34,4,0);w(pool+0x38,4,0);w(pool+0x3c,4,0)
     w(ROWS+0x1000,4,0);w(CROWS+0x1000,4,0);w(BITS,4,0);w(CBITS,4,0);w(ALLOC+0x3c,4,0xffffffff);w(ALLOC+0x40,4,0xffffffff)
   fault=spec.get('fault')
   if fault=='pool':w(ALLOC+0x64,4,0);gp[2]=0
   elif fault=='rows':w(POOL+0x44,4,0);w(CACHE+0x44,4,0)
   elif fault=='bitmap':w(POOL+0x48,4,0);w(CACHE+0x48,4,0)
   elif fault=='allocator':w(STACK+4,4,0);gp[1]=0;gp[7]=0
   elif fault=='stack':gp[4]=0x2000000
   elif fault=='tags':w(0x4f0414,4,0)
   elif fault=='binding':w(STACK+4,4,0);gp[0]=0
   xmm=[rng.getrandbits(128)for _ in range(8)];fp=[[0x27f,0x37f,0x77f,0xb7f,0xf7f][ordinal%5],[0x1f80,0x3f80,0x5f80,0x7f80,0x9fc0][ordinal%5],((ordinal%8)<<11)|(ordinal&0x3f)|0x4500]
   flags=0x202|(0x400 if spec.get('df',False)else 0);g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x800000)
   for section in image.sections:g.mem_write(section.va,image.section_bytes(section))
   text=image.section('.text');g.mem_protect(text.va&~4095,((text.va%4096+text.vsize+4095)//4096)*4096,5)
   for at,n in regions:
    if at>=0x1000000:g.mem_map(at,n)
    g.mem_write(at,bytes(n))
   g.mem_map(STOP,4096)
   for at,n,v in fields:g.mem_write(at,v.to_bytes(n,'little'))
   for reg,v in zip(REGS,gp):g.reg_write(reg,v)
   for n,v in enumerate(xmm):g.reg_write(UC_X86_REG_XMM0+n,v)
   for reg,v in zip([UC_X86_REG_FPCW,UC_X86_REG_MXCSR,UC_X86_REG_FPSW],fp):g.reg_write(reg,v)
   g.reg_write(UC_X86_REG_EFLAGS,flags);stores=[];stack_stores=[];calls=[];bad=[];executed=set();stopped=[]
   def u(at,n=4):return int.from_bytes(g.mem_read(at,n),'little')
   def code(_g,at,n,_):
    assert image.read(at,n)==bytes(g.mem_read(at,n))
    stops=header['required_boundary_callees']
    if at not in stops:executed.add(at)
    if at in header['native_cpu_callees']+stops and at!=entry:
     sp=g.reg_read(UC_X86_REG_ESP);count=6 if at==0x1a3b70 else 0 if at==0x2dbf74 else 1
     calls.append(dict(target=at,gp=[g.reg_read(reg)for reg in REGS],return_=u(sp),args=[u(sp+4+n*4)for n in range(count)]));calls[-1]['return']=calls[-1].pop('return_')
     if at in stops:stopped.append(at);g.emu_stop()
   def write(_g,kind,at,n,v,_):
    try:before=u(at,n)
    except UcError:return
    pc=g.reg_read(UC_X86_REG_EIP);v&=(1<<(8*n))-1
    if 0x2000000<=at<0x2010000:stack_stores.append([at,n,before,v])
    else:stores.append([pc,at,n,before,v])
   def invalid(_g,kind,at,n,v,_):bad.append([at,n]);return False
   g.hook_add(UC_HOOK_CODE,code);g.hook_add(UC_HOOK_MEM_WRITE,write);g.hook_add(UC_HOOK_MEM_INVALID,invalid)
   try:g.emu_start(entry,STOP,count=4000000)
   except UcError:
    if not bad:raise
   if not bad:assert g.reg_read(UC_X86_REG_EIP)==(stopped[0]if stopped else STOP),(ordinal,spec,hex(g.reg_read(UC_X86_REG_EIP)))
   assert (g.reg_read(UC_X86_REG_EFLAGS)&0x400)==(flags&0x400)
   assert [g.reg_read(UC_X86_REG_XMM0+n)for n in range(8)]==xmm
   assert [g.reg_read(reg)for reg in [UC_X86_REG_FPCW,UC_X86_REG_MXCSR,UC_X86_REG_FPSW]]==fp
   emit(spec|dict(case=ordinal,fields=fields,gp=gp,eflags=flags,out_gp=None if bad else[g.reg_read(reg)for reg in REGS],xmm=[[v>>(k*32)&0xffffffff for k in range(4)]for v in xmm],fp=fp,stores=stores,source_stack_stores=stack_stores,calls=calls,fault=bad[0]if bad else None,stopped_at_required_owner=bool(stopped)))
   counts[f'{entry:08x}']+=1;sites.update(executed);total.update(dict(cases=1,returns=not bad and not stopped,faults=bool(bad),backwards=bool(flags&0x400),stores=len(stores),stack_stores=len(stack_stores),callbacks=len(calls)-len(stopped),read_boundaries=len(stopped)))
   if ordinal%512==511:print(json.dumps(dict(completed=ordinal+1)),flush=True)
 md=Cs(CS_ARCH_X86,CS_MODE_32);functions={}
 for start,end in [(0x128970,0x1289f6),(0x1293f0,0x129465),(0x21f260,0x21f35f)]if args.request_chain else [(0x13fe00,0x13fe4d),(0x13fe50,0x14030d),(0x140310,0x14038c),(0x16e6a0,0x16e761),(0x16e770,0x16e7ea),(0x16e860,0x16e892),(0x16e8a0,0x16e932),(0x21f3b0,0x21f3e4),(0x21f3f0,0x21f41d),(0x21f420,0x21f4c8),(0x32a660,0x32a6a1)]:
  raw=image.read(start,end-start);instructions=list(md.disasm(raw,start));missing=[f'{q.address:08x}'for q in instructions if q.address not in sites]
  functions[f'{start:08x}']=dict(bytes=len(raw),sha256=hashlib.sha256(raw).hexdigest(),static_sites=len(instructions),exercised=len(instructions)-len(missing),unexercised=missing)
 report=dict(passed=True,scope=__doc__,original_xbe_sha256=NEW_PIN,counts=dict(total),entries=dict(counts),instruction_sites=len(sites),executed_instructions_lower_bound=sum(sites.values()),
  static_functions=functions,executed_sites=[f'{at:08x}'for at in sorted(sites)],fixture=str(fixture),fixture_bytes=fixture.stat().st_size,fixture_sha256=digest.hexdigest(),source_instruction_patches=[],supplied_callee_or_platform_results=[],source_cache_or_voice_admission=False)
 (args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:v for k,v in report.items()if k not in ['executed_sites','static_functions']}),flush=True)

if __name__=='__main__':main()
