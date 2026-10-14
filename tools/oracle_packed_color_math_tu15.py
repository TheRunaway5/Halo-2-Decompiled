#!/usr/bin/env python3
"""Compare unchanged TU1.5 packed-color instructions with the native lowering.

Build tools/physics_packed_color_tu15_native_probe.cpp against the current
halo2_physics_world_runtime library and provide its executable. This verifies
linear packed-color arithmetic and CPU state; it does not establish GPU pixels
or complete physics parity. All original instructions run without callee hooks.
"""
import sys,struct,random,json,subprocess,hashlib,argparse
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parent))
from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--native-driver',type=Path,required=True)
parser.add_argument('--report',type=Path,required=True)
parser.add_argument('--xbe',type=Path,default=NEW_DEFAULT)
args=parser.parse_args();args.native_driver=args.native_driver.resolve(strict=True);args.report=args.report.resolve();args.report.parent.mkdir(parents=True,exist_ok=True)
b=args.report.parent;image=Xbe(args.xbe)
assert hashlib.sha256(args.xbe.read_bytes()).hexdigest()==NEW_PIN
sections=b/'packed-color-original-sections.bin'
with sections.open('wb')as out:
 for section in image.sections:
  raw=image.section_bytes(section);out.write(struct.pack('<II',section.va,len(raw))+raw)
m=Uc(UC_ARCH_X86,UC_MODE_32);m.mem_map(0x10000,0x800000)
for s in image.sections:m.mem_write(s.va,image.section_bytes(s))
m.mem_map(0x60000000,0x110000)
regs=[UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_EBX,UC_X86_REG_ESP,UC_X86_REG_EBP,UC_X86_REG_ESI,UC_X86_REG_EDI]
xs=[UC_X86_REG_XMM0+i for i in range(8)]
def bits(f):return struct.unpack('<I',struct.pack('<f',f))[0]
pairs=[(0,0xffffffff),(0xffffffff,0),(0xff008011,0x60ff0044),(0x7f807f80,0x807f807f)]
fractions=[bits(f)for f in (-1.,-0.,0.,.001,.25,.5,.75,.999,1.,2.)]
cases=[(a,c,f,r)for a,c in pairs for f in fractions for r in range(4)]
rng=random.Random(15)
cases +=[(rng.getrandbits(32),rng.getrandbits(32),bits(rng.uniform(-.1,1.1)),rng.randrange(4))for _ in range(1000)]
source=''.join(f'{a:x} {c:x} {f:x} {r:x}\n'for a,c,f,r in cases)
native=subprocess.run([str(args.native_driver),str(sections)],input=source,capture_output=True,text=True,check=True)
rows=native.stdout.splitlines();assert len(rows)==len(cases)
trace=[];seen=set();m.hook_add(UC_HOOK_CODE,lambda _m,p,_size,_u:trace.append(p))
for idx,((a,c,f,r),row)in enumerate(zip(cases,rows)):
 for reg in regs+xs:m.reg_write(reg,0)
 for p,v in ((0x60020000,1),(0x60020004,0x60020100),(0x60020100,0x2002),(0x60020104,a),(0x60020110,c),(0x600f0000,0x60300000),(0x600f0004,f)):m.mem_write(p,struct.pack('<I',v))
 m.reg_write(UC_X86_REG_ECX,0x60020000);m.reg_write(UC_X86_REG_ESP,0x600f0000)
 m.reg_write(UC_X86_REG_EFLAGS,2);m.reg_write(UC_X86_REG_FPCW,0x27f|(r<<10));m.reg_write(UC_X86_REG_FPSW,0);m.reg_write(UC_X86_REG_FPTAG,0xffff);m.reg_write(UC_X86_REG_MXCSR,0x1f80|(r<<13))
 trace.clear();m.emu_start(0x13e6e0,0x60300000,count=1000);assert m.reg_read(UC_X86_REG_EIP)==0x60300000;seen.update(trace)
 expected=[m.reg_read(reg)for reg in regs]
 for reg in xs:
  v=m.reg_read(reg);expected +=[(v>>(32*k))&0xffffffff for k in range(4)]
 actual=[int(v,16)for v in row.split()]
 assert actual[:40]==expected,(idx,hex(a),hex(c),hex(f),r,actual[:8],expected[:8])
 assert actual[40]&0xcd5 ==m.reg_read(UC_X86_REG_EFLAGS)&0xcd5,(idx,'flags')
 assert actual[41]==0,(idx,'x87 depth')
required=(0x13e793,0x13e7a8,0x13e7b0,0x13e7c5,0x13e7c8,0x13e7cc,0x13e7cf,0x13e7d3,0x13e7da,0x13e7e1,0x13e7e4,0x13e7e7)
assert set(required)<=seen
report=dict(passed=True,cases=len(cases),reference='English retail TU1.5 only',xbe_sha256=NEW_PIN,original_entry='0013e6e0',rounding_modes=4,compared='All GP and SSE registers, defined flags and empty x87 depth',required_mmx_sites=[hex(p)for p in required],native_driver_sha256=hashlib.sha256(args.native_driver.read_bytes()).hexdigest(),limit='Linear authored packed-color profile branch only; original GPU pixels, other profile modes and full physics parity are separate.')
args.report.write_text(json.dumps(report,indent=2)+'\n');print('PASS original TU1.5 packed-color MMX',len(cases),'cases')
