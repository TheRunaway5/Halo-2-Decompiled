#!/usr/bin/env python3
"""Offline unchanged-TU1.5 scoreboard kerning controls; run through run_test.py."""
from pathlib import Path
import sys,os,json,struct,hashlib,argparse
ROOT=Path(__file__).resolve().parents[1]
from oracle_game_variants import Source,STACK,STOP
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import *
parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--report-output',type=Path,required=True);parser.add_argument('--fixture',type=Path);args=parser.parse_args()
Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
s=Source(dict(path=str(NEW_DEFAULT),pin=NEW_PIN,globals=0x4ecc1c));g=s.g;g.mem_map(0x1100000,0x4000)
FONT,POOL,GLYPHS,TEXT,RECT,COLOR=0x1100000,0x1100400,0x1100800,0x1101000,0x1101800,0x1101900
w=lambda a,v:g.mem_write(a,struct.pack('<I',v&0xffffffff))
h=lambda a,v:g.mem_write(a,struct.pack('<H',v&65535))
header=0x4ef1f0+6*0x1d0;g.mem_write(header,bytes(0x1d0));h(header+4,10);h(header+6,0);h(header+8,0);w(header+0x20,2)
for i,(a,b,k) in enumerate([(65,86,-3),(86,65,2)]):g.mem_write(header+0x24+4*i,struct.pack('<BBh',a,b,k))
w(header+0x1a4+(65>>5)*4,(1<<(65&31))|(1<<(86&31)))
g.mem_write(header+0x1c8,b'\1');w(0x4ef1c4+6*4,6);w(0x55c404,POOL);w(POOL+0x44,GLYPHS)
for index in range(2):
 p=GLYPHS+index*0x38;w(p+0x10,4);g.mem_write(p+0x1c,struct.pack('<8h',10,0,8,8,0,8,0,0));w(p+0x34,0);w(p+0x2c,0)
w(0x4f3c70,6);w(0x4f3c78,0xffffffff);w(0x4f3c74,0);g.mem_write(0x4f3c90,b'\0');g.mem_write(0x4f3c94,bytes(16))
g.mem_write(0x4c2ac0,b'\1');w(0x4c2acc,FONT);g.mem_write(FONT+4,struct.pack('<2h',32,8));g.mem_write(FONT+0xe,b'\0')
g.mem_write(0x4c65f0,struct.pack('<4h',0,0,100,100));g.mem_write(0x4c65f8,struct.pack('<4h',0,0,100,100));g.mem_write(RECT,struct.pack('<4h',0,0,100,100));g.mem_write(COLOR,struct.pack('<4f',1,1,1,1))
quads=[];kerning=[];services=[]
def ret(cleanup):
 sp=g.reg_read(UC_X86_REG_ESP);pc=s.u(sp);g.reg_write(UC_X86_REG_ESP,sp+4+cleanup);g.reg_write(UC_X86_REG_EIP,pc)
def hook(u,a,n,_):
 if a==0x143620:
  c=g.reg_read(UC_X86_REG_ECX);g.reg_write(UC_X86_REG_EAX, [65,86].index(c) if c in [65,86] else 0xffffffff);services.append('declared-font-glyph-lookup');ret(0)
 elif a==0x143370:g.reg_write(UC_X86_REG_EAX,1);services.append('declared-font-pixel-residency');ret(4)
 elif a==0x36390:
  p=s.u(g.reg_read(UC_X86_REG_ESP)+4);raw=bytes(g.mem_read(p,80));quads.append(dict(x=struct.unpack_from('<f',raw,0)[0],y=struct.unpack_from('<f',raw,4)[0],right=struct.unpack_from('<f',raw,40)[0],raw=raw.hex()));services.append('void-glyph-quad-submission');ret(4)
 elif a==0x124f60:kerning.append(dict(previous=g.reg_read(UC_X86_REG_EDI),current=g.reg_read(UC_X86_REG_EBX)))
g.hook_add(UC_HOOK_CODE,hook)
cases=[]
for text in ['AV','AVA','AVAV','VAAV']:
 for alignment in (0,2):
  g.mem_write(TEXT,struct.pack('<'+str(len(text)+1)+'I',*map(ord,text),0));w(0x4f3c7c,alignment)
  s.call(0x141750,registers=[(UC_X86_REG_EAX,COLOR)]);quads.clear();kerning.clear();services.clear();s.executed.clear()
  s.call(0x141bc0,(0x1f470,RECT,0,RECT,0,0x3f800000,TEXT))
  cases.append(dict(text=text,alignment=alignment,quads=list(quads),kerning_calls=list(kerning)))
report=dict(source_sha256=NEW_PIN,entry='141BC0->141A20/1421E0->124F60',font_stimulus=dict(ascent=10,descent=0,leading=0,glyph_advance=10,glyph_width=8,glyph_height=8,glyph_bearing_x=0,glyph_bearing_y=8,pairs=[[65,86,-3],[86,65,2]],atlas_width=32,atlas_height=8),cases=cases,instruction_mutations=False,scope='Actual original scoreboard text geometry with declared asymmetric ready-font cache and void glyph submission. No GPU/full frame or runtime asset claim.')
payload=json.dumps(report,indent=2)+'\n';args.report_output.write_text(payload)
if args.fixture:args.fixture.write_text(payload)
print(json.dumps({'passed':True,'cases':len(cases),'glyph_origins':sum(len(c['quads']) for c in cases),'source_sha256':NEW_PIN}))
