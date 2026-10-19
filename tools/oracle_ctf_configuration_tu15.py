#!/usr/bin/env python3
"""Offline original CPU proof for three owners and a genuine rotation tail."""
import hashlib,json,os,random,struct,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.dont_write_bytecode=True
sys.path[:0]=[str(ROOT/'tools'),str(ROOT/'tools/reference/halo2-decompiled/tools')]
from xbe import Xbe
from functions import _Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_READ,UC_HOOK_MEM_WRITE
import unicorn.x86_const as U
ENTRIES=(0x24b390,0x249440,0x24a6c0)
REUSED=(0x164930,)
K,G,E,SOURCE_SCNR,SP,STOP=0x4000000,0x4010000,0x4020000,0x4030000,0x3008000,0x5000000
REGS=[getattr(U,'UC_X86_REG_'+r)for r in('EAX','ECX','EDX','EBX','ESP','EBP','ESI','EDI')]
XMM=[getattr(U,'UC_X86_REG_XMM'+str(i))for i in range(8)]
def enc(v,n=4):return int(v&((1<<(8*n))-1)).to_bytes(n,'little')
def sha(b):return hashlib.sha256(b).hexdigest()
def main():
 S=Path(os.environ['HALO2_TEST_SCRATCH']);assert S.is_dir()
 image=Xbe(NEW_DEFAULT);assert sha(image.data)==NEW_PIN
 code=_Code(image,'.text');bodies={p:_trace(code,p,set(ENTRIES+REUSED))for p in ENTRIES+REUSED}
 owned={q for body in bodies.values()for q in body.code};ins={p:bytes(code.at(p).bytes)for p in owned}
 production={q for p in ENTRIES for q in bodies[p].code}
 assert len(production)==26 and len(owned)==50 and all(not x.calls for x in bodies.values())
 assert bodies[0x24a6c0].tail_jumps=={0x164930}
 blobs={};cases=[];seen=set();rng=random.Random(0x24b390)
 def blob(raw):h=sha(raw);blobs[h]=bytes(raw).hex();return h
 def add(name,entry,index=0,left=0xffff,right=0xffff,kind=1,populated=False,scenario_flags=0,team_mask=0,selection=0):
  bank=bytearray(random.Random(0x249440).randbytes(0x208));game=bytearray(random.Random(0x249460).randbytes(0x200))
  for off,v in((2,left),(0x14,right)):
   p=(off+index*2)&0xffffffff;assert p+2<=len(bank);bank[p:p+2]=enc(v,2)
  game[0x180:0x184]=enc(kind)
  stack=bytearray(random.Random(0x15a43b).randbytes(0x2000));stack[0x1000:0x1004]=enc(STOP);stack[0x1004:0x1008]=enc(index)
  banks={K:bank,G:game,0x52d710:enc(K),0x4f3218:enc(G),SP-0x1000:stack}
  if entry==0x24a6c0:
   engine=bytearray(0x100);engine[0xc:0xe]=enc(team_mask,2)
   scenario=bytearray(0x40);scenario[0x12]=scenario_flags
   bank[0x1f4:0x1f8]=enc(selection)
   banks.update({E:engine,SOURCE_SCNR:scenario,0x4f63b8:enc(E),0x4ecc20:enc(SOURCE_SCNR)})
  gp=[rng.getrandbits(32)for _ in REGS];gp[4]=SP
  flags=0x202|(rng.getrandbits(12)&0xcd5)
  mask=0xcd5 if entry!=0x24b390 or left!=0xffff or right!=0xffff else 0xcc5
  g=Uc(UC_ARCH_X86,UC_MODE_32);pages={STOP}
  for p,b in list(banks.items())+list(ins.items()):pages.update(range(p&~4095,(p+len(b)+4095)&~4095,4096))
  for p in sorted(pages):g.mem_map(p,4096)
  for p,b in list(ins.items())+list(banks.items()):g.mem_write(p,bytes(b))
  for r,v in zip(REGS,gp):g.reg_write(r,v)
  xmm=[list(range(0x3f000000+i*0x100,0x3f000000+i*0x100+4))for i in range(8)]
  for r,v in zip(XMM,xmm):g.reg_write(r,int.from_bytes(struct.pack('<4I',*v),'little'))
  g.reg_write(U.UC_X86_REG_EFLAGS,flags);g.reg_write(U.UC_X86_REG_FPCW,0x27f);g.reg_write(U.UC_X86_REG_FPSW,0);g.reg_write(U.UC_X86_REG_MXCSR,0x1f80)
  g.reg_write(U.UC_X86_REG_FPTAG,0xffc0 if populated else 0xffff)
  fps=[(0x8000000000000000,e)for e in(0x3fff,0xbffe,0x4000)]if populated else[]
  for i,v in enumerate(fps):g.reg_write(getattr(U,'UC_X86_REG_FP'+str(i)),v)
  trace=[];reads=[];writes=[];last=[0]
  def instruction(u,p,n,_):
   if p==STOP:return
   assert p in owned,hex(p)
   trace.append(p);seen.add(p);last[0]=p
  def read(u,a,p,n,v,_):
   if not any(q<=p and p+n<=q+len(b)for q,b in banks.items()):reads.append((p,n))
  g.hook_add(UC_HOOK_CODE,instruction);g.hook_add(UC_HOOK_MEM_READ,read)
  def write(u,a,p,n,v,d):
   for i,x in enumerate(enc(v,n)):writes.append((last[0],p+i,x))
  g.hook_add(UC_HOOK_MEM_WRITE,write)
  g.emu_start(entry,STOP,count=1000);assert g.reg_read(U.UC_X86_REG_EIP)==STOP and not reads
  last_flags=next(code.at(p).mnemonic for p in reversed(trace)if code.at(p).mnemonic in('cmp','xor','test','or','inc','shl'))
  assert last_flags in('cmp','xor','test')
  # CMP defines AF; final XOR/TEST leaves it architecturally undefined.
  mask=0xcd5 if last_flags=='cmp'else 0xcc5
  final={str(p):sha(bytes(g.mem_read(p,len(b))))for p,b in banks.items()if bytes(g.mem_read(p,len(b)))!=bytes(b)}
  x87=[]
  for i in range(len(fps)):
   m,e=g.reg_read(getattr(U,'UC_X86_REG_FP'+str(i)));x87.append(enc(m,8).hex()+enc(e,2).hex())
  expected=dict(boundary=0,gp=[g.reg_read(r)for r in REGS],xmm=[list(struct.unpack('<4I',g.reg_read(r).to_bytes(16,'little')))for r in XMM],
   eflags=g.reg_read(U.UC_X86_REG_EFLAGS)&mask,x87=x87,fs_base=0,x87_status=g.reg_read(U.UC_X86_REG_FPSW),x87_control=g.reg_read(U.UC_X86_REG_FPCW),mxcsr=g.reg_read(U.UC_X86_REG_MXCSR),
   trace_sha256=sha(b''.join(enc(p)for p in trace)),instructions=len(trace),store_sha256=sha(b''.join(struct.pack('<IIB',*row)for row in writes)),store_bytes=len(writes),banks_sha256=final)
  cases.append(dict(name=name,entry=entry,gp=gp,eflags=flags,defined_flags_mask=mask,populated_x87=populated,banks=[dict(address=p,blob=blob(b))for p,b in sorted(banks.items())],expected=expected))
 for index in list(range(32))+[0xffffffff,0x80000000,0x7fffffff]:
  for left in(0xffff,0,7,0x8000):
   for right in(0xffff,0,7,0x8000):add(f'team-{index:x}-{left:x}-{right:x}',0x24b390,index,left,right,populated=bool((index+left+right)&1))
 for kind in(0,1,2,8,9,10,0x7fffffff,0xffffffff):
  for populated in(False,True):add(f'kind-{kind:x}-{populated}',0x249440,kind=kind,populated=populated)
 for flags in(0,1,7,8,9,0xf,0x80,0xff):
  for mask in(0,1,2,0x40,0x80,0x100,0x155,0x1ff,0xffff):
   for selection in(-1,0,7,8):add(f'select-{flags:x}-{mask:x}-{selection}',0x24a6c0,scenario_flags=flags,team_mask=mask,selection=selection,populated=bool((flags+mask+selection)&1))
 assert production<=seen and seen<=owned
 common_expected={k:v for k,v in cases[0]['expected'].items()if all(row['expected'].get(k)==v for row in cases)}
 common_banks=[b for b in cases[0]['banks']if all(b in row['banks']for row in cases)]
 for row in cases:
  row['expected']={k:v for k,v in row['expected'].items()if k not in common_expected}
  row['banks']=[b for b in row['banks']if b not in common_banks]
 out=ROOT/'tests/fixtures/ctf_configuration_original_tu15.json'
 out.write_text(json.dumps(dict(schema='H2_CTF_CONFIGURATION_ORIGINAL_ABI_V1',original_tu15_sha256=NEW_PIN,base_banks=common_banks,base_expected=common_expected,blobs=blobs,cases=cases,
  instruction_patches=[],supplied_gameplay_results=[],source_sites=sorted(production),executed_sites=sorted(seen&production),reused_source_sites=sorted(owned-production),reused_executed_sites=sorted(seen-production),fp_limits='Integer-only owners; all initialized SSE/x87 state preserved exactly including populated3-value x87 stacks. AF excluded only after a final source XOR/TEST where architecturally undefined; compared after final CMP.'),indent=2,sort_keys=True)+'\n')
 print(json.dumps(dict(fixture=str(out),sha256=sha(out.read_bytes()),cases=len(cases),whole_sites=len(production),executed_sites=len(seen&production),reused_sites=len(owned-production),reused_executed_sites=len(seen-production),scratch=str(S))))
if __name__=='__main__':main()
