#!/usr/bin/env python3
"""Compare actual gameplay GPU glow inputs with unchanged Xbox instructions.

The native SDL observer retains completed marker, camera bank, physical viewport
and submitted triangle vertices. Each original image independently generates its
sprites and projects every quad with those exact inputs. Hardware entry/submission
and push-buffer state services are unavailable boundaries. This compares render
inputs before NV2A quantization, not Xbox framebuffer pixels or original flight.
"""
import argparse,csv,hashlib,json,struct
from pathlib import Path
from audit_rendering import Cache
from weapon_reference_versions import images
from oracle_smg_firing_1_1 import ROOT,_Code,_trace
from research_mp_inventory_switch_u5 import raw_prefix_candidates,norm
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_ECX,UC_X86_REG_FPCW,UC_X86_REG_MXCSR
STATE,STACK,STOP,TABLE=0x2100000,0x21fff00,0x3000000,0x2300000

def records(path):
 raw=path.read_bytes();assert raw[:4] in (b'LVG1',b'LVG2');version=raw[:4];at=4;out=[]
 while at<len(raw):
  tick,view,owner,definition,count=struct.unpack_from('<5I',raw,at);at+=20
  state=raw[at:at+68];at+=68;bank=raw[at:at+192];at+=192
  viewport=struct.unpack_from('<4f',raw,at);at+=16;distance=raw[at:at+4];at+=4
  vertices=raw[at:at+count*40];at+=count*40
  assert len(state)==68 and len(bank)==192 and len(vertices)==count*40 and count%6==0
  materials={};ranges=[]
  if version==b'LVG2':
   material_count=struct.unpack_from('<I',raw,at)[0];at+=4
   for _ in range(material_count):
    bitmap,material=struct.unpack_from('<2I',raw,at);at+=8;assert material not in materials;materials[material]=bitmap
   range_count=struct.unpack_from('<I',raw,at)[0];at+=4
   for _ in range(range_count):
    first,size,material=struct.unpack_from('<3I',raw,at);at+=12;assert material in materials;ranges.append((first,size,materials[material]))
  out.append(dict(tick=tick,view=view,owner=owner,definition=definition,count=count,state=state,
    bank=bank,viewport=viewport,distance=distance,vertices=vertices,materials=materials,ranges=ranges))
 assert at==len(raw) and out
 return out

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--capture',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
 controls=records(a.capture);bank={label:x for label,x,_ in images()};old=bank['retail'];old_code=_Code(old,'.text')
 old_starts={int(r['address'],16) for r in csv.DictReader((ROOT/'analysis/retail/functions.tsv').open(),delimiter='\t')}
 cache=Cache(ROOT/'assets/disc/maps/lockout.map');versions=[]
 for label,inventory in [('retail','analysis/retail/functions.tsv'),('TU1.1','analysis/map-pack-update/functions.tsv'),('TU1.5','analysis/multiplayer/title-update-1.5/decompiled/functions.tsv')]:
  x=bank[label];code=_Code(x,'.text');starts={int(r['address'],16) for r in csv.DictReader((ROOT/inventory).open(),delimiter='\t')};owners=[];absolute={};calls={};entries={}
  for entry in (0x4f010,0x4ff50,0x30830,0x30bf0,0x480a0,0x48b00):
   f=_trace(old_code,entry,old_starts);seq=[old_code.at(at) for at in sorted(f.code)];pos={i.address:n for n,i in enumerate(seq)};expected=[norm(i,old,pos) for i in seq];matches=[]
   for at in raw_prefix_candidates(old_code,old,code,entry):
    trace=_trace(code,at,starts);other=[code.at(pc) for pc in sorted(trace.code)];np={i.address:n for n,i in enumerate(other)}
    if [norm(i,x,np) for i in other]==expected:matches.append((at,other))
   assert len(matches)==1,(label,hex(entry),len(matches));at,other=matches[0];entries[entry]=at
   for before,after in zip(seq,other):
    if before.mnemonic=='call' and before.operands[0].type==2:calls[before.operands[0].imm]=after.operands[0].imm
    for u,v in zip(before.operands,after.operands):
     if u.type==v.type==2 and 0x400000<=u.imm<0x600000:absolute[u.imm]=v.imm
     if u.type==v.type==3 and u.mem.base==u.mem.index==v.mem.base==v.mem.index==0 and u.mem.disp and v.mem.disp:absolute[u.mem.disp]=v.mem.disp
   owners.append(dict(retail_entry=f'{entry:08x}',entry=f'{at:08x}',instruction_count=len(seq),normalized_complete_body_equal=True))
  guest=Uc(UC_ARCH_X86,UC_MODE_32);guest.mem_map(0x10000,0x800000)
  for section in x.sections:guest.mem_write(section.va,x.section_bytes(section))
  guest.mem_map(STATE,0x100000);guest.mem_map(TABLE,0x100000);guest.mem_map(STOP,4096)
  page=cache.magic&~4095;guest.mem_map(page,(cache.magic+len(cache.data)-page+4095)&~4095);guest.mem_write(cache.magic,bytes(cache.data))
  guest.mem_write(absolute[0x4e3b44],struct.pack('<I',TABLE))
  for tag in cache.tags.values():guest.mem_write(TABLE+(tag['datum']&65535)*16+8,struct.pack('<I',tag['address']))
  captured=[];reached={};draw=calls[0x4f7e0];begin=calls[0x16b10]
  services={calls[old_at]:(8 if old_at==0x3fb3e0 else 0) for old_at in (0x1cf50,0x400fa0,0x3fb3e0,0x3fb480)}
  def hook(g,pc,size,_):
   reached.setdefault(pc,bytes(g.mem_read(pc,size)).hex())
   if pc==draw:
    sp=g.reg_read(UC_X86_REG_ESP);words=struct.unpack('<9I',g.mem_read(sp,36))
    captured.append((g.reg_read(UC_X86_REG_ECX),words[3],words[4],words[5],words[8]&255,[bytes(g.mem_read(words[1]+i*24,24)) for i in range(words[2])]))
    g.reg_write(UC_X86_REG_ESP,sp+36);g.reg_write(UC_X86_REG_EIP,words[0])
   elif pc==begin or pc in services:
    sp=g.reg_read(UC_X86_REG_ESP);ret=struct.unpack('<I',g.mem_read(sp,4))[0]
    g.reg_write(UC_X86_REG_ESP,sp+4+services.get(pc,0));g.reg_write(UC_X86_REG_EIP,ret)
  guest.hook_add(UC_HOOK_CODE,hook);compared=0;failures=[]
  for ordinal,c in enumerate(controls):
   marker=struct.unpack('<13f',c['state'][:52]);view=c['bank'][:52];projection=c['bank'][120:184]
   guest.mem_write(absolute[0x4b9da0],c['bank'][92:104])
   guest.mem_write(absolute[0x4b9e14],view);guest.mem_write(absolute[0x4b9ecc],c['bank'][184:192])
   payload=struct.pack('<2I10f',c['owner'],c['definition'],*marker[10:13],*marker[1:4],*marker[7:10],marker[0])+c['state'][52:68]
   assert len(payload)==64;guest.mem_write(STATE,payload);guest.mem_write(STACK,struct.pack('<2I',STOP,STATE))
   guest.reg_write(UC_X86_REG_ESP,STACK);guest.reg_write(UC_X86_REG_FPCW,0x027f);guest.reg_write(UC_X86_REG_MXCSR,0x1f80);captured.clear()
   guest.emu_start(entries[0x4f010],STOP,count=2000000);assert guest.reg_read(UC_X86_REG_EIP)==STOP
   guest.mem_write(absolute[0x48568c],view);guest.mem_write(absolute[0x485704],projection)
   left,top,width,height=c['viewport'];assert all(v==int(v) for v in c['viewport'])
   guest.mem_write(absolute[0x485648],struct.pack('<4h',int(top),int(left),int(top+height),int(left+width)))
   guest.mem_write(absolute[0x48565c],c['distance'])
   expected=bytearray();expected_ranges=[]
   for bitmap,frame,cosine,sine,fuzzy,sprites in captured:
    assert not fuzzy and frame==0
    first=len(expected)//40
    for sprite in sprites:
     px,py,pz,rx,ry,argb=struct.unpack('<5fI',sprite);guest.mem_write(STATE+0x1000,sprite[:12])
     guest.mem_write(absolute[0x407488],struct.pack('<I',STATE+0x3000))
     guest.mem_write(STATE+0x3000,struct.pack('<3I',STATE+0x4000,STATE+0x5000,0))
     guest.mem_write(STACK,struct.pack('<2I2f3I',STOP,STATE+0x1000,max(rx,ry),ry,cosine,sine,argb))
     guest.reg_write(UC_X86_REG_ESP,STACK);guest.reg_write(UC_X86_REG_FPCW,0x027f);guest.reg_write(UC_X86_REG_MXCSR,0x1f80)
     guest.emu_start(entries[0x480a0],STOP,count=100000);assert guest.reg_read(UC_X86_REG_EIP)==STOP
     end=struct.unpack('<I',guest.mem_read(STATE+0x3000,4))[0];commands=bytes(guest.mem_read(STATE+0x4000,end-STATE-0x4000))
     quad=[];at=0;uv=None;color=struct.pack('<4f',((argb>>16)&255)/255,((argb>>8)&255)/255,(argb&255)/255,(argb>>24)/255)
     while at<len(commands):
      opcode=struct.unpack_from('<I',commands,at)[0]
      if opcode==0x417fc:at+=8
      elif opcode==0x4190c:
       word=struct.unpack_from('<I',commands,at+4)[0];uv=struct.pack('<2f',word&65535,word>>16);at+=8
      else:
       assert opcode==0x101a00 and uv is not None;quad.append(commands[at+4:at+20]+color+uv);at+=20
     assert len(quad) in (0,4)
     if quad:
      for index in (0,1,2,0,2,3):expected+=quad[index]
    if len(expected)//40!=first:expected_ranges.append((first,len(expected)//40-first,bitmap))
   if c['materials']:assert c['ranges']==expected_ranges,(label,ordinal,c['ranges'],expected_ranges)
   compared+=len(expected)//40
   if expected!=c['vertices']:
    before=struct.unpack('<'+'I'*(len(expected)//4),expected);after=struct.unpack('<'+'I'*(len(c['vertices'])//4),c['vertices'])
    differences=[dict(word=i,original=f'{u:08x}',native=f'{v:08x}') for i,(u,v) in enumerate(zip(before,after)) if u!=v]
    failures.append(dict(record=ordinal,tick=c['tick'],view=c['view'],original_vertices=len(expected)//40,native_vertices=c['count'],differences=differences[:12]))
  for pc,raw in reached.items():assert x.read(pc,len(bytes.fromhex(raw)))==bytes.fromhex(raw)
  versions.append(dict(label=label,xbe_sha256=hashlib.sha256(x.data).hexdigest(),owners=owners,
    unavailable_hardware_boundaries=[f'{pc:08x}' for pc in sorted({begin,draw,*services})],
    compared_vertices=compared,failures=failures,bit_identical=not failures,
    reached_instruction_bytes={f'{pc:08x}':raw for pc,raw in sorted(reached.items())}))
  print(label,len(controls),'actual GPU records,',compared,'vertices,',len(failures),'differences',flush=True)
 report=dict(scope=__doc__,capture_sha256=hashlib.sha256(a.capture.read_bytes()).hexdigest(),
  tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),cache_sha256=hashlib.sha256(cache.data).hexdigest(),
  actual_gpu_records=len(controls),actual_bitmap_ranges_compared=all(c['materials'] for c in controls),views=sorted({c['view'] for c in controls}),versions=versions,
  instruction_patches=[],framebuffer_parity_verified=False,passed=all(v['bit_identical'] for v in versions))
 a.output.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n');assert report['passed']
if __name__=='__main__':main()
