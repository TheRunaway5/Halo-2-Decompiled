"""Execute unchanged TU1.5 projected implementation/switch owners over declared caller inputs.

30D80 receives the literal15D50 second argument1 and material shad+3E !=0.
34050 switch6 receives the literal16D17/1AFCC shader-LOD layer mask. These
are bounded CPU selection fixtures, not live GPU or complete31150 evidence.
"""
import argparse,hashlib,json,struct,sys
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn import x86_const as R
ROOT=Path(__file__).resolve().parents[2];sys.path.insert(0,str(ROOT/'tools'))
from oracle_smg_firing_1_1 import Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=True)
assert hashlib.sha256(NEW_DEFAULT.read_bytes()).hexdigest()==NEW_PIN
x=Xbe(NEW_DEFAULT);code=_Code(x,'.text');g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x800000)
for section in x.sections:g.mem_write(section.va,x.section_bytes(section))
g.mem_map(0x70000000,0x10000);stack,stop,definition=0x70008000,0x7000ff00,0x70001000
pack=lambda *v:struct.pack('<'+'I'*len(v),*(n&0xffffffff for n in v))
g.mem_write(0x4920e4,pack(definition));visited=set();g.hook_add(UC_HOOK_CODE,lambda _g,at,_s,_d:visited.add(at))
def call(entry,args,registers=()):
 g.mem_write(stack,pack(stop,*args));g.reg_write(R.UC_X86_REG_ESP,stack)
 for register,value in registers:g.reg_write(register,value)
 g.emu_start(entry,stop,count=1000);assert g.reg_read(R.UC_X86_REG_EIP)==stop
 return g.reg_read(R.UC_X86_REG_EAX)
rows=[]
for flags in (0,2,0x20,0x22,0x100a,0x201000):
 g.mem_write(definition,pack(flags))
 for mask in (0,1,2,3,4,5,6,7,0x3f,0xffffffe0):
  g.mem_write(0x4c6834,pack(mask))
  for material in (0,1):
   implementation=call(0x30d80,(material,1))
   for layers in (0,0x1000,0x2000,0x3000,0x13d0a,0xffffffff):
    selected=call(0x34050,(layers,),((R.UC_X86_REG_EAX,6),))
    rows.append((flags,mask,material,layers,implementation,selected))
(a.output/'selection.tsv').write_text(''.join('\t'.join(str(v)for v in row)+'\n'for row in rows))
functions=[]
for entry in (0x30d80,0x34050):
 b=_trace(code,entry,{entry});functions.append(dict(entry=f'{entry:08x}',instructions=len(b.code),visited=len(set(b.code)&visited),instruction_sha256=hashlib.sha256(b''.join(bytes(code.at(pc).bytes)for pc in sorted(b.code))).hexdigest(),tables=[dict(address=f'{at:08x}',width=w,count=n,raw_hex=x.read(at,w*n).hex())for at,w,n in b.tables]))
report=dict(source_sha256=NEW_PIN,cases=len(rows),functions=functions,intercepted_instructions=0,tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),fixture_sha256=hashlib.sha256((a.output/'selection.tsv').read_bytes()).hexdigest(),scope=__doc__)
(a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(dict(cases=len(rows),functions=functions)))
