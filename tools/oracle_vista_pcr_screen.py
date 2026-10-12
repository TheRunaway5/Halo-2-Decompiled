#!/usr/bin/env python3
"""Unchanged supplied Vista PCR row, input and intro-draw consumer oracle.

Widget lookup/lifetime, complete appearance, locale formatting, session action
and base draw are explicit external services. No screenshot/GPU parity claim.
"""
from pathlib import Path
import hashlib,json,struct,itertools
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
from unicorn.x86_const import *
ROOT=Path(__file__).resolve().parents[1];SHA='33119398d3f68ad569c5010b652039105d3ce1b6512c418508e49ce69309f109'
BASE=0x400000;STOP=0x1800000;STACK=0x1900000;OBJ=0x1a00000;ROW=OBJ+0x9000;GRID=ROW+0x1000;CHILD=GRID+0x1000;BUFFER=CHILD+0x3000
OUT=ROOT/'analysis/frontend/vista-session-flow-audit/pcr-screen-original'
def u32(u,a):return struct.unpack('<I',u.mem_read(a,4))[0]
def w32(u,a,v):u.mem_write(a,struct.pack('<I',v&0xffffffff))
def signed(v):return struct.unpack('<i',struct.pack('<I',v&0xffffffff))[0]
def wide(u,a,s):u.mem_write(a,s.encode('utf-16-le')+b'\0\0')
def string(u,a):
 r=[]
 for i in range(1024):
  v=bytes(u.mem_read(a+2*i,2))
  if v==b'\0\0':break
  r.append(v)
 else:raise RuntimeError('unterminated string')
 return b''.join(r).decode('utf-16-le')
def bank(seed):
 p=bytearray((i*37+seed*13)&255 for i in range(0x1100));t=bytearray((i*23+seed*19)&255 for i in range(0x840))
 def ws(b,a,s,n):b[a:a+n*2]=(s.encode('utf-16-le')+b'\0\0').ljust(n*2,b'\0')[:n*2]
 for i in range(16):
  a=i*0x110;ws(p,a,f'Player {i}',32);ws(p,a+0x40,f'Score {i}',16);ws(p,a+0xe0,f'Place {i}',16)
  for k in range(4):struct.pack_into('<I',p,a+0x60+4*k,(seed*17+i*113+k*1009)&0xffffffff)
  vals=[(0,0),(1,2),(0x7fffffff,1),(0x7fffffff,0x7fffffff),(123456789,987654321),(-1,1),(9,-1),(1,0)]
  shots,hits=vals[seed%len(vals)];struct.pack_into('<ii',p,a+0x84,shots,hits)
  struct.pack_into('<II',p,a+0x108,(seed*99991+i*71)&0xffffffff,(-seed*119-i*31)&0xffffffff)
  masks=[0,0x80000000,0xffffff,0x800001,0x124824,1,0x2000,0xaaaaaa]
  struct.pack_into('<I',p,a+0x80,masks[seed%len(masks)])
  a=i*0x84;ws(t,a,f'Team {i}',32);ws(t,a+0x40,f'TScore {i}',16);ws(t,a+0x64,f'TPlace {i}',16);struct.pack_into('<H',t,a+0x62,i%8)
 return bytes(p),bytes(t)
def locale(key,seed):
 if key==0xb000734:return 'TIME'
 if key==0x11000757:return 'PERCENT'
 return f'M{key:08x}'+('abcdefghijklmnopqrstuvwxy'*9 if seed==23 else '')
def fmt(template,args):return template+'|'+','.join(str(signed(a)) for a in args)
def machine(image,cw=0x27f,mx=0x1f80):
 u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(BASE,(len(image)+4095)&~4095);u.mem_write(BASE,image)
 for a,n in ((STOP,0x1000),(STACK,0x20000),(OBJ,0x20000)):u.mem_map(a,n)
 u.reg_write(UC_X86_REG_FPCW,cw);u.reg_write(UC_X86_REG_MXCSR,mx);return u

