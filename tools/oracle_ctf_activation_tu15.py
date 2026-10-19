#!/usr/bin/env python3
"""Offline original CPU proof for complete native TU1.5 objective owners.

Only authenticated original code and literal operands are read from the XBE.
MCC markers are authored data. No Xbox asset bank is loaded and no callee,
allocator, dirty owner or callback return is supplied. Unknown owners stop
before their first instruction, preserving the real request and full ABI.
"""
import argparse, hashlib, json, os, random, struct, sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.path[:0]=[str(ROOT/'tools/reference/halo2-decompiled/tools'),str(ROOT/'tools')]
from xbe import Xbe
from oracle_ctf_initialization import SOURCE
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE,UC_HOOK_MEM_READ
import unicorn.x86_const as R

E,G,S,ROWS,OP,OR,OB,PP,PR,PB,LU,W,T,NS,MATG,MULG,RT,COL=range(0x4000000,0x4048000,0x4000)
K=E+0xfc
DEF=0x4048000
STACK,STOP=0x3008000,0x5000000
REGS=(R.UC_X86_REG_EAX,R.UC_X86_REG_ECX,R.UC_X86_REG_EDX,R.UC_X86_REG_EBX,R.UC_X86_REG_ESP,R.UC_X86_REG_EBP,R.UC_X86_REG_ESI,R.UC_X86_REG_EDI)
XMM=tuple(getattr(R,f'UC_X86_REG_XMM{i}')for i in range(8))
def sha(b):return hashlib.sha256(b).hexdigest()
def u32(b,o=0):return struct.unpack_from('<I',b,o)[0]
def encoded(v,n=4):return int(v&((1<<(n*8))-1)).to_bytes(n,'little')

class Banks:
 def __init__(self):self.rows={}
 def add(self,p,b):
  assert not any(p<q+len(c)and q<p+len(b)for q,c in self.rows.items()),hex(p)
  self.rows[p]=bytearray(b)
 def w(self,p,v,n=4):
  for q,b in self.rows.items():
   if q<=p and p+n<=q+len(b):b[p-q:p-q+n]=encoded(v,n);return
  raise RuntimeError(f'Unbound input {p:08x}/{n}')
 def raw(self,p,data):
  for q,b in self.rows.items():
   if q<=p and p+len(data)<=q+len(b):b[p-q:p-q+len(data)]=data;return
  raise RuntimeError(f'Unbound input {p:08x}/{len(data)}')

