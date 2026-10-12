#!/usr/bin/env python3
"""Execute unchanged Vista40C8CE/468E88 including the original tie sort.

Input records are discriminating CPU controls, not a captured/native match.
Locale, engine presence and optional live lookup remain declared services.
"""
from pathlib import Path
import hashlib,struct,json
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_INVALID
from unicorn.x86_const import *
ROOT=Path(__file__).resolve().parents[1]
SHA='33119398d3f68ad569c5010b652039105d3ce1b6512c418508e49ce69309f109'
BASE=0x400000;STOP=0x1900000;STACK=0x1910000;DUMMY=0x1a00000
OUT=ROOT/'analysis/frontend/vista-session-flow-audit/pcr-results-original'
def u32(u,a):return struct.unpack('<I',u.mem_read(a,4))[0]
def put(u,a,v):u.mem_write(a,struct.pack('<I',v&0xffffffff))
def text(u,a,s):u.mem_write(a,s.encode('utf-16-le')+b'\0\0')
def source(seed,pmask,tmask,variant,current,finished,teams,ties):
 b=bytearray((i*37+seed*13+(i>>8)*17)&255 for i in range(0xdc78))
 b[0x10:0x12]=bytes([current,finished]);struct.pack_into('<I',b,0x64,variant);b[0x68]=teams
 for p in range(16):
  at=0x370+p*0x94;b[at]=bool(pmask&(1<<p))
  name=f'Source {p}-{seed}'
  if seed==47:name='ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 long'
  raw=name.encode('utf-16-le')+b'\0\0';b[at+12:at+12+64]=raw[:64].ljust(64,b'\0')
  b[at+0x88]=(p%8) if teams else 255
  b[at+0x90]=0 if ties else ((p*5+seed)%7)-1 &255
  score=123 if ties else ((seed*113+p*947+32768)%65536)-32768
  struct.pack_into('<h',b,at+0x92,score)
 for t in range(16):
  at=0xcb0+t*0x18;b[at]=bool(tmask&(1<<t));b[at+1]=0 if ties else (t*3+seed)%9
  struct.pack_into('<h',b,at+2,123 if ties else ((seed*127+t*491+32768)%65536)-32768)
 return bytes(b)
