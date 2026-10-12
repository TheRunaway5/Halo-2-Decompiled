#!/usr/bin/env python3
"""Execute supplied PC40D0D7/611CD0/620652/640F96 appearance owners unchanged.

Identity queries and widget text/tag lookup are explicit owner services. Real
rank/palette/frame/HSV/descriptor/emblem instructions execute over bound data.
"""
from pathlib import Path
import hashlib,json,itertools,struct
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import *
from capstone import Cs,CS_ARCH_X86,CS_MODE_32
from oracle_vista_pcr_screen import ROOT,SHA,BASE,STOP,STACK,OBJ,machine,u32,w32,wide,string
GROUP=OBJ+0x1000;CHILD=OBJ+0x3000;BUFFER=OBJ+0x8000;GLOBALS=BUFFER;RANKREF=BUFFER+0x2000;RANKTAG=BUFFER+0x2100
TABLE=BUFFER+0x2800;CACHE=BUFFER+0x3000;TAGDATA=BUFFER+0x8000
OUT=ROOT/'analysis/frontend/vista-session-flow-audit/pcr-appearance-original'
WINDOWS={'producer':(0x40d0d7,0x40d1df),'groups':(0x611cd0,0x611d81),'skin':(0x620652,0x620a09),
 'descriptor':(0x6205bd,0x620652),'rank_descriptor':(0x620a09,0x620a3a),'rank_lookup':(0x60c1a9,0x60c226),
 'palette':(0x409a3f,0x409aa6),'team_palette':(0x5b4db6,0x5b4e48),'colors':(0x5b4e48,0x5b4f46),
 'rgb_hsv':(0x475d34,0x475e44),'hsv_rgb':(0x475e44,0x475f78),'frame':(0x61c300,0x61c344),'emblem':(0x640f96,0x640ff8)}
