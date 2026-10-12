#!/usr/bin/env python3
"""Finite unchanged hill polygon calls over genuine captured renderer state.

The original record filler and complete polygon owner receive declared authored
Lockout geometry/color/API inputs. Captured device, shader/bitmap/program banks,
readiness and command capacity remain untouched. Actual request/MMIO/capacity
boundaries stop without supplied outcomes. CPU commands do not prove GPU output.
"""
import hashlib,json,struct,sys
from pathlib import Path
from oracle_camouflage_pass_services import *
from oracle_camouflage_scene_copy import decode_packets
OUT=ROOT/'analysis/multiplayer/game-variants/king-draw-snapshot-source-20261012'
VISUAL=ROOT/'analysis/multiplayer/game-variants/king-visual-consumers-source-20261012/complete/report.json'
class HillTrace(Trace):
 def invoke(self,entry,args,registers=()):
  self.boundary=None;self.put(STACK,packed([STOP,*args]))
  self.g.reg_write(UC_X86_REG_ESP,STACK);self.g.reg_write(UC_X86_REG_EFLAGS,2)
  self.g.reg_write(UC_X86_REG_FPCW,0x027f);self.g.reg_write(UC_X86_REG_MXCSR,0x1f80)
  self.g.reg_write(UC_X86_REG_FPTAG,0xffff);self.g.reg_write(UC_X86_REG_FPSW,0)
  for r,v in registers:self.g.reg_write(r,v&0xffffffff)
  device=self.u(0x407488);begin=self.u(device);limit=self.u(device+4)
  try:self.g.emu_start(entry,STOP,count=2000000)
  except UcError:
   if not self.boundary:raise
  self.finish_writes();complete=not self.boundary and self.g.reg_read(UC_X86_REG_EIP)==STOP
  assert complete or self.boundary
  end=self.u(device);assert begin<=end<=limit
  command=self.read(begin,end-begin)
  return dict(entry=f'{entry:08x}',arguments=args,complete_original_return=complete,stop=self.boundary,
   eax=f'{self.g.reg_read(UC_X86_REG_EAX):08x}',esp=f'{self.g.reg_read(UC_X86_REG_ESP):08x}',
   command_begin=f'{begin:08x}',command_limit=f'{limit:08x}',command_hex=command.hex(),packets=decode_packets(command))
def main():
 import argparse
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,default=OUT/'complete/report.json');a=ap.parse_args()
 snapshot=Snapshot(SNAPSHOT);assert sha(snapshot.ram)==RAM_PIN;image=Xbe(ROOT/'assets/disc/default.xbe');assert sha(image.data)==PIN
 visual=json.loads(VISUAL.read_text());geometry=bytes.fromhex(visual['geometry_hex']);shader=int(next(r for r in visual['cache_inventory']if r['path'].endswith('lockout.map'))['hill_shader']['datum'],16)
 points=SYNTH+0x40000;center=points+0x164;color=points+0x200;record=points+0x300
 radius,perimeter=words(geometry[0x170:0x174])[0],words(geometry[0x17c:0x180])[0]
 cx,cy,cz=struct.unpack_from('<3f',geometry,0x164);r=struct.unpack('<f',geometry[0x170:0x174])[0]
 cases=[]
 for mode in ('captured_camera','declared_center','declared_far'):
  t=HillTrace(snapshot,image,read_rows(FUNCTIONS_CSV));t.put(points,geometry);t.put(color,bytes.fromhex(visual['results']['retail']['constants']['00468c84']['hex']));t.put(record,b'\xa5'*24)
  initial_camera=t.read(0x4b9da0,12)
  if mode!='captured_camera':t.put(0x4b9da0,struct.pack('<3f',cx+(0 if mode=='declared_center'else 10*r),cy,cz))
  fill=t.invoke(0xd4bc0,[shader,0,0,15,0,0xffffffff,record]);fill['record24_hex']=t.read(record,24).hex()
  draw=None
  if fill['complete_original_return'] and int(fill['eax'],16)&255:
   draw=t.invoke(0x19f680,[15,record,points+0x64,32,center,radius,perimeter,color,fw(.8)],
    ((UC_X86_REG_EAX,0),(UC_X86_REG_ECX,0),(UC_X86_REG_EDX,shader)))
  cases.append(dict(mode=mode,camera_before_hex=initial_camera.hex(),camera_input_hex=t.read(0x4b9da0,12).hex(),fill=fill,draw=draw,
   executed=[dict(address=f'{pc:08x}',bytes=image.read(pc,n).hex())for pc,n in sorted(t.executed.items())],pages=len(t.pages),resource_calls=t.calls))
 dependencies=sorted({Path(module.__file__).resolve()for module in sys.modules.values()
  if getattr(module,'__file__',None)and Path(module.__file__).is_file()
  and Path(module.__file__).resolve().is_relative_to(ROOT/'tools')})
 result=dict(schema='king-draw-snapshot-source-v1',scope=__doc__,tool_sha256=sha(Path(__file__).read_bytes()),
  dependencies={str(p.relative_to(ROOT)):sha(p.read_bytes())for p in dependencies},
  snapshot_export_sha256=sha((SNAPSHOT/'export.json').read_bytes()),
  functions_index=str(FUNCTIONS_CSV),functions_index_sha256=sha(Path(FUNCTIONS_CSV).read_bytes()),
  snapshot=str(SNAPSHOT),snapshot_ram_sha256=RAM_PIN,source_xbe_sha256=PIN,visual_report_sha256=sha(VISUAL.read_bytes()),
  geometry_hex=geometry.hex(),shader=f'{shader:08x}',cases=cases,instruction_patches=[],supplied_returns=[])
 a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(result,sort_keys=True,indent=2)+'\n')
 print(json.dumps([dict(mode=c['mode'],fill=c['fill']['complete_original_return'],draw_complete=c['draw']['complete_original_return']if c['draw']else None,stop=c['draw']['stop']if c['draw']else c['fill']['stop'],command_bytes=len(bytes.fromhex(c['draw']['command_hex']))if c['draw']else 0)for c in cases],indent=2))
if __name__=='__main__':main()
