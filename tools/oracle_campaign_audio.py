#!/usr/bin/env python3
from pathlib import Path
import hashlib,struct,json,math
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EAX,UC_X86_REG_EIP,UC_X86_REG_XMM0,UC_X86_REG_XMM1,UC_X86_REG_FPCW,UC_X86_REG_MXCSR
root=Path(__file__).resolve().parents[1]
(root/'analysis/campaign').mkdir(parents=True,exist_ok=True)
blob=(root/'assets/disc/default.xbe').read_bytes()
expected='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
assert hashlib.sha256(blob).hexdigest()==expected
u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x10000,0x600000)
base=struct.unpack_from('<I',blob,0x104)[0];count,table=struct.unpack_from('<II',blob,0x11c)
for i in range(count):
 _,addr,_,off,size=struct.unpack_from('<5I',blob,table-base+i*56);u.mem_write(addr,blob[off:off+size])
u.mem_map(0x2000000,0x10000);u.mem_map(0x3000000,0x1000)
u.reg_write(UC_X86_REG_FPCW,0x027f);u.reg_write(UC_X86_REG_MXCSR,0x1f80)
def bits(x):return struct.unpack('<I',struct.pack('<f',x))[0]
def f32(x):return struct.unpack('<f',struct.pack('<f',x))[0]
def call(addr,*args):
 u.reg_write(UC_X86_REG_ESP,0x200f000);u.mem_write(0x200f000,struct.pack('<'+'I'*(1+len(args)),0x3000000,*args));u.emu_start(addr,0x3000000,count=1000000)
 assert u.reg_read(UC_X86_REG_EIP)==0x3000000
conversions=[]
for gain in [-.2,0,.00001,.00063095734448,.001,.01,.1,.5,1,2]:
 gain=f32(gain);u.reg_write(UC_X86_REG_XMM0,bits(gain));call(0x2197f0)
 b=u.reg_read(UC_X86_REG_EAX);db=struct.unpack('<f',struct.pack('<I',b))[0]
 conversions.append(dict(linear_gain=gain,decibels=db,bits=f'{b:08x}'))
linear=[]
u.mem_write(0x3000000,b'\xd9\x1d'+struct.pack('<I',0x2000100))
u.ctl_remove_cache(0x3000000,0x3001000)
for db in [-100,-64,-32,-6.0206,0,5]:
 call(0x2195f0,bits(db))
 # Unicorn caches the previous emu_start stop boundary as an empty block.
 u.ctl_remove_cache(0x3000000,0x3001000)
 u.emu_start(0x3000000,0x3000006,count=1);b=struct.unpack('<I',u.mem_read(0x2000100,4))[0]
 linear.append(dict(decibels=db,linear_gain=struct.unpack('<f',struct.pack('<I',b))[0],bits=f'{b:08x}'))
u.mem_write(0x502118,struct.pack('<I',0x2000200))
def fade(name,db,seconds):
 u.mem_write(0x2000100,name.encode()+b'\0');call(0x221980,0x2000100,bits(db),bits(seconds))
def advance(delta):
 u.reg_write(UC_X86_REG_XMM1,bits(delta));call(0x221900)
def values(index):return list(struct.unpack('<3f',u.mem_read(0x2000200+index*16,12)))
u.mem_write(0x2000200,bytes(54*16));fade('device_machinery',-64,.5)
sequence=[dict(elapsed=0,target_current_remaining=values(28),unselected=values(34))]
elapsed=0
for delta in [.1,.15,.2,.05]:
 advance(delta);elapsed+=delta;sequence.append(dict(elapsed=elapsed,target_current_remaining=values(28),unselected=values(34)))
selection=[]
for name in ['device_machinery','machinery','cinematic','music','','missing']:
 u.mem_write(0x2000200,bytes(54*16));fade(name,-32,0)
 selection.append(dict(substring=name,selected_indices=[i for i in range(54) if values(i)[0]==-32]))
negative=[]
for seconds in [0,-.5]:
 u.mem_write(0x2000200,bytes(54*16));fade('device_machinery',-32,seconds);before=values(28);advance(.001)
 negative.append(dict(requested_seconds=seconds,immediate=before,after_positive_update=values(28)))
u.mem_write(0x2000200,bytes(54*16));fade('device_machinery',-64,.5);advance(.25)
interrupted=[dict(stage='halfway to -64',state=values(28))]
fade('device_machinery',0,.5);interrupted.append(dict(stage='new target before update',state=values(28)))
advance(.25);interrupted.append(dict(stage='halfway back to zero',state=values(28)))
nonpositive_delta=[]
for delta in [0,-.1]:
 before=values(28);advance(delta);nonpositive_delta.append(dict(delta=delta,before=before,after=values(28)))
p=root/'analysis/campaign/audio-research-golden.json'
p.write_text(json.dumps(dict(xbe_sha256=expected,scope='Bounded original numeric/class state fixture, no live sound hardware or whole campaign proof',fpcw='027f',mxcsr='1f80',conversions=conversions,db_to_linear=linear,fade_sequence=sequence,substring_selection=selection,nonpositive_duration=negative,interrupted_fade=interrupted,nonpositive_delta=nonpositive_delta),indent=2)+'\n')
print('Wrote',p)
