#!/usr/bin/env python3
"""Unchanged retail19CA0 then whole17420 with declared scalar37/animation inputs.

This measures only the material-bank composition, not resident geometry, the
biped camouflage clone, texture/framebuffer producers or rendered Xbox pixels.
"""
import argparse,collections,hashlib,json,random,struct,sys
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
STATE,STACK,STOP=0x2100000,0x22fef00,0x3000000
DEFINITION,PP,IMPL,EXTERNS,FRAGMENTS,LOD,LAYERS,PASSES,ANIMIMPL,MAPS,PARAMS,REFS,OVERLAYS,CURVES,RANGES=[STATE+i for i in range(0x10000,0x100000,0x10000)]
def packed(values):return struct.pack('<'+'I'*len(values),*(v&0xffffffff for v in values))
def bits(value):return struct.unpack('<I',struct.pack('<f',value))[0]
def sha(data):return hashlib.sha256(data).hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__)
 p.add_argument('--fixture',type=Path,default=ROOT/'tests/fixtures/camouflage_material_animation_original.json')
 p.add_argument('--output',type=Path,default=ROOT/'analysis/multiplayer/game-variants/camouflage-material-original-20261011/report.json')
 a=p.parse_args();x=Xbe(ROOT/'assets/disc/default.xbe')
 assert sha(x.data)=='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
 u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x10000,0x600000)
 for section in x.sections:u.mem_write(section.va,x.section_bytes(section))
 u.mem_map(STATE,0x200000);u.mem_map(STOP,0x1000)
 executed={};entries=collections.Counter()
 def trace(g,pc,size,_):
  assert bytes(g.mem_read(pc,size))==x.read(pc,size)
  executed[pc]=size
  if pc in (0x19ca0,0x17420,0x176a0,0x1b580):entries[f'{pc:X}']+=1
 u.hook_add(UC_HOOK_CODE,trace)
 def run(entry,args=(),registers=()):
  u.mem_write(STACK,packed([STOP,*args]));u.reg_write(UC_X86_REG_ESP,STACK)
  u.reg_write(UC_X86_REG_FPCW,0x027f);u.reg_write(UC_X86_REG_MXCSR,0x1f80)
  u.reg_write(UC_X86_REG_FPTAG,0xffff);u.reg_write(UC_X86_REG_FPSW,0)
  for reg,value in registers:u.reg_write(reg,value)
  u.emu_start(entry,STOP,count=200000)
  assert u.reg_read(UC_X86_REG_EIP)==STOP
 def original(case):
  u.mem_write(STATE,bytes(0x1800));u.mem_write(DEFINITION,bytes(0x100))
  u.mem_write(PP,bytes(0x100));u.mem_write(IMPL,bytes(0x140))
  u.mem_write(STATE+12,packed([DEFINITION]));u.mem_write(STATE+32,packed([PP,IMPL]))
  u.mem_write(STATE+0x1424,packed(case['initial']))
  u.mem_write(STATE+0x5c+37*4,packed([case['camouflage']]))
  u.mem_write(STATE+0x300,packed([case['selected'],0,0,0,0,0,0,0]))
  u.mem_write(STATE+0x1520,bytes([case['offset'],0,0,0,0,0,0,0]))
  u.mem_write(EXTERNS,packed(case['externs']));u.mem_write(PP+0x3c,packed([EXTERNS]))
  u.mem_write(FRAGMENTS,struct.pack('<hH',0,8<<9));u.mem_write(PP+0x24,packed([FRAGMENTS]))
  u.mem_write(IMPL+0xf6,struct.pack('<H',int(case['fragmented'])<<9))
  u.mem_write(RANGES,struct.pack('<H',len(case['externs'])<<9))
  u.mem_write(LOD,struct.pack('<IH',1,1<<9));u.mem_write(LAYERS,struct.pack('<H',1<<9))
  u.mem_write(PASSES,struct.pack('<H',1<<9))
  u.mem_write(ANIMIMPL,struct.pack('<5H',0,0,0,len(case['animated'])<<9,0))
  u.mem_write(MAPS,packed(case['animated']))
  u.mem_write(PARAMS,struct.pack('<H',1<<9));u.mem_write(REFS,struct.pack('<hh',0,0))
  u.mem_write(CURVES,packed(case['function'][1:]))
  u.mem_write(OVERLAYS,packed([0,0,case['function'][0],20,CURVES]))
  for field,data in [(0x1c,[1,LOD]),(0x24,[1,LAYERS]),(0x2c,[1,PASSES]),(0x34,[1,ANIMIMPL]),
      (0x3c,[1,OVERLAYS]),(0x44,[1,REFS]),(0x4c,[1,PARAMS]),(0x54,[len(case['animated']),MAPS])]:
   u.mem_write(DEFINITION+field,packed(data))
  seconds=struct.unpack('<f',packed([case['seconds']]))[0]
  u.mem_write(0x4858a0,struct.pack('<d',seconds));u.mem_write(0x485a58,packed([0]))
  run(0x19ca0,[RANGES],[(UC_X86_REG_ESI,STATE)])
  before=list(struct.unpack('<60I',u.mem_read(STATE+0x1424,240)))
  run(0x17420,[STATE,0],[(UC_X86_REG_EAX,0),(UC_X86_REG_ECX,0),(UC_X86_REG_EBX,0)])
  after=list(struct.unpack('<60I',u.mem_read(STATE+0x1424,240)))
  return dict(before_animation=before,after_animation=after)
 rng=random.Random(0x37ca017420);cases=[]
 for fragmented in (False,True):
  for slot in (0,1,6,8):
   for bank in (0,1):
    for matched in (False,True):
     for inverse in (False,True):
      for component in range(5):
       for camouflage in (0,.25,.5,1):
        offset=int(fragmented);selected=int(matched)
        destination=slot<<6|bank<<10|int(fragmented)<<3
        extern=destination|(3+8*int(inverse))<<16|37<<24
        animation=destination|component<<16
        function=[bits(1),0x1001,0xff196fd9,0,0,0]if component==4 else[bits(1),0,bits(.13),bits(.89),0,0]
        case=dict(initial=[rng.getrandbits(32)for _ in range(60)],fragmented=fragmented,selected=selected,offset=offset,
            camouflage=bits(camouflage),externs=[extern],animated=[animation],function=function,seconds=bits(.37))
        case.update(original(case));cases.append(case)
 fixture=dict(schema='original-camouflage-material-animation-v1',boundary=__doc__,cases=cases)
 a.fixture.parent.mkdir(parents=True,exist_ok=True);a.fixture.write_text(json.dumps(fixture,sort_keys=True,separators=(',',':'))+'\n')
 report=dict(schema='original-camouflage-material-animation-report-v1',boundary=__doc__,xbe_sha256=sha(x.data),
   fixture_sha256=sha(a.fixture.read_bytes()),oracle_sha256=sha(Path(__file__).read_bytes()),cases=len(cases),entries=dict(entries),
   unique_original_instructions=len(executed),instruction_sha256=sha(b''.join(x.read(pc,size)for pc,size in sorted(executed.items()))),
   controls=dict(fpcw='027F',mxcsr='1F80'),services_replaced=[],
   limits=['Declared synthetic finite scalar37 and authored animation banks; original19CA0 then whole17420, not whole16B90 caller.',
     'Original retail image only; no title-update image, natural draw construction, GPU or biped clone claim.'])
 a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(report,sort_keys=True,indent=2)+'\n')
 print(json.dumps({k:report[k]for k in('cases','entries','fixture_sha256')},indent=2))
if __name__=='__main__':main()
