#!/usr/bin/env python3
"""Unchanged retail224FD0 PRTM matrix and span differential.

Declared completed value cache, actual authored PRTM tag and resident pushbuffer
are inputs. Stops after original matrix uploads, before color/driver submission.
No calculation, allocation result, GPU admission or expected output is patched.
"""
import argparse,hashlib,json,random,struct,subprocess,sys
from pathlib import Path
from audit_rendering import Cache
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
S,STACK,STOP=0x2100000,0x21fff00,0x3000000
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--native-binary',type=Path,required=True);p.add_argument('--output',type=Path,required=True)
p.add_argument('--require-byte-exact',action='store_true',help='Fail on any matrix or index-span byte difference from the unchanged Xbox executable')
p.add_argument('--cases',type=int,default=128,help='Deterministic original/native submission count (at least six)')
a=p.parse_args()
if a.cases<6:p.error('--cases must be at least six')
x=Xbe(ROOT/'assets/disc/default.xbe');assert hashlib.sha256(x.data).hexdigest()=='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x600000)
for s in x.sections:g.mem_write(s.va,x.section_bytes(s))
g.mem_map(S,0x100000);g.mem_map(STOP,4096)
c=Cache(ROOT/'assets/disc/maps/lockout.map');base=c.magic&~4095
g.mem_map(base,(c.magic+len(c.data)-base+4095)&~4095);g.mem_write(c.magic,bytes(c.data))
t,o=c.tag(0xe95b07d6,'PRTM');g.mem_write(0x407488,struct.pack('<I',S+0x600))
def hook(g,addr,size,user):
 if addr==0x225688:g.emu_stop()
g.hook_add(UC_HOOK_CODE,hook)
rng=random.Random(0x224fd0);inputs=bytearray(struct.pack('<I',a.cases));retail=bytearray()
f32=lambda v:struct.unpack('<f',struct.pack('<f',v))[0]
epsilon_bits=struct.unpack('<I',struct.pack('<f',.0001))[0]
epsilon_neighbors=[struct.unpack('<f',struct.pack('<I',epsilon_bits+step))[0]for step in (-1,0,1)]
direction_boundaries=[(v,0.,0.)for v in epsilon_neighbors]+[
 (0.,-1.,0.),(-1.,0.,0.),(0.,0.,-1.),(1.,1.,1.),(.00005,.00005,.00005)]
for n in range(a.cases):
 values=[f32(rng.random())for _ in range(17)];values[11]=f32((0,.125,.25,.5,.75,1)[n])if n<6 else values[11]
 position=[f32(rng.uniform(-25,25))for _ in range(3)]
 velocity=([0.,0.,0.]if n==0 else [0.,0.,1.]if n<6 else [f32(rng.uniform(-5,5))for _ in range(3)])
 if 6<=n<6+len(direction_boundaries):velocity=[f32(v)for v in direction_boundaries[n-6]]
 scale=f32(rng.uniform(.01,2));model_random=rng.getrandbits(16)
 seed=bytearray(64);struct.pack_into('<H',seed,0x1a,model_random);g.mem_write(S,bytes(seed))
 g.mem_write(S+0x100,struct.pack('<17f4I',*values,0xffffffff,0,0,0));g.mem_write(S+0x200,struct.pack('<6f',*position,*velocity))
 g.mem_write(S+0x300,struct.pack('<4f',1,1,1,1));g.mem_write(S+0x400,struct.pack('<2I',t['address'],2))
 g.mem_write(S+0x600,struct.pack('<2I',S+0x800,S+0x1000));g.mem_write(S+0x800,bytes(256))
 g.mem_write(STACK,struct.pack('<4I',STOP,S,S+0x100,S+0x200)+struct.pack('<If2I',S+0x20c,scale,S+0x300,S+0x400))
 g.reg_write(UC_X86_REG_ESP,STACK);g.reg_write(UC_X86_REG_FPCW,0x027f);g.reg_write(UC_X86_REG_MXCSR,0x1f80)
 try:g.emu_start(0x224fd0,STOP,count=100000)
 except Exception as e:raise RuntimeError(f'Original PRTM fault at {g.reg_read(UC_X86_REG_EIP):08x}, case {n}')from e
 assert g.reg_read(UC_X86_REG_EIP)==0x225688
 raw=bytes(g.mem_read(S+0x800,60));words=struct.unpack('<15I',raw);assert words[0::5]==(0x101ac0,0x101ad0,0x101ae0)
 retail+=b''.join(raw[q+4:q+20]for q in (0,20,40));span=g.reg_read(UC_X86_REG_EBP)
 first,count=struct.unpack('<2H',g.mem_read(span+4,4));retail+=struct.pack('<2I',first,count)
 inputs+=struct.pack('<17f6ffH',*values,*position,*velocity,scale,model_random)
a.output.parent.mkdir(parents=True,exist_ok=True);inp=a.output.with_suffix('.inputs.bin');out=a.output.with_suffix('.native.bin');ret=a.output.with_suffix('.retail.bin')
inp.write_bytes(inputs);ret.write_bytes(retail)
subprocess.run([str(a.native_binary.resolve()),str(c.path.resolve()),str(inp.resolve()),str(out.resolve())],check=True)
native=out.read_bytes();assert len(native)==len(retail)
mismatch=[];maximum=0
for n in range(a.cases):
 a0=struct.unpack_from('<12f2I',retail,n*56);b0=struct.unpack_from('<12f2I',native,n*56)
 assert a0[12:]==b0[12:]
 for k in range(12):
  delta=abs(a0[k]-b0[k]);maximum=max(maximum,delta)
  if delta>1e-6:mismatch.append(dict(case=n,component=k,retail=a0[k],native=b0[k],error=delta))
report=dict(entry='00224fd0',stop='00225688',fixtures=a.cases,original_sha256=hashlib.sha256(x.data).hexdigest(),authored_particle=t,
 matrix_and_span_byte_exact=native==retail,maximum_float_error=maximum,mismatches=mismatch[:20],whole_frame_parity=False)
a.output.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
raise SystemExit(bool(mismatch) or (a.require_byte_exact and native!=retail))