def base(image, markers=bytes()):
 b=Banks()
 # Exact literal operands, code-authored engine vtable and static registration
 # inputs. These are not a runtime map/tag bank or a fabricated callee result.
 for p,n in((0x466190,51*4),(0x48bfa0,4),(0x4822a0,16),(0x47d174,4),(0x47a2f8,4),
             (0x46a0cc,4),(0x46a0d0,4),(0x46a178,4),(0x46a194,4),
             (0x474d08,4),(0x474d10,4),(0x474c94,4)):
  b.add(p,image.read(p,n))
 for p in(0x474d08,0x474d10,0x474c94):
  q=u32(image.read(p,4))
  if not any(a<=q and q+12<=a+len(z)for a,z in b.rows.items()):b.add(q,image.read(q,12))
 for p,v in((0x4f63b8,E),(0x4f3218,G),(0x4ecc20,S),(0x52d710,K),
             (0x4ecbd0,OP),(0x4f54f4,PP),(0x4f54f0,LU),(0x4dc04c,W),
             (0x51df54,T),(0x4f0414,NS),(0x4ecc1c,MATG)):
  b.add(p,encoded(v))
 b.add(0x51dfa0,bytes(2));b.add(0x56dfa0,bytes(44))
 rr=random.Random(0x249590)
 for p,n in((E,0x1000),(G,0x2000),(S,0x1000),(ROWS,0x1000),(OP,0x100),
             (OR,0x100),(OB,0x1000),(PP,0x100),(PR,0x2000),(PB,4),
             (LU,0x100),(W,0x100),(T,0x100),(NS,0x100),(MATG,0x200),
             (MULG,0x100),(RT,0x100),(COL,9*12),(DEF,0x400)):
  b.add(p,bytes(n))
 b.raw(E,rr.randbytes(0x1000));b.raw(E+0x304,bytes(0x400))
 b.w(E,0);b.w(E+8,0x1ff,2);b.w(E+0xc,3,2)
 for n in range(9):b.w(E+0x10+n*2,n,2)
 b.w(E+0x24,-1);b.w(E+0x28,-1);b.w(E+0x6c,0,2);b.w(E+0x6e,0,2)
 b.w(E+0xc04,0);b.w(E+0xc14,1);b.w(E+0xc18,0)
 for n in range(32):b.w(E+0x7dc+n*4,-1)
 b.w(G+0xc,1,1);b.w(G+0x180,1);b.w(G+0x184,1,1);b.w(G+0x190,300,2);b.w(G+0x240,0)
 b.w(S+0x118,len(markers)//32);b.w(S+0x11c,ROWS);b.raw(ROWS,markers)
 b.w(OP+0x20,8);b.w(OP+0x24,12);b.w(OP+0x38,8);b.w(OP+0x44,OR)
 for i in range(8):
  b.w(OR+i*12,0x101+i,2);b.w(OR+i*12+8,OB+i*0x200)
  b.w(OB+i*0x200+0xd4,-1);b.w(OB+i*0x200+0x154,-1)
  b.w(OB+i*0x200+0x17e,-1,2)
  b.w(OB+i*0x200,0xa0010001);b.w(OB+i*0x200+0x13c,-1)
 b.w(PP+0x20,8);b.w(PP+0x24,0x200);b.w(PP+0x38,0);b.w(PP+0x44,PR);b.w(PP+0x48,PB)
 for i in range(4):b.w(LU+0xc+i*4,-1)
 b.w(W+8,1);b.w(MATG+0x16c,0)
 b.w(NS+8,MULG);b.w(NS+0x18,DEF);b.w(DEF+0x290,1,2)
 b.w(MULG+4,MULG+0x40);b.w(MULG+0xc,RT)
 b.w(MULG+0x50,9);b.w(MULG+0x54,COL)
 b.w(RT+4,0xa0010001);b.w(RT+0x3c,0xa0090002)
 for i in range(9):b.raw(COL+i*12,struct.pack('<3f',i/8,1-i/8,(i%3)/2))
 b.add(0x3007000,rr.randbytes(0x2000));b.w(STACK,STOP)
 return b

def cases(image):
 out=[]
 def add(name,entry,setup=None,regs=None,args=(),markers=bytes(),initialize=False):
  b=base(image,markers)
  if setup:setup(b)
  gp=[0x11335577,0x22446688,0x33557799,0x446688aa,STACK,0x557799bb,0x6688aacc,0x7799bbdd]
  if regs:
   for n,v in regs.items():gp[n]=v&0xffffffff
  for i,a in enumerate(args):b.w(STACK+4+i*4,a)
  out.append(dict(name=name,entry=entry,banks=b,gp=gp,initialize=initialize))
 add('engine-static-registration',0x3825d0)
 for mask in(0,1,0x80,0x55,0x1ff):
  for current in(-1,0,3,7):
   add(f'next-team-{mask:x}-{current}',0x164930,lambda b,m=mask:b.w(E+0xc,m,2),{1:current})
 for team in(-1,0,1,3,7,8,9,32767,65536):
  for mask in(0,3,0x1ff):
   add(f'spawn-team-{mask:x}-{team}',0x15b3b0,lambda b,m=mask:b.w(E+8,m,2),args=(team,))
 for engine in(1,9):
  for kind in(0,1,2):
   for team in(-1,0,1,7,8,9):
    def setup(b,engine=engine,kind=kind):b.w(G+0x180,engine);b.w(G+0x240,kind);b.w(K+0x1f4,1)
    add(f'team-gate-{engine}-{kind}-{team}',0x24cd70,setup,{6:team})
 # Exact authored marker inputs from the previously pinned independent oracle.
 fixture=json.loads((ROOT/'tests/fixtures/ctf_initialization_original.json').read_text())
 for mm in fixture['maps']:
  if not mm['name'].startswith('mcc-'):continue
  markers=bytearray(bytes.fromhex(mm['fill_row_hex'])*mm['count'])
  for row in mm['patches']:markers[row['index']*32:(row['index']+1)*32]=bytes.fromhex(row['row_hex'])
  for engine in(1,9):
   for kind in(0,1,2):
    def setup(b,engine=engine,kind=kind):b.w(G+0x180,engine);b.w(G+0x240,kind)
    add(f"initialize-{mm['name']}-{engine}-{kind}",0x249460,setup,markers=markers)
    add(f"activate-{mm['name']}-{engine}-{kind}",0x249590,setup,markers=markers,initialize=True)
    add(f"composed-first-caller-{mm['name']}-{engine}-{kind}",0x15afb0,setup,markers=markers,initialize=True)
 # Real B9960 request prefix always stops at the untouched B9B70 factory.
 for engine in(1,9):
  for selected in(0,1):
   def setup(b,engine=engine,selected=selected):
    b.w(G+0x180,engine);b.w(E+0xc14,selected);b.raw(ROWS,struct.pack('<3f',12.5,-8.25,3.75))
   add(f'factory-request-{engine}-{selected}',0x24b500,setup,{7:ROWS})
 for team in(0,1,7,8):
  def reset(b):
   for n in range(9):b.w(K+2+n*2,-1,2)
   b.w(K+0x38,-1)
  add(f'reset-no-spawn-{team}',0x24b5c0,reset,args=(team,))
 for engine in(0,1):
  for count in(0,1,7,8):
   for team in(-1,0,7,8):
    def registry(b,engine=engine,count=count):
     b.w(E+0xc14,engine);b.w(E+0xc18,count)
     for i in range(8):b.w(E+0xc1c+i*16,0x1010000);b.w(E+0xc20+i*16,i,2)
    add(f'register-{engine}-{count}-{team}',0x160a90,registry,args=(0x1010000,team))
    add(f'lookup-{engine}-{count}-{team}',0x160e50,registry,{3:team})
 # A real source carrier path is declared, rather than silently forced NONE.
 def carrier(b):b.w(OB+0x154,0x1020001);b.w(K+0x1f4,0)
 add('register-existing-carrier',0x160a90,carrier,args=(0x1010000,0))
 def player_carrier(b):carrier(b);b.w(OB+0x200+0x13c,0x1010000);b.w(K+0x170,0,1)
 add('register-carrier-real-stat-owner-required',0x160a90,player_carrier,args=(0x1010000,0))
 for entry in(0x15afb0,0x15a560,0x15a500,0x15d750,0x160680,0x1f1a60,0x15b840,0x15f5e0,0x160f10):
  regs={0:E+0x304,7:E+0x7dc} if entry in(0x1f1a60,0x15d750)else None
  args=(E+0x304,)if entry==0x1f1a60 else ()
  add(f'first-round-real-caller-{entry:x}',entry,regs=regs,args=args,initialize=entry==0x15afb0)
 for count in(1,7,8):
  for entry in(0x15afb0,0x15a500,0x160680,0x15b840):
   def players(b,count=count):
    b.w(PP+0x38,count);b.w(PB,(1<<count)-1)
    for i in range(count):b.w(PR+i*0x200,0x101+i,2)
   add(f'active-players-{count}-{entry:x}',entry,players,initialize=entry==0x15afb0)
 for retained in(0,1):
  def scores(b,retained=retained):
   b.w(G+0x1128,retained,1)
   for n in range(16):b.w(E+0x304+n*0x1c+4,n+1,2)
   for n in range(8):b.w(E+0x304+0x1c4+n*0x12,n+3,2)
  add(f'round-score-carry-{retained}',0x1f1a60,scores,args=(E+0x304,))
 add('later-round-owner-required',0x15a560,lambda b:b.w(E+0x6e,1,2))
 for phase in(1,3):add(f'phase-change-{phase}',0x15f5e0,regs={0:phase})
 add('registry-detach-real-position-owner-required',0x160da0,regs={6:0x1010000},args=(ROWS,))
 add('match-dirty-real-native-callee',0xb1a10,lambda b:b.w(E+0x24,0x1001000),args=(0x200,))
 add('client-activation-guard',0x249590,lambda b:b.w(G+0xc,4,1))
 add('actual-registry-destroy-owner-required',0x160f10,lambda b:(b.w(E+0xc18,1),b.w(E+0xc1c,0x1010000),b.w(E+0xc20,-1,2)))
 return out

class Original:
 def __init__(self,image,owned):self.image=image;self.owned=owned;self.executed=set()
 def run(self,case):
  g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x800000);g.mem_map(0x4000000,0x50000);g.mem_map(0x3000000,0x10000);g.mem_map(STOP,0x1000)
  for sec in self.image.sections:g.mem_write(sec.va,self.image.section_bytes(sec))
  b=case['banks']
  for p,raw in b.rows.items():g.mem_write(p,bytes(raw))
  for r,v in zip(REGS,case['gp']):g.reg_write(r,v)
  for i,r in enumerate(XMM):g.reg_write(r,int.from_bytes(struct.pack('<4I',*(0x3f000000+i*0x100+j for j in range(4))),'little'))
  g.reg_write(R.UC_X86_REG_EFLAGS,0x202);g.reg_write(R.UC_X86_REG_FPCW,0x27f);g.reg_write(R.UC_X86_REG_FPSW,0);g.reg_write(R.UC_X86_REG_FPTAG,0xffff);g.reg_write(R.UC_X86_REG_MXCSR,0x1f80)
  trace=[];stores=[];reads=set();boundary=[];last=[0]
  def code(u,p,n,_):
   if p==STOP:return
   if p not in self.owned:boundary.append(p);u.emu_stop();return
   if not trace or p!=trace[-1]:trace.append(p)
   self.executed.add(p);last[0]=p
  def write(u,access,p,n,v,_):
   # Byte-ordered records normalize host/native vector write granularity only.
   for i,x in enumerate(encoded(v,n)):stores.append((last[0],p+i,x))
  def read(u,access,p,n,v,_):
   if not any(q<=p and p+n<=q+len(z)for q,z in b.rows.items()):reads.add((p,n,last[0]))
  g.hook_add(UC_HOOK_CODE,code);g.hook_add(UC_HOOK_MEM_WRITE,write);g.hook_add(UC_HOOK_MEM_READ,read)
  # Execute the real original static constructor; a table is not supplied as a
  # callback result. Setup runs through the same native owner in the test.
  g.emu_start(0x3825d0,STOP,count=1000000)
  if case['initialize']:
   for r,v in zip(REGS,case['gp']):g.reg_write(r,v)
   g.emu_start(0x249460,STOP,count=1000000)
  # Test entry ABI is independently declared. Prior source stores remain.
  for r,v in zip(REGS,case['gp']):g.reg_write(r,v)
  g.reg_write(R.UC_X86_REG_EFLAGS,0x202)
  g.emu_start(case['entry'],STOP,count=2000000)
  pc=g.reg_read(R.UC_X86_REG_EIP)
  if not boundary and pc!=STOP:raise RuntimeError(f"{case['name']} source budget exhausted at {pc:x}")
  if reads:raise RuntimeError(f"{case['name']} undeclared source reads: {sorted(reads)[:12]}")
  status=g.reg_read(R.UC_X86_REG_FPSW);tag=g.reg_read(R.UC_X86_REG_FPTAG);x87=[];empty=False
  for i in range(8):
   physical=(((status>>11)&7)+i)&7
   if ((tag>>(physical*2))&3)==3:empty=True;continue
   assert not empty,'Sparse original x87 stack needs a separate owner'
   m,e=g.reg_read(getattr(R,f'UC_X86_REG_FP{physical}'));x87.append(encoded(m,8).hex()+encoded(e,2).hex())
  result=dict(boundary=boundary[-1]if boundary else 0,gp=[g.reg_read(r)for r in REGS],
    xmm=[list(struct.unpack('<4I',g.reg_read(r).to_bytes(16,'little')))for r in XMM],
    x87=x87,fs_base=g.reg_read(R.UC_X86_REG_FS_BASE),
    eflags=g.reg_read(R.UC_X86_REG_EFLAGS)&0x8d5,x87_status=g.reg_read(R.UC_X86_REG_FPSW)&0xfdc0,
    x87_control=g.reg_read(R.UC_X86_REG_FPCW),mxcsr=g.reg_read(R.UC_X86_REG_MXCSR)&0xffffffc0,
    trace_sha256=sha(b''.join(encoded(p)for p in trace)),instructions=len(trace),
    store_sha256=sha(b''.join(struct.pack('<IIB',*s)for s in stores)),store_bytes=len(stores),
    banks_sha256={str(p):sha(bytes(g.mem_read(p,len(z))))for p,z in sorted(b.rows.items())if bytes(g.mem_read(p,len(z)))!=bytes(z)})
  return result

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--directory',type=Path,required=True);p.add_argument('--fixture',type=Path,required=True);a=p.parse_args()
 assert Path(os.environ['HALO2_TEST_SCRATCH']).is_dir()
 path=Path(SOURCE['u5']['path']);assert sha(path.read_bytes())==SOURCE['u5']['pin'];image=Xbe(path)
 own=set();body_sets={}
 for name in('bodies.json','test-support/bodies.json'):
  data=json.loads((a.directory/name).read_text());own.update(p for q in data['body_sites'].values()for p in q)
  body_sets[name]=data['body_sites']
 original=Original(image,own);blobs={};records=[];baseline=None
 default_expected=dict(xmm=[[0x3f000000+i*0x100+j for j in range(4)]for i in range(8)],
  x87=[],fs_base=0,x87_status=0,x87_control=0x27f,mxcsr=0x1f80)
 for case in cases(image):
  try:expected=original.run(case)
  except Exception:
   print('FIRST ORIGINAL FAILURE',case['name'],flush=True);raise
  inputs=[];all_inputs=[]
  for addr,raw in sorted(case['banks'].rows.items()):
   key=sha(raw);blobs[key]=raw.hex();item=dict(address=addr,blob=key);all_inputs.append(item)
   if baseline is None or key!=baseline.get(addr):inputs.append(item)
  if baseline is None:baseline={r['address']:r['blob']for r in all_inputs};base_inputs=all_inputs;inputs=[]
  records.append(dict(name=case['name'],entry=case['entry'],initialize=case['initialize'],gp=case['gp'],banks=inputs,
   expected={k:v for k,v in expected.items()if k not in default_expected or v!=default_expected[k]}))
 fixture=dict(schema=1,original_code_sha256=SOURCE['u5']['pin'],source_code_only=True,
   supplied_service_results=[],base_expected=default_expected,base_banks=base_inputs,blobs=blobs,cases=records)
 a.fixture.write_text(json.dumps(fixture,separators=(',',':'))+'\n')
 report=dict(passed=True,cases=len(records),source_instructions=len(own),observed_instructions=len(original.executed),
  complete_return_cases=sum(not c['expected']['boundary']for c in records),
  unsupplied_boundaries={f'{q:08x}':sum(c['expected']['boundary']==q for c in records)for q in sorted({c['expected']['boundary']for c in records})if q},
  source_sha256=SOURCE['u5']['pin'],oracle_sha256=sha(Path(__file__).read_bytes()),fixture_sha256=sha(a.fixture.read_bytes()),fixture_bytes=a.fixture.stat().st_size,
  per_body_coverage={name:{entry:dict(whole=len(sites),observed=len(set(sites)&original.executed))for entry,sites in bodies.items()}for name,bodies in body_sets.items()},
  supplied_service_results=[],original_instruction_bytes_modified=False,original_map_tag_assets_loaded=False,
  x87_status_compared_mask='fdc0',mxcsr_compared_mask='ffffffc0',x87_oracle_limitation='Unicorn does not reproduce hardware FSQRT exception/C1 bits or SSE sticky exceptions; those bits are excluded, not supplied. TOP and other condition bits, full GP/XMM, FP controls, ordered stores/instructions and every declared memory byte remain compared.',
  scope='Offline original CPU owner/state correspondence only; no native factory birth or mode admission yet.')
 (a.directory/'original-abi-report.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n');print(json.dumps(report),flush=True)
if __name__=='__main__':main()
