#!/usr/bin/env python3
"""Execute original 23E460 and compare native view constants and captured uploads.

Only the final device upload is intercepted. Original FSIN/FCOS, SSE differences,
orientation transpose, shadow-copy branch and dirty-flag reset execute unchanged.
This does not reconstruct the camera producer or live particle scheduler.
"""
import argparse,hashlib,json,random,struct,subprocess,sys
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_FPCW,UC_X86_REG_MXCSR,UC_X86_REG_FPSW,UC_X86_REG_FPTAG
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
PIN='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
STACK,STOP=0x21fff00,0x3000000

def main():
 p=argparse.ArgumentParser(description=__doc__)
 p.add_argument('--native-binary',type=Path,required=True)
 p.add_argument('--output',type=Path,default=ROOT/'analysis/frontend/title-particle-view.json')
 a=p.parse_args();x=Xbe(ROOT/'assets/disc/default.xbe');assert hashlib.sha256(x.data).hexdigest()==PIN
 g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x600000)
 for s in x.sections:g.mem_write(s.va,x.section_bytes(s))
 g.mem_map(0x2100000,0x100000);g.mem_map(STOP,4096);uploads=[]
 def hook(g,address,size,_):
  if address!=0x3f7330:return
  sp=g.reg_read(UC_X86_REG_ESP);ret,count=struct.unpack('<2I',g.mem_read(sp,8))
  assert g.reg_read(UC_X86_REG_ECX)==148 and count==88
  uploads.append(bytes(g.mem_read(g.reg_read(UC_X86_REG_EDX),count*4)))
  g.reg_write(UC_X86_REG_ESP,sp+8);g.reg_write(UC_X86_REG_EIP,ret)
 g.hook_add(UC_HOOK_CODE,hook)
 def run(orientation,dirty=True,shadow=True):
  before=len(uploads);sentinel=bytes([0xcc])*352
  g.mem_write(0x4856c4,orientation);g.mem_write(0x485af0,bytes([dirty]));g.mem_write(0x404fe8,bytes([0 if shadow else 16]));g.mem_write(0x408078,sentinel)
  g.mem_write(STACK,struct.pack('<I',STOP));g.reg_write(UC_X86_REG_ESP,STACK)
  for reg,value in ((UC_X86_REG_FPCW,0x027f),(UC_X86_REG_MXCSR,0x1f80),(UC_X86_REG_FPSW,0),(UC_X86_REG_FPTAG,0xffff)):g.reg_write(reg,value)
  g.emu_start(0x23e460,STOP,count=10000)
  assert g.reg_read(UC_X86_REG_EIP)==STOP and g.reg_read(UC_X86_REG_ESP)==STACK+4
  assert g.mem_read(0x485af0,1)==b'\0' and len(uploads)==before+int(dirty)
  assert bytes(g.mem_read(0x408078,352))==(uploads[-1]if dirty and shadow else sentinel)
  return uploads[-1]if dirty else None
 fixture=ROOT/'analysis/frontend/xemu-capture/particle-graphics-fixture';captured=[];orientations=[];expected=[]
 for event in (5879,6463):
  raw=(fixture/f'{event}-vertex-0.bin').read_bytes();rows=struct.unpack_from('<12f',raw,165*16)
  orientation=struct.pack('<9f',*(rows[(i%3)*4+i//3]for i in range(9)))
  original=run(orientation);actual=raw[148*16:170*16];assert original==actual
  captured.append({'event':event,'original_upload_matches_capture':True,'upload_sha256':hashlib.sha256(actual).hexdigest()})
  orientations.append(orientation);expected.append(original)
 rng=random.Random(0x23e460)
 for n in range(254):
  if n<4:
   bits=(0,0x80000000,1,0x80000001,0x7f7fffff,0xff7fffff,0x7f800000,0xff800000,0x7fc01234)
   orientation=struct.pack('<9I',*(bits[(i+n)%9]for i in range(9)))
  else:orientation=struct.pack('<9f',*(rng.uniform(-8,8)for _ in range(9)))
  original=run(orientation,shadow=n%2==0);assert run(orientation,dirty=False) is None
  orientations.append(orientation);expected.append(original)
 ip=a.output.with_suffix('.inputs.bin');ep=a.output.with_suffix('.original.bin');np=a.output.with_suffix('.native.bin')
 a.output.parent.mkdir(parents=True,exist_ok=True);ip.write_bytes(struct.pack('<I',len(orientations))+b''.join(orientations));ep.write_bytes(b''.join(expected))
 subprocess.run([str(a.native_binary.resolve()),str(ip.resolve()),str(np.resolve())],check=True)
 original=ep.read_bytes();native=np.read_bytes();assert len(native)==len(original)
 report={'xbe_sha256':PIN,'original_entry':'0023e460','intercepted_device_upload':'003f7330','physical_register_range':[148,169],
  'fixtures':len(orientations),'original_bytes':len(original),'different_bytes':sum(x!=y for x,y in zip(native,original)),
  'all_bytes_exact':native==original,'captured_upload_checks':captured,'original_shadow_copy_and_dirty_reset_branches_checked':True,
  'input_sha256':hashlib.sha256(ip.read_bytes()).hexdigest(),'expected_sha256':hashlib.sha256(original).hexdigest(),
  'native_binary_sha256':hashlib.sha256(a.native_binary.read_bytes()).hexdigest(),'live_camera_producer_verified':False,'live_particle_scheduling_verified':False}
 a.output.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2));raise SystemExit(native!=original)
if __name__=='__main__':main()