def execute(image,q):
 u=machine(image,q.get('cw',0x27f),q.get('mx',0x1f80));events=[];visits=0;hosts=0;selecteds=0;colors=[]
 def ret(value=None,pop=0):
  s=u.reg_read(UC_X86_REG_ESP)
  if value is not None:u.reg_write(UC_X86_REG_EAX,value&0xffffffff)
  u.reg_write(UC_X86_REG_EIP,u32(u,s));u.reg_write(UC_X86_REG_ESP,s+4+pop)
 def event(k,*v):events.append([k,*v])
 def hook(_u,pc,size,_):
  nonlocal visits,hosts,selecteds
  visits+=1;s=u.reg_read(UC_X86_REG_ESP);this=u.reg_read(UC_X86_REG_ECX)
  if pc==STOP:u.emu_stop();return
  if pc==0x6875d3:ret();return
  if pc==0x611a7b:event('grid',u32(u,s+4));ret(GRID,4);return
  if pc==0x611973:
   i=u32(u,s+4);event('child',i);ret(CHILD+i*0x100 if q['mask']&(1<<i) else 0,4);return
  if pc==0x6119f7:event('bitmap',u32(u,s+4));ret(CHILD+0x1000 if q['bitmap'] else 0,4);return
  if pc==0x40d0d7:event('appearance',u32(u,s+8));ret(0,8);return
  if pc==0x61383b:
   v=q['selected'][selecteds];selecteds+=1;event('selected',v);ret(v);return
  if pc==0x61baa9:event('text',(this-CHILD)//0x100,string(u,u32(u,s+4)));ret(0,4);return
  if pc==0x611ba1:
   key,dst=u32(u,s+4),u32(u,s+8);event('locale',key);wide(u,dst,locale(key,q['seed']));ret(dst,8);return
  if pc==0x6101a4:ret(this);return
  if pc==0x40c85b:
   dst,template=u32(u,s+4),string(u,u32(u,s+8));args=[u32(u,s+12+i*4) for i in range(2 if template=='TIME' else 1)]
   event('format',template,*args);wide(u,dst,fmt(template,args));ret(dst);return
  if pc==0x5b4db6:
   dst,key=u32(u,s+4),u32(u,s+8);event('team_color',key);u.mem_write(dst,struct.pack('<fff',.125,.5,.875));ret(dst);return
  if pc==0x40d28e:colors.extend([u32(u,s+0x10+i*4) for i in range(6)])
  if pc==0x5ad940:
   v=q['hosts'][hosts];hosts+=1;event('host',v);ret(v);return
  if pc==0x6156b9:event('details',q['details']);ret(q['details']);return
  if pc==0x6152b5:
   dst=u32(u,s+4);event('identity',u32(u,dst),u32(u,dst+4));u.mem_write(dst,struct.pack('<II',0x12345678,0x9abcdef0));ret(q['identity']);return
  if pc==0x60b38e:event('player_details',u32(u,u32(u,s+4)),u32(u,u32(u,s+4)+4));ret();return
  if pc==0x612604:event('close',u32(u,s+4));ret(0,4);return
  if pc==0x5ad9e3:event('continue');ret(0);return
  if pc==0x60e243:event('dialog',*[u32(u,s+4+i*4) for i in range(8)]);ret();return
  if pc==0x5b3cb4:event('elapsed',u32(u,s+4));ret(q['elapsed']);return
  if pc==0x6093ef:event('black',u32(u,s+8));ret();return
  if pc==0x60ebc2:event('draw');ret(0,4);return
 u.hook_add(UC_HOOK_CODE,hook)
 def write(_u,access,address,size,value,_):
  index,offset=divmod(address-CHILD,0x100)
  if offset==0x30 and size==4:
   values=[u32(u,address-8),u32(u,address-4),value&0xffffffff]
   event('bitmap_color' if index==16 else 'text_color',0 if index==16 else index,*values)
  if offset==0x6e and size==1:event('visible',0 if index==16 else index,value)
 u.hook_add(UC_HOOK_MEM_WRITE,write,begin=CHILD,end=CHILD+0x1fff)
 if q['kind']=='row':
  p,t=bank(q['seed']);u.mem_write(0x86eef8,p);u.mem_write(0x86fff8,t);w32(u,0x86eee4,q['variant'])
  entry=[0x40d1df,0x40d32e,0x40d52a,0x40d6b2,0x40d7dc,0x40da4e][q['pane']];args=[ROW,q['row']]
 elif q['kind']=='input':
  u.mem_write(OBJ+0x7f74,bytes([q['lobby']]));w32(u,ROW+4,q['controller']);w32(u,ROW+8,q['code']);entry=0x40c01a;args=[ROW]
 else:
  u.mem_write(OBJ+0x7f74,bytes([q['lobby']]));w32(u,OBJ+0x7f78,q['creation']);entry=0x40bd5f;args=[ROW]
 esp=STACK+0x18000;u.mem_write(esp,struct.pack('<'+'I'*(1+len(args)),STOP,*args));u.reg_write(UC_X86_REG_ESP,esp);u.reg_write(UC_X86_REG_ECX,OBJ)
 u.emu_start(entry,STOP+1,count=300000);assert u.reg_read(UC_X86_REG_EIP)==STOP
 return dict(input=q,events=events,colors=colors,returned=u.reg_read(UC_X86_REG_EAX)&255 if q['kind']=='input' else None,visits=visits)
def main():
 image=(ROOT/'Halo 2 Vista installer/halo2.exe').read_bytes();assert hashlib.sha256(image).hexdigest()==SHA
 rows=[]
 for pane,mask in itertools.product(range(6),(0,1,2,4,8,16,31)):
  for seed in (0,1,2,5,7,23):
   q=dict(kind='row',pane=pane,mask=mask,bitmap=seed%2,row=seed%16,seed=seed,variant=seed%11,selected=[0xffffffff] if seed%2 else [3,11])
   rows.append(execute(image,q))
 for variant in range(11):rows.append(execute(image,dict(kind='row',pane=1,mask=31,bitmap=0,row=15,seed=17,variant=variant,selected=[0xffffffff])))
 for cw,mx,seed in itertools.product((0x27f,0x67f,0xa7f,0xe7f),(0x1f80,0x3f80,0x5f80,0x7f80),range(8)):
  rows.append(execute(image,dict(kind='row',pane=5,mask=31,bitmap=0,row=0,seed=seed,variant=2,selected=[0xffffffff],cw=cw,mx=mx)))
 for lobby,code,hosts,detail,identity in itertools.product((0,1,0x80),(0,1,3,13,14),([0,1],[1,0],[0x80,1]),(0,0x80),(0,1)):
  rows.append(execute(image,dict(kind='input',lobby=lobby,code=code,hosts=hosts,details=detail,identity=identity,controller=33,seed=0)))
 for lobby,elapsed,cw,mx in itertools.product((0,1,0x80),(0,1999,2000,2001,2002,2500,3000,3999,4000,4001,0x7fffffff,0x80000000,0xffffffff),(0x27f,0x67f,0xa7f,0xe7f),(0x1f80,0x3f80,0x5f80,0x7f80)):
  rows.append(execute(image,dict(kind='draw',lobby=lobby,elapsed=elapsed,creation=0xfffffff0,cw=cw,mx=mx,seed=0)))
 OUT.mkdir(parents=True,exist_ok=True);fixture=ROOT/'tests/fixtures/vista_pcr_screen_original.json';fixture.write_text(json.dumps(rows,separators=(',',':'))+'\n')
 report=dict(source_sha256=SHA,cases=len(rows),visits=sum(r['visits'] for r in rows),fixture_sha256=hashlib.sha256(fixture.read_bytes()).hexdigest(),limits=['External widget, appearance, locale formatting and session continuation owners are explicit controlled services.','Team color core475D34/475E44 and medal512-unit bounded append44BCD5 execute unchanged.','This is not a live native report, original font/render output, gameplay fade, or full session integration.'])
 (OUT/'original.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))
if __name__=='__main__':main()
