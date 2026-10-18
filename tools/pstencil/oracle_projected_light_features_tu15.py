"""Execute unchanged TU1.5 311A8..31273 projected feature production.

Inputs are the completed original pre-49790 specular RGB, layer10-gated shadow
byte, volume fade and actual definition kind. This is a declared CPU fixture
for the bounded original prefix, not a complete31150 GPU execution.
"""
import argparse,hashlib,json,struct,sys
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn import x86_const as R
ROOT=Path(__file__).resolve().parents[2];sys.path.insert(0,str(ROOT/'tools'))
from oracle_smg_firing_1_1 import Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=True)
assert hashlib.sha256(NEW_DEFAULT.read_bytes()).hexdigest()==NEW_PIN
x=Xbe(NEW_DEFAULT);code=_Code(x,'.text');g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x800000)
for section in x.sections:g.mem_write(section.va,x.section_bytes(section))
g.mem_map(0x70000000,0x10000);definition,shape=0x70001000,0x70002000
pack=lambda *v:struct.pack('<'+'I'*len(v),*(n&0xffffffff for n in v))
bits=lambda v:struct.unpack('<I',struct.pack('<f',v))[0]
epsilon=struct.unpack('<I',x.read(0x46a18c,4))[0];eps=struct.unpack('<f',pack(epsilon))[0]
g.mem_write(0x51df50,pack(0));g.mem_write(definition+4,struct.pack('<H',2));g.mem_write(shape,struct.pack('<H',2));visited=set();g.hook_add(UC_HOOK_CODE,lambda _g,at,_s,_d:visited.add(at))
values=[bits(-1.),bits(0.),epsilon,epsilon+1,bits(1.),0x7fc00001,0x7f800000]
colors=[(v,bits(0.),bits(0.))for v in values]+[(bits(0.),v,bits(0.))for v in values]+[(bits(0.),bits(0.),v)for v in values]+[(values[-2],values[-1],0),(values[-1],values[-2],0)]
rows=[]
for color in colors:
 for shadow in (0,1):
  for volume in (bits(-1.),bits(0.),bits(1.),0x7fc00001):
   for kind in (0,1,2,3):
    g.mem_write(0x492170,pack(*color));g.mem_write(0x492260,pack(volume));g.mem_write(definition+0x14,struct.pack('<H',kind))
    for reg,value in ((R.UC_X86_REG_ESI,definition),(R.UC_X86_REG_EDI,shape),(R.UC_X86_REG_EBX,shadow),(R.UC_X86_REG_ESP,0x70008000),(R.UC_X86_REG_MXCSR,0x1fa0)):g.reg_write(reg,value)
    g.emu_start(0x311a8,0x31273,count=500);assert g.reg_read(R.UC_X86_REG_EIP)==0x31273
    result=struct.unpack('<I',bytes(g.mem_read(0x4c6834,4)))[0];rows.append((*color,epsilon,shadow,volume,kind,result))
(a.output/'features.tsv').write_text(''.join('\t'.join(str(v)for v in row)+'\n'for row in rows))
report=dict(source_sha256=NEW_PIN,cases=len(rows),entry='000311a8',stop='00031273',epsilon_bits=f'{epsilon:08x}',epsilon=eps,visited_instructions=len(visited),instruction_sha256=hashlib.sha256(b''.join(bytes(code.at(pc).bytes)for pc in sorted(visited))).hexdigest(),intercepted_instructions=0,fixture_sha256=hashlib.sha256((a.output/'features.tsv').read_bytes()).hexdigest(),tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),scope=__doc__)
(a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))
