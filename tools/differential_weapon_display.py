#!/usr/bin/env python3
"""Compare magazine digit inputs and bitmap-frame writes against retail FF5F0/17550."""
import argparse,hashlib,json,struct,subprocess
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32
from unicorn.x86_const import UC_X86_REG_EBX,UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_FPCW,UC_X86_REG_MXCSR
from audit_rendering import Cache
ROOT=Path(__file__).resolve().parents[1]
EXPECTED='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
STATE,STACK,STOP=0x2100000,0x21fff00,0x3000000

def main():
 p=argparse.ArgumentParser(description=__doc__)
 p.add_argument('--binary',type=Path,default=ROOT/'build-appearance/weapon_display_tests')
 p.add_argument('--output',type=Path,default=ROOT/'analysis/appearance/weapon-display-differential.json')
 a=p.parse_args();a.output.parent.mkdir(parents=True,exist_ok=True)
 vectors=a.output.with_suffix('.bin')
 subprocess.run([str(a.binary.resolve()),str(ROOT/'assets/disc/maps'),str(vectors)],check=True)
 image=(ROOT/'assets/disc/default.xbe').read_bytes()
 if hashlib.sha256(image).hexdigest()!=EXPECTED:raise ValueError('unsupported retail executable')
 u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x10000,0x600000)
 base=struct.unpack_from('<I',image,0x104)[0];n,t=struct.unpack_from('<II',image,0x11c)
 for i in range(n):
  _,ad,_,off,sz=struct.unpack_from('<5I',image,t-base+i*56);u.mem_write(ad,image[off:off+sz])
 u.mem_map(STATE,0x100000);u.mem_map(STOP,0x1000)
 cache=Cache(ROOT/'assets/disc/maps/01b_spacestation.map')
 mapped=cache.magic&~4095;size=(cache.magic+len(cache.data)-mapped+4095)&~4095
 u.mem_map(mapped,size);u.mem_write(cache.magic,bytes(cache.data))
 def put(at,*values):u.mem_write(at,struct.pack('<'+'I'*len(values),*values))
 put(0x4e0300,STATE);put(STATE+0x44,STATE+0x1000);put(STATE+0x1008,STATE+0x2000)
 put(STATE+0x2000,0);put(0x4e3b44,STATE+0x3000);put(STATE+0x3008,STATE+0x4000)
 put(STATE+0x4000+0x2c0,1)
 def run(entry,args,registers=()):
  put(STACK,STOP,*args);u.reg_write(UC_X86_REG_ESP,STACK)
  u.reg_write(UC_X86_REG_FPCW,0x027f);u.reg_write(UC_X86_REG_MXCSR,0x1f80)
  for r,v in registers:u.reg_write(r,v)
  u.emu_start(entry,STOP,count=100000)
  if u.reg_read(UC_X86_REG_EIP)!=STOP:raise RuntimeError(f'retail {entry:x} did not return')
 # Per-object shader callback supplies the actual outputs of FF5F0 below.
 callback=bytes.fromhex('8b4424083d6b05001774163d6c0500177415')
 callback+=b'\xd9\x05'+struct.pack('<I',STOP+0x208)+b'\xc2\x08\x00'
 callback+=b'\xd9\x05'+struct.pack('<I',STOP+0x200)+b'\xc2\x08\x00'
 callback+=b'\xd9\x05'+struct.pack('<I',STOP+0x204)+b'\xc2\x08\x00'
 # Jumps land at offsets 27 and 36.
 callback=bytearray(callback);callback[10]=16;callback[17]=18
 u.mem_write(STOP+0x100,bytes(callback));put(STOP+0x300,0,STOP+0x100);put(0x485a58,STOP+0x300)
 put(STOP+0x208,0);u.mem_write(0x4858a0,struct.pack('<d',0))
 failures=[];cases=0
 data=vectors.read_bytes()
 if len(data)%20:raise ValueError('malformed native vectors')
 shaders={t['name'].split('\\')[-1]:t for t in cache.tags.values() if t['group']=='shad' and t['name'].endswith(('\\tens','\\ones'))}
 for at in range(0,len(data),20):
  loaded,ones,tens,one_frame,ten_frame=struct.unpack_from('<iffII',data,at)
  u.mem_write(STATE+0x2000+0x22c,struct.pack('<h',loaded))
  for name,native,frame,slot in [('ones',ones,one_frame,0),('tens',tens,ten_frame,1)]:
   sid=0x1700056b+slot
   run(0xff5f0,[0,sid,STATE+0x5000,STATE+0x5004])
   actual=bytes(u.mem_read(STATE+0x5000,4));u.mem_write(STOP+0x200+slot*4,actual)
   if actual!=struct.pack('<f',native):failures.append({'loaded':loaded,'digit':name,'surface':'input','native':native,'retail':struct.unpack('<f',actual)[0]})
   _,shader=cache.tag(shaders[name]['datum'],'shad');d=cache.block(shader+32,124)[0]
   impl=cache.block(d+52,10)[0]
   put(STATE+0x8000+12,cache.magic+d)
   run(0x17550,[cache.magic+impl],[(UC_X86_REG_EBX,STATE+0x8000)])
   actual_frame=struct.unpack('<f',u.mem_read(STATE+0x8000+0x338,4))[0]
   if int(actual_frame)!=frame:failures.append({'loaded':loaded,'digit':name,'surface':'frame','native':frame,'retail':actual_frame})
   cases+=1
 report={'xbe_sha256':EXPECTED,'binary_sha256':hashlib.sha256(a.binary.read_bytes()).hexdigest(),
  'retail_entries':['000FF5F0','00017550'],'cases':cases,'passed':cases-len(failures),'failures':failures[:20],
  'scope':'primary magazine digit input and authored bitmap-frame selection; no framebuffer parity claim'}
 a.output.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
 return bool(failures)
if __name__=='__main__':raise SystemExit(main())