def execute(image,q):
 seed,pmask,tmask,variant,current,finished,teams,ties,engine=q
 u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(BASE,(len(image)+4095)&~4095);u.mem_write(BASE,image)
 for a,n in ((STOP,4096),(STACK,0x20000),(DUMMY,0x3000)):u.mem_map(a,n)
 u.mem_write(0x8b1c80,source(*q[:-1]));u.mem_write(DUMMY+0x48,bytes([teams]));put(u,DUMMY+0x44,variant)
 calls=[];visits=0
 def ret(v=None,pop=0):
  s=u.reg_read(UC_X86_REG_ESP)
  if v is not None:u.reg_write(UC_X86_REG_EAX,v&0xffffffff)
  u.reg_write(UC_X86_REG_EIP,u32(u,s));u.reg_write(UC_X86_REG_ESP,s+4+pop)
 def hook(_u,pc,size,_):
  nonlocal visits
  visits+=1;s=u.reg_read(UC_X86_REG_ESP)
  if pc==STOP:u.emu_stop();return
  if pc==0x6875d3:ret();return
  if pc==0x45b15e:ret(DUMMY if engine else 0);return
  if pc==0x45b171:ret(DUMMY);return
  if pc==0x687ba9:
   dst,val,n=[u32(u,s+4+i*4) for i in range(3)];u.mem_write(dst,bytes([val&255])*n);ret(dst);return
  if pc==0x687e18:
   dst,src,n=[u32(u,s+4+i*4) for i in range(3)];u.mem_write(dst,bytes(u.mem_read(src,n)));ret(dst);return
  if pc in (0x45c341,0x470182,0x46c675):
   value,dst=u32(u,s+4),u32(u,s+8);signed=struct.unpack('<i',struct.pack('<I',value))[0]
   kind={0x45c341:0,0x470182:1,0x46c675:2}[pc]
   word=f'Team {value}' if kind==0 else f'{"score" if kind==1 else "rank"} {signed}'
   text(u,dst,word);calls.append((kind,value));ret();return
  if pc==0x44bd27:
   dst,src,count=[u32(u,s+4+i*4) for i in range(3)];raw=bytes(u.mem_read(src,count*2));i=next((i for i in range(0,len(raw),2) if raw[i:i+2]==b'\0\0'),len(raw)-2)
   u.mem_write(dst,raw[:i]+b'\0\0');ret(dst);return
  if pc in (0x466419,0x4498df):ret(0);return
 def invalid(_u,access,a,n,value,_):raise RuntimeError(f'unmapped{a:x} pc{u.reg_read(UC_X86_REG_EIP):x}')
 u.hook_add(UC_HOOK_CODE,hook);u.hook_add(UC_HOOK_MEM_INVALID,invalid)
 def run(pc,args=()):
  esp=STACK+0x18000;put(u,esp,STOP)
  for i,a in enumerate(args):put(u,esp+4+i*4,a)
  u.reg_write(UC_X86_REG_ESP,esp);u.emu_start(pc,STOP+1,count=2000000)
  assert u.reg_read(UC_X86_REG_EIP)==STOP
 run(0x468e88,(DUMMY+0x100,DUMMY+0x80,DUMMY+0x200,DUMMY+0x84))
 counts=(u32(u,DUMMY+0x80),u32(u,DUMMY+0x84));order=[]
 for at,n in ((DUMMY+0x100,counts[0]),(DUMMY+0x200,counts[1])):order.append(list(struct.unpack('<'+'I'*n,u.mem_read(at,4*n))) if n else [])
 for a in (0x86eee4,0x86eee8,0x86eef0):put(u,a,0x6d6d6d6d)
 u.mem_write(0x86eef8,b'\x6d'*0x1940);run(0x40c8ce)
 raw=bytes(u.mem_read(0x86eef8,0x1100))+bytes(u.mem_read(0x86fff8,0x840))
 return {'q':q,'order':order,'counts':(u32(u,0x86eee8),u32(u,0x86eef0)),'variant':u32(u,0x86eee4),'bytes':raw,'calls':calls,'visits':visits}
def main():
 image=(ROOT/'Halo 2 Vista installer/halo2.exe').read_bytes();assert hashlib.sha256(image).hexdigest()==SHA
 queries=[]
 for n in (0,1,2,7,8,9,15,16):
  for ties in (0,1):
   for teams in (0,1):queries.append((17+n,(1<<n)-1,0xffff,2,1,1,teams,ties,1))
 for variant in range(11):queries.append((47,0x9249,0x849,variant,1,1,variant%2,0,1))
 for current,finished,engine in ((0,1,1),(1,0,1),(0,0,1),(1,1,0)):
  queries.append((38,0xffff,0xffff,2,current,finished,0,0,engine))
 rows=[execute(image,q) for q in queries];OUT.mkdir(parents=True,exist_ok=True)
 fixture=ROOT/'tests/fixtures/vista_pcr_results_original.tsv'
 with fixture.open('w') as f:
  f.write('# H2V_PCR_RESULTS_ORIGINAL_V1\n')
  for i,r in enumerate(rows):
   words=[*r['q'],*r['counts'],r['variant'],len(r['order'][0]),*r['order'][0],len(r['order'][1]),*r['order'][1],len(r['calls'])]
   for k,v in r['calls']:words.extend((k,v))
   f.write(str(i)+' '+','.join(str(w) for w in words)+' '+r['bytes'].hex()+'\n')
 report={'source_sha256':SHA,'cases':len(rows),'instruction_visits':sum(r['visits'] for r in rows),'fixture_sha256':hashlib.sha256(fixture.read_bytes()).hexdigest(),'limits':['Synthetic input banks; locale/engine/live-record/saved-report services are explicit.','Original468E88 comparator and48D12B full sort execute unchanged; no substituted sorting.','This proves PC display-bank construction, not native identity/order/stat publication or a live report.']}
 (OUT/'original.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))
if __name__=='__main__':main()
