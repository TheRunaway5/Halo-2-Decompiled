#!/usr/bin/env python3
"""Untouched player 13887->214F0, including complete 245C0/24EE0 context reset.

No instruction patches or function hooks. Stop at the actual GPU blur submission
entry; its float arguments are independent expected values. Controlled factor
cases start at 138EE, so they do not claim a player-view context producer.
"""
from pathlib import Path
import argparse,collections,hashlib,json,random,struct,subprocess,sys
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
PIN='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
BASE,CTX,TIMELINE,STACK=0x2100000,0x2100000,0x2101000,0x21fff00

def pack(v):return struct.pack('<'+'I'*len(v),*(x&0xffffffff for x in v))
def words(v):return list(struct.unpack('<'+'I'*(len(v)//4),v))
def bits(v):return struct.unpack('<I',struct.pack('<f',v))[0]
def sha(v):return hashlib.sha256(v).hexdigest()
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--native',type=Path);a=ap.parse_args()
 x=Xbe(ROOT/'assets/disc/default.xbe');assert sha(x.data)==PIN
 u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x10000,0x600000)
 for s in x.sections:u.mem_write(s.va,x.section_bytes(s))
 u.mem_map(BASE,0x100000)
 seen={};calls=collections.Counter()
 def observe(g,pc,size,_):
  assert bytes(g.mem_read(pc,size))==x.read(pc,size),(hex(pc),'original bytes changed')
  seen[pc]=size
  if pc in [0x245c0,0x24ee0,0x137a0]:calls[f'{pc:08X}']+=1
 u.hook_add(UC_HOOK_CODE,observe)
 # 24EE0's existing LUT resource descriptor; writes are contained in mapped
 # original globals. Mode0 performs no GPU/async submission or image read.
 u.mem_write(0x4b54c8,pack([1]));u.mem_write(0x4b54d5,b'\0')
 u.mem_write(0x50934c,pack([TIMELINE]));u.mem_write(0x4858a0,bytes(8));u.mem_write(0x485898,pack([0]))
 def original(r):
  kind,index,factor,amount,brightness,*sky=r
  u.mem_write(CTX,pack([0x55555555]*9));u.mem_write(CTX+0x10,pack([factor]))
  u.mem_write(0x485890,pack([0 if kind==0 else CTX]));u.mem_write(0x485600,struct.pack('<h',index if index<32768 else index-65536))
  timeline=bytearray(52)
  for offset in [12,16,36,40]:struct.pack_into('<I',timeline,offset,amount)
  for offset in [20,24,44,48]:struct.pack_into('<I',timeline,offset,brightness)
  u.mem_write(TIMELINE,bytes(timeline));u.mem_write(0x4b9fd0,pack(sky))
  u.mem_write(STACK-128,bytes(256));u.mem_write(STACK+0x6c,pack([1]));u.reg_write(UC_X86_REG_ESP,STACK)
  for reg in [UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP]:u.reg_write(reg,0)
  u.reg_write(UC_X86_REG_EFLAGS,2);u.reg_write(UC_X86_REG_FPCW,0x027f);u.reg_write(UC_X86_REG_FPSW,0);u.reg_write(UC_X86_REG_FPTAG,0xffff);u.reg_write(UC_X86_REG_MXCSR,0x1f80)
  start=0x13887
  if kind==2:
   start=0x138ee;u.reg_write(UC_X86_REG_ESI,CTX)
   for reg in [UC_X86_REG_XMM4,UC_X86_REG_XMM5]:u.reg_write(reg,bits(.9))
   u.reg_write(UC_X86_REG_XMM3,0);u.mem_write(STACK+12,pack([bits(.9)]));u.mem_write(STACK+16,pack([brightness]));u.mem_write(STACK+0x6c,pack([amount]))
  u.emu_start(start,0x214f0,count=2000000)
  assert u.reg_read(UC_X86_REG_EIP)==0x214f0
  sp=u.reg_read(UC_X86_REG_ESP);_,threshold,strength=words(u.mem_read(sp+4,12))
  context=words(u.mem_read(CTX,36))if kind==1 else []
  return [*context,threshold,strength,sky[3]]
 rows=[];labels=[]
 def add(kind,index,factor,amount,brightness,sky,label):
  rows.append([kind,index&65535,bits(factor),bits(amount),bits(brightness),*map(bits,sky)]);labels.append(label)
 for kind in [0,1]:
  for index in [-32768,-1,0,1,2,3,32767]:
   add(kind,index,.123,.5,.5,[0,0,0,1],'null context'if kind==0 else 'normal player context: signed view index')
 rng=random.Random(0x13870245c0)
 for i in range(512):
  kind=i%3;amount=rng.choice([.001,.1,.5,1,2]);brightness=rng.choice([-2,-1,-.5,0,.5,1,2])
  factor=rng.choice([0,2**-25,2**-24,.125,.4,.75,1,1.5])
  sky=[rng.choice([-.25,0,.1,.25,.5,1,1.5]),rng.uniform(0,1),rng.uniform(0,1),rng.choice([.8,1,1.2])]
  add(kind,i%4,factor,amount,brightness,sky,'controlled factor arithmetic'if kind==2 else 'normal/null with explicit sky and cinematic values')
 out=[original(r)for r in rows]
 fixture=ROOT/'tests/fixtures/retail_source_bloom_context.tsv';fixture.write_text('# kind/index/raw-factor/cinematic-amount/brightness/sky4 | optional player context9 + actual threshold/strength/gamma\n'+''.join(' '.join(f'{v:08X}'for v in r)+' | '+' '.join(f'{v:08X}'for v in o)+'\n'for r,o in zip(rows,out)))
 retained=[]
 for filename in ['ivory-live-view.bin','zanzibar-live-view.bin']:
  p=ROOT/'analysis/multiplayer-shading/original-xemu'/filename;d=p.read_bytes();assert len(d)==0x118
  context=words(d[0xf4:])
  # literal index0 context; do not relabel these postcapture RAM words same-frame.
  assert context==[0,0x3f400000,0x3ecccccd,0,0x3f400000,0x3ecccccd,0x51ecf0,0,1]
  retained.append({'path':str(p.relative_to(ROOT)),'size':len(d),'sha256':sha(d),'context_words':context,'boundary':'Immutable paused-after-capture view bank; not a claim of same-frame RAM with original20 RDC.'})
 pixel=ROOT/'analysis/multiplayer-shading/appearance-residual-replay/source/8941-fragment-block0.bin';pixeldata=pixel.read_bytes()
 rgba=struct.unpack_from('<4f',pixeldata,544+6*16)
 assert tuple(round(c*255)for c in rgba)==(172,172,172,32)
 report={'xbe_sha256':PIN,'cases':len(rows),'exact_output_words':sum(map(len,out)),'calls':calls,'original_instruction_addresses':len(seen),
  'producer_path':'normal view 2B790 +F4 -> 2BA10 +290 -> 132D0 485890 -> 13870 modeEBX0 -> 245C0/24EE0',
  'execution_boundary':'13887 starts after shader context reset16B10; stop214F0 observes actual blur submission arguments. Complete245C0/24EE0 and their math callees execute without instruction patches or function hooks. Controlled factor rows start138EE and do not represent normal-player factor production.',
  'native_scope':'normal-player context only; null and other callers remain explicit. Cinematic/sky inputs are explicit fixtures. No async exposure/history pipeline port claimed.',
  'functions':{f'{va:08X}':{'size':size,'sha256':sha(x.read(va,size))}for va,size in [(0x245c0,209),(0x13870,793),(0x137a0,202),(0x2b790,497),(0x132d0,384),(0x2ba10,698),(0x24ee0,707)]},
  'actual_original20_gpu_threshold':{'event':8941,'path':str(pixel.relative_to(ROOT)),'sha256':sha(pixeldata),'block_size':len(pixeldata),'C6_offset':640,'C6_words':words(pixeldata[640:656]),'C6_bytes':[172,172,172,32],'boundary':'Independent original20 RenderDoc/xemu GPU execution, distinct from paused-after-capture inline view RAM.'},
  'retained_contexts':retained,'fixture_sha256':sha(fixture.read_bytes()),'labels':labels}
 if a.native:
  result=subprocess.run([str(a.native),'--probe',str(fixture)],capture_output=True,text=True,check=True);actual=[list(map(lambda x:int(x,16),s.split()))for s in result.stdout.splitlines()]
  assert actual==out;report['native_exact']=True;report['native_sha256']=sha(a.native.read_bytes())
 output=ROOT/'analysis/multiplayer-shading/bloom-context-original.json';output.write_text(json.dumps(report,indent=2)+'\n')
 fixture.with_suffix('.provenance.json').write_text(json.dumps({k:v for k,v in report.items()if k!='labels'},indent=2)+'\n')
 print(f'{len(rows)} cases, {sum(map(len,out))} exact words, complete245C0 calls{calls["000245C0"]}, complete24EE0 calls{calls["00024EE0"]}')
if __name__=='__main__':main()