def rgb(v):return list(struct.unpack('<3I',struct.pack('<3f',*v)))
PALETTE=[rgb(v) for v in [(0,0,0),(1,1,1),(.125,.5,.875),(.75,.25,.25),(.25,.75,.25),(.25,.25,.75),(.5,.5,.25),(.5,.25,.5),(.25,.5,.5)]]
TEAMS=[rgb((i/8,(8-i)/8,(i%3)/3)) for i in range(9)]
RANGES=[[i,i] for i in range(49)]+[[49,127]]
def execute(image,q):
 u=machine(image,q.get('cw',0x27f),q.get('mx',0x1f80));events=[];visits=0;descriptor={}
 def ret(value=None,pop=0):
  s=u.reg_read(UC_X86_REG_ESP)
  if value is not None:u.reg_write(UC_X86_REG_EAX,value&0xffffffff)
  u.reg_write(UC_X86_REG_EIP,u32(u,s));u.reg_write(UC_X86_REG_ESP,s+4+pop)
 def hook(_u,pc,size,_):
  nonlocal visits,descriptor
  visits+=1;s=u.reg_read(UC_X86_REG_ESP);this=u.reg_read(UC_X86_REG_ECX)
  if pc==STOP:u.emu_stop();return
  if pc==0x6875d3:ret();return
  if pc in (0x6156cd,0x616d34):
   identity=bytes(u.mem_read(u32(u,s+4),8)).hex();kind='identity' if pc==0x6156cd else 'primary_player'
   value=q['identity'] if pc==0x6156cd else q['primary_player'];events.append([kind,identity,value]);ret(value);return
  if pc==0x615820:events.append(['player',u32(u,s+4),q['player']]);ret(q['player']);return
  if pc==0x60bb1d:ret(RANKREF if q['rank_owner'] else 0);return
  if pc==0x63967a:assert u32(u,s+4)==123;ret(RANKTAG);return
  if pc==0x639640:assert u32(u,s+4)==456;ret(BUFFER+0x1800);return
  if pc==0x61baa9:events.append(['text',string(u,u32(u,s+4))]);ret(0,4);return
  if pc==0x611cd0:
   d=u32(u,s+4);assert u32(u,s+8)==1
   descriptor=dict(flags=u32(u,d),appearance=bytes(u.mem_read(d+0x48,16)).hex(),team=struct.unpack('<H',u.mem_read(d+0x5c,2))[0],no_team=u.mem_read(d+0x5f,1)[0],rank=struct.unpack('<H',u.mem_read(d+0x60,2))[0])
 u.hook_add(UC_HOOK_CODE,hook)
 p=bytearray((i*37+q['seed']*13)&255 for i in range(0x110));p[:64]=bytes(64);p[:len(q['name'].encode('utf-16le'))]=q['name'].encode('utf-16le')
 struct.pack_into('<HBI',p,0x72,q['team'],q['no_team'],0) # rank separately below
 struct.pack_into('<I',p,0x78,q['rank_value']&0xffffffff);p[0xd0:0xe0]=bytes(q['appearance']);p[0x100:0x108]=bytes([1,2,3,4,5,6,7,8])
 u.mem_write(0x86eef8+q['row']*0x110,bytes(p));u.mem_write(OBJ,bytes(0x1000));w32(u,OBJ+0x14,GROUP)
 for i in range(16):
  g=GROUP+i*0x100;u.mem_write(g,b'\xa5'*0x100);w32(u,g+4,10);w32(u,g+0x18,GROUP+(i+1)*0x100 if i<15 else 0);w32(u,g+0x14,CHILD if i==0 else 0)
  w32(u,g+0x74,BUFFER+0x1700);w32(u,BUFFER+0x1708,456)
 # Complete imported PC skin: name, primary, secondary, rank, emblem.
 for i,t in enumerate((6,8,8,8,9)):
  c=CHILD+i*0x100;u.mem_write(c,b'\xa5'*0x100);w32(u,c+4,t);u.mem_write(c+0xa,struct.pack('<H',0 if i in (0,1,4) else i-1));w32(u,c+0x18,c+0x100 if i<4 else 0);w32(u,c+0x70,BUFFER+0x1000+i*0x40)
  w32(u,BUFFER+0x1000+i*0x40, q['emblem_flags'] if i==4 else 0)
  if i in (1,3):
   tag=0 if i==1 else 1;w32(u,BUFFER+0x1000+i*0x40+0x1c,tag if q['primary_bitmap' if i==1 else 'rank_bitmap'] else 0xffffffff)
   w32(u,TABLE+tag*16+8,tag*0x100);w32(u,CACHE+tag*0x100+0x44,q['primary_pages' if i==1 else 'rank_pages']);u.mem_write(c+0x90,struct.pack('<H',q['previous_primary_frame' if i==1 else 'previous_rank_frame']))
 w32(u,BUFFER+0x1820,0x4000);w32(u,BUFFER+0x1828,0x4100)
 w32(u,TAGDATA+0x4000+0x24,0x0b000696)
 for i,key in enumerate((0x14000698,0x0e000699,0x0900069a)):w32(u,TAGDATA+0x4100+i*0x38+0x2c,key)
 w32(u,0x879e74,int(q['globals']));w32(u,0x879e70,GLOBALS);w32(u,GLOBALS+0x160,len(q['palette']));w32(u,GLOBALS+0x164,0x2000)
 u.mem_write(TAGDATA+0x2000,b''.join(struct.pack('<3I',*v) for v in q['palette']))
 w32(u,GLOBALS+0x16c,2 if q['teams'] else 0xffffffff);w32(u,TABLE+2*16+8,0x200);w32(u,CACHE+0x204,0x1000)
 w32(u,TAGDATA+0x1010,len(q['teams']));w32(u,TAGDATA+0x1014,0x1100)
 if q['teams']:u.mem_write(TAGDATA+0x1100,b''.join(struct.pack('<3I',*v) for v in q['teams']))
 w32(u,0x87cd50,TABLE);w32(u,0x87cd54,CACHE);w32(u,0x882290,TAGDATA)
 w32(u,RANKREF+4,123);w32(u,RANKTAG+0x158,len(q['ranges']));w32(u,RANKTAG+0x15c,0x3000)
 u.mem_write(TAGDATA+0x3000,b''.join(struct.pack('<hh',*v) for v in q['ranges']))
 esp=STACK+0x18000;u.mem_write(esp,struct.pack('<III',STOP,OBJ,q['row']));u.reg_write(UC_X86_REG_ESP,esp)
 u.emu_start(0x40d0d7,STOP+1,count=100000);assert u.reg_read(UC_X86_REG_EIP)==STOP
 view=dict(descriptor=descriptor,primary_color=[u32(u,CHILD+0x100+0x28+i*4) for i in range(3)],name_color=[u32(u,CHILD+0x28+i*4) for i in range(3)],
  primary_visible=u.mem_read(CHILD+0x16e,1)[0],secondary_visible=u.mem_read(CHILD+0x26e,1)[0],rank_visible=u.mem_read(CHILD+0x36e,1)[0],emblem_visible=u.mem_read(CHILD+0x46e,1)[0],
  primary_frame=struct.unpack('<H',u.mem_read(CHILD+0x190,2))[0],rank_frame=struct.unpack('<H',u.mem_read(CHILD+0x390,2))[0],groups=[u.mem_read(GROUP+i*0x100+0x6e,1)[0] for i in range(16)])
 return dict(input=q,view=view,events=events,visits=visits)
