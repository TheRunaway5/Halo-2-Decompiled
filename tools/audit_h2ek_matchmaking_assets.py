#!/usr/bin/env python3
"""Read permitted H2EK assets and compare offline DVD references; no import.

The pinned authoring parser and generated definitions stay in run_test scratch.
No kit is executed or changed. No new production payload is generated here.
"""
import argparse
import base64
import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess
import sys
from audit_rendering import Cache

PARSER_PIN='7b01076b485cb851277c14defb71c8f88e214de4'
ROOT=Path(__file__).resolve().parents[1]
def sha(v):return hashlib.sha256(v).hexdigest()

def font(path):
 data=path.read_bytes();u16=lambda at:struct.unpack_from('<H',data,at)[0];u32=lambda at:struct.unpack_from('<I',data,at)[0]
 assert u32(0x200)==0xf0000001
 records={}
 for code in range(32,65536):
  index=u32(0x400+code*4)
  if index==0xffffffff:continue
  p=(index+0x4040)*16;advance,size,width,height,x,y,pointer=struct.unpack_from('<HHHHhhI',data,p)
  stream=data[pointer:pointer+size];assert len(stream)==size
  pixels=[];color=0xfff;i=0
  def append(alpha,count):
   assert count<=width*height-len(pixels)
   pixels.extend([(((((alpha<<1)|(alpha&1))&15)<<12)|color)]*count)
  while i<len(stream):
   value=stream[i];i+=1;kind=value>>6
   if kind==3:append((value>>3)&7,1);append(value&7,1)
   elif kind==2:
    append((value>>3)&7,1);low=value&7
    if 1<=low<=3:append(7,5-low)
    if low>=4:append(0,9-low)
   elif kind==0 and not(value&63):
    pixel=int.from_bytes(stream[i:i+2],'big');i+=2;color=pixel&0xfff;pixels.append(pixel)
   else:append(7 if kind==1 else 0,value&63)
  assert len(pixels)<=width*height
  pixels.extend([0]*(width*height-len(pixels)))
  records[code]=(advance,width,height,x,y,sha(struct.pack('<'+'H'*len(pixels),*pixels)))
 return dict(path=str(path),bytes=len(data),sha256=sha(data),metrics=[u16(0x204),u16(0x206),u16(0x208)]),records

def summary(value):
 if isinstance(value,dict):
  if 'encoded'in value and 'length'in value:
   raw=base64.b64decode(value['encoded']);assert len(raw)==value['length']
   return dict(bytes=len(raw),sha256=sha(raw),prefix16=raw[:16].hex())
  return {k:summary(v)for k,v in value.items()}
 if isinstance(value,list):return [summary(v)for v in value]
 return value

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--kit',type=Path,required=True);p.add_argument('--report',type=Path,required=True)
 a=p.parse_args();scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
 interface=scratch/'tag-interface'
 subprocess.run(['git','clone','--quiet','https://github.com/General-101/Halo-Tag-Interface.git',str(interface)],check=True)
 subprocess.run(['git','-C',str(interface),'checkout','--quiet',PARSER_PIN],check=True)
 assert subprocess.check_output(['git','-C',str(interface),'rev-parse','HEAD'],text=True).strip()==PARSER_PIN
 sys.path.insert(0,str(interface/'tag_interface'));import tag_interface as ti
 game,engine,definitions,groups,extensions,_,_=ti.get_version_args('halo2',0)
 definitions=game.generate_defs(definitions,str(scratch/'definitions'),groups,extensions)
 def read(path):return ti.read_file(definitions,str(path.parent),str(path),0,engine_tag=engine)['Data']
 result=dict(schema='H2_H2EK_MATCHMAKING_ASSET_IDENTITY_1',kit=str(a.kit),parser_commit=PARSER_PIN,tool_sha256=sha(Path(__file__).read_bytes()),
   installed_kit_modified=False,pe_executed=False,production_payload_generated=False,sounds=[],fonts=[],emblems=[])
 for name in ('countdown_for_respawn','pickup_health'):
  path=a.kit/'tags/sound/game_sfx/multiplayer'/(name+'.sound')
  result['sounds'].append(dict(name=name,path=str(path),bytes=path.stat().st_size,sha256=sha(path.read_bytes()),tag=summary(read(path))))
 kit_fonts={name:font(a.kit/'h2_fonts'/name)for name in ('conduit-12','conduit-13','conduit-9','handel_gothic-11','handel_gothic-13','handel_gothic-24')}
 for name in ('conduit-12','handel_gothic-11','conduit-13','conduit-16'):
  header,records=font(ROOT/'assets/disc/maps/fonts'/name);comparisons=[]
  for kit_name,(kh,kr)in kit_fonts.items():
   common=records.keys()&kr.keys();equal=[c for c in common if records[c]==kr[c]]
   ascii_equal=[c for c in range(32,127)if c in records and c in kr and records[c]==kr[c]]
   comparisons.append(dict(kit_name=kit_name,kit_header=kh,shared_glyphs=len(common),exact_decoded_pixels_and_metric_glyphs=len(equal),
     exact_ascii_glyphs=len(ascii_equal),all_original_glyphs_exact=len(equal)==len(records),
     missing_original_glyphs=len(records.keys()-kr.keys()),vertical_metrics_equal=kh['metrics']==header['metrics']))
  result['fonts'].append(dict(original=header,glyphs=len(records),comparisons=comparisons))
 dvd=Cache(ROOT/'assets/disc/maps/mainmenu.map')
 try:
  for name,count in [('background_small',32),('foreground_small',64)]:
   path=a.kit/'tags/ui/global_bitmaps/emblems'/(name+'.bitmap');tag=read(path)
   raw=base64.b64decode(tag['processed pixel data']['encoded']);assert len(raw)==tag['processed pixel data']['length']
   original=next(t for t in dvd.tags.values()if t['group']=='bitm'and t['name']=='ui\\global_bitmaps\\emblems\\'+name)
   _,at=dvd.tag(original['datum'],'bitm');pages=dvd.block(at+68,116);assert len(pages)==count
   comparisons=[]
   for i,page in enumerate(pages):
    pointer,length=dvd.u32(page+28),dvd.u32(page+52);payload=bytes(dvd.data[pointer&0x3fffffff:(pointer&0x3fffffff)+length])
    kp=tag['bitmaps'][i]
    comparisons.append(dict(index=i,original_width=dvd.u16(page+4),original_height=dvd.u16(page+6),
      original_format=dvd.u16(page+12),kit_width=kp['width'],kit_height=kp['height'],kit_format=kp['format'],
      kit_mipmap_count=kp['mipmap count'],original_payload_bytes=length,original_payload_sha256=sha(payload),
      exact_original_encoded_bytes_found_in_kit=raw.find(payload)))
   result['emblems'].append(dict(path=str(path),bytes=path.stat().st_size,sha256=sha(path.read_bytes()),
     processed_bytes=len(raw),processed_sha256=sha(raw),pages=comparisons))
 finally:dvd.close()
 encoded=json.dumps(result,indent=2)+'\n';assert len(encoded.encode())<=1024*1024
 a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(encoded)
 print(json.dumps(dict(passed=True,report=str(a.report),sounds=len(result['sounds']),font_roles=len(result['fonts']),emblem_pages=96)))

if __name__=='__main__':main()
