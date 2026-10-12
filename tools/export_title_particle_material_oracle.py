#!/usr/bin/env python3
"""Compare particle material loading with original XBE kernels and captured texels.

Implementation choice is an explicit input, not an inferred live draw choice.
Programs come from original pass records and 17170 remapping; vertex constants
run through 17000; sampler states run through 189A0. Expected texels are the
unchanged original Vulkan capture's RGBA8 resource bytes. No native decoder or
shader implementation produces the expected records.
"""
import argparse, hashlib, json, struct, subprocess, sys
from pathlib import Path
from audit_rendering import Cache
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EDX, UC_X86_REG_ESP, UC_X86_REG_EIP
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
EXPECTED='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
STATE,STACK,STOP=0x2100000,0x21fff00,0x3000000

def main():
 p=argparse.ArgumentParser(description=__doc__)
 p.add_argument('--native-binary',type=Path,required=True)
 p.add_argument('--output',type=Path,default=ROOT/'analysis/frontend/title-particle-material.json')
 a=p.parse_args();image=Xbe(ROOT/'assets/disc/default.xbe')
 assert hashlib.sha256(image.data).hexdigest()==EXPECTED
 g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x600000)
 for s in image.sections:g.mem_write(s.va,image.section_bytes(s))
 g.mem_map(STATE,0x100000);g.mem_map(STOP,4096)
 c=Cache(ROOT/'assets/disc/maps/mainmenu.map');capdir=ROOT/'analysis/frontend/xemu-capture'
 cap=json.loads((capdir/'title-billboard-capture.json').read_text())
 assets=json.loads((ROOT/'analysis/frontend/title-billboard-asset-matches.json').read_text())
 original_textures={int(v[0]['datum'],16):cap['textures'][key]for key,v in assets['matches'].items()}
 def word(at,v):g.mem_write(at,struct.pack('<I',v))
 def run(entry,register):
  g.mem_write(STACK,struct.pack('<II',STOP,STATE+0x6000));g.reg_write(UC_X86_REG_ESP,STACK)
  g.reg_write(register,STATE);g.emu_start(entry,STOP,count=100000)
  assert g.reg_read(UC_X86_REG_EIP)==STOP
 def selected_block(at,stride):
  return c.block(at,stride)
 inputs=bytearray(struct.pack('<I',6));original=bytearray();rows=[]
 for datum in (0xe21a00a5,0xe21f00aa,0xe22100ac):
  tag,r=c.tag(datum,'prt3');pp=c.block(r+0x8c,124)[0]
  _,stem=c.tag(c.u32(pp),'stem');tp=c.block(stem+88,40)[0]
  layers=c.block(tp+8,2);passes=c.block(tp+16,10)
  lr=c.u16(c.block(tp,10)[0]);layer=layers[(lr&511)+15];pr=c.u16(layer)
  entry=passes[pr&511];_,sp=c.tag(c.u32(entry+4),'spas');spp=c.block(sp+28,88)[0]
  im=c.block(spp,306);ti=c.block(tp+24,6);maps=c.block(tp+32,4)
  source_pc=b''.join(bytes(c.data[x:x+4])for x in c.block(pp+12,4))
  source_vc=b''.join(bytes(c.data[x:x+16])for x in c.block(pp+20,16))
  bindings=c.block(spp+8,4);states=c.block(spp+16,5);samplers=c.block(spp+24,24)
  for implementation in (0,1):
   i=im[implementation];mapping=ti[(c.u16(entry+8)&511)+implementation]
   assert c.u16(i+246)>>9==0 and c.u16(i+0x122)>>9==0 and c.u16(i+0x124)>>9==0
   g.mem_write(STATE,bytes(0x1800));word(STATE+0xc,STATE+0x2000);word(STATE+0x10,STATE+0x3000)
   word(STATE+0x20,STATE+0x8000);word(STATE+0x24,STATE+0x9000)
   g.mem_write(STATE+0x9000,bytes(306))
   word(STATE+0x3024,STATE+0x5000)
   word(STATE+0x2010,STATE+0x4000);word(STATE+0x2018,STATE+0x7000)
   g.mem_write(STATE+0x1424,bytes(c.data[i+6:i+246]))
   if source_pc:g.mem_write(STATE+0x4000,source_pc)
   if source_vc:g.mem_write(STATE+0x7000,source_vc)
   g.mem_write(STATE+0x5000,b''.join(bytes(c.data[x:x+4])for x in maps))
   g.mem_write(STATE+0x6000,struct.pack('<H',c.u16(mapping+2)));run(0x17170,UC_X86_REG_EAX)
   program=bytes(g.mem_read(STATE+0x1424,240))
   g.mem_write(STATE+0x6000,struct.pack('<H',c.u16(mapping+4)));run(0x17000,UC_X86_REG_EAX)
   constants=bytes(g.mem_read(STATE+0x1530,512));record=bytearray(program+constants)
   sr=c.u16(i+2);record+=struct.pack('<I',sr>>9)
   record+=b''.join(bytes(c.data[x:x+5])for x in states[sr&511:(sr&511)+(sr>>9)])
   tsr=c.u16(i+4);record+=struct.pack('<I',tsr>>9)
   for n in range(tsr>>9):
    g.mem_write(STATE,bytes(0x1800));word(STATE+0x20,STATE+0x1000);word(STATE+0x101c,STATE+0x2000)
    word(0x407488,STATE+0x3000);word(STATE+0x3004,STATE+0xf0000);word(STATE+0x3000,STATE+0x4000)
    g.mem_write(STATE+0x2000,bytes(c.data[samplers[(tsr&511)+n]:samplers[(tsr&511)+n]+24]))
    g.mem_write(STATE+0x6000,struct.pack('<I',512));run(0x189a0,UC_X86_REG_EDX)
    record+=bytes(g.mem_read(0x404dd0,48))+bytes(g.mem_read(0x404e44,8))
   br=c.u16(mapping);bitmap_remaps=[struct.unpack_from('<HBB',c.data,x)for x in maps[br&511:(br&511)+(br>>9)]]
   texture_stage={};tr=c.u16(i)
   for x in bindings[tr&511:(tr&511)+(tr>>9)]:
    param,external,stage,flags=struct.unpack_from('<4B',c.data,x);assert external==flags==0
    remap=next(v for v in bitmap_remaps if v[0]&255==param)
    bitmap=c.block(pp+4,12)[remap[2]];texture_stage[stage]=bitmap
   tex_rows=[]
   for stage in range(4):
    record+=struct.pack('<I',stage in texture_stage)
    if stage not in texture_stage:continue
    bitmap=texture_stage[stage];bitmap_datum,index=c.u32(bitmap),c.u32(bitmap+4);assert index==0
    texture=original_textures[bitmap_datum];desc=texture['description'];levels=texture['levels']
    record+=bytes(c.data[bitmap:bitmap+12])+struct.pack('<3I',desc['width'],desc['height'],len(levels))
    for mip,level in enumerate(levels):
     data=(capdir/level['file']).read_bytes();assert hashlib.sha256(data).hexdigest()==level['sha256']
     w,h=max(1,desc['width']>>mip),max(1,desc['height']>>mip);assert len(data)==w*h*4
     record+=struct.pack('<3I',w,h,len(data))+data
    tex_rows.append({'stage':stage,'bitmap':f'{bitmap_datum:08x}','mip_sha256':[v['sha256']for v in levels]})
   inputs+=struct.pack('<4I',datum,15,0,implementation);original+=struct.pack('<I',len(record))+record
   rows.append({'particle':f'{datum:08x}','implementation':implementation,'pixel_program_sha256':hashlib.sha256(program).hexdigest(),
     'vertex_constants_sha256':hashlib.sha256(constants).hexdigest(),'record_bytes':len(record),'textures':tex_rows})
 a.output.parent.mkdir(parents=True,exist_ok=True);ip=a.output.with_suffix('.inputs.bin');ep=a.output.with_suffix('.original.bin');np=a.output.with_suffix('.native.bin')
 ip.write_bytes(inputs);ep.write_bytes(original)
 subprocess.run([str(a.native_binary.resolve()),str(c.path.resolve()),str(ip),str(np)],check=True)
 native=np.read_bytes();equal=native==original
 report={'scope':'explicit particle implementations: material constants/states/samplers and every bound original captured mip texel',
  'xbe_sha256':EXPECTED,'capture_sha256':hashlib.sha256((capdir/cap['capture']).read_bytes()).hexdigest(),
  'original_functions':['00017000','00017170','000189a0'],'cases':rows,'all_bytes_exact':equal,'original_bytes':len(original),
  'different_bytes':sum(x!=y for x,y in zip(original,native))+abs(len(original)-len(native)),
  'native_binary_sha256':hashlib.sha256(a.native_binary.read_bytes()).hexdigest(),
  'input_sha256':hashlib.sha256(inputs).hexdigest(),'expected_sha256':hashlib.sha256(original).hexdigest(),
  'live_pixel_implementation_selection_verified':False,'particle_fragment_shader_verified':False,
  'native_particle_rasterization_verified':False,'whole_frame_parity_verified':False}
 a.output.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:v for k,v in report.items()if k!='cases'},indent=2))
 if not equal:
  mismatch=next((i for i,(x,y)in enumerate(zip(original,native))if x!=y),None);print('first mismatch',mismatch)
 raise SystemExit(not equal)
if __name__=='__main__':main()