def query(**kw):
 d=dict(seed=17,row=0,name='Actual Player',team=0xffff,no_team=0,rank_value=-1,appearance=[2,3,4,5]+list(range(4,16)),identity=1,primary_player=3,player=1,globals=True,palette=PALETTE,teams=TEAMS,rank_owner=True,ranges=RANGES,
  primary_bitmap=True,rank_bitmap=True,primary_pages=7,rank_pages=50,previous_primary_frame=6,previous_rank_frame=17,emblem_flags=1)
 d.update(kw);return d
def main():
 image=(ROOT/'Halo 2 Vista installer/halo2.exe').read_bytes();assert hashlib.sha256(image).hexdigest()==SHA;rows=[]
 for identity,primary,team,no_team,rank in itertools.product((0,1),(0xffffffff,3),(0xffff,0,8,9,0x8000),(0,0x80),(-1,0,48,49,127,128)):
  rows.append(execute(image,query(identity=identity,primary_player=primary,team=team,no_team=no_team,rank_value=rank)))
 for globals,first,second,present,flags in itertools.product((False,True),(0,2,8,9,0x80,0xff),(0,4,0x80),(False,True),(1,2)):
  rows.append(execute(image,query(globals=globals,appearance=[first,second,4,5]+list(range(4,16)),rank_owner=present,emblem_flags=flags)))
 for rank,primary_present,rank_present,pages in itertools.product((0,49),(False,True),(False,True),(0,1,49,50,0xffffffff)):
  rows.append(execute(image,query(rank_value=rank,primary_bitmap=primary_present,rank_bitmap=rank_present,primary_pages=pages,rank_pages=pages)))
 for cw,mx,first in itertools.product((0x27f,0x67f,0xa7f,0xe7f),(0x1f80,0x3f80,0x5f80,0x7f80),range(9)):
  rows.append(execute(image,query(cw=cw,mx=mx,appearance=[first,3,4,5]+list(range(4,16)))))
 OUT.mkdir(parents=True,exist_ok=True);fixture=ROOT/'tests/fixtures/vista_pcr_appearance_original.jsonl';fixture.write_text(''.join(json.dumps(r,separators=(',',':'))+'\n' for r in rows))
 m=Cs(CS_ARCH_X86,CS_MODE_32);asm=[];windows={}
 for name,(lo,hi) in WINDOWS.items():
  raw=image[lo-BASE:hi-BASE];windows[name]=dict(begin=hex(lo),end_exclusive=hex(hi),sha256=hashlib.sha256(raw).hexdigest());asm.append(f'; {name}');asm.extend(f'{i.address:08X} {i.mnemonic} {i.op_str}' for i in m.disasm(raw,lo))
 (OUT/'source.asm').write_text('\n'.join(asm)+'\n');report=dict(source_sha256=SHA,cases=len(rows),visits=sum(r['visits'] for r in rows),fixture_sha256=hashlib.sha256(fixture.read_bytes()).hexdigest(),windows=windows,
  boundaries=['Original PC producer,descriptor,rank lookup,appearance/team palette,skin traversal,frame and emblem owners execute unchanged.',
   'Identity queries,rank-reference/tag lookup and text assignment are explicit services with exact ordered operands/results.',
   'Controlled actual asset-shaped palette/rank/bitmap records prove source consumers,not live imported assets or GPU emblem rendering.'])
 (OUT/'execution.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:report[k] for k in ('cases','visits','fixture_sha256')}))
if __name__=='__main__':main()
