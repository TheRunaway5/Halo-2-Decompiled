#!/usr/bin/env python3
"""Freeze bounded original 219110 menu loop selections; no audible parity claim."""
import hashlib,json,random,struct,sys
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'analysis/frontend/profile-regression'
EXPECTED='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
sys.path.insert(0,str(ROOT/'tools'))
from audit_rendering import Cache
b=(ROOT/'assets/disc/default.xbe').read_bytes();assert hashlib.sha256(b).hexdigest()==EXPECTED
u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x10000,0x600000)
base=struct.unpack_from('<I',b,0x104)[0];n,table=struct.unpack_from('<II',b,0x11c)
for i in range(n):
 _,va,_,off,size=struct.unpack_from('<5I',b,table-base+i*56);u.mem_write(va,b[off:off+size])
STATE,STACK,STOP=0x2000000,0x20fff00,0x3000000
u.mem_map(STATE,0x100000);u.mem_map(STOP,4096)
c=Cache(ROOT/'assets/disc/maps/mainmenu.map')
ugh=next(t for t in c.tags.values()if t['group']=='ugh!');g=c.pointer(ugh['address'],c.magic,0x48)
_,s=c.tag(0xe5290309,'snd!');r=c.block(g+32,12)[c.u16(s+8)]
actual_chances=[c.u16(p+2)for p in c.block(g+40,16)[c.u16(r+8):c.u16(r+8)+c.u16(r+10)]]
assert actual_chances==[0,0,0,0]
u.mem_write(0x51ebd4,struct.pack('<I',STATE));u.mem_write(STATE+0x24,struct.pack('<I',STATE+0x100))
u.mem_write(STATE+0x2c,struct.pack('<I',STATE+0x200))
u.mem_write(STATE+0x500,bytes(20));u.mem_write(STATE+0x508,b'\0\0')
rows=[];stores=[];supplied_time=None
def hook(guest,a,size,_):
 if supplied_time is not None and a in (0x321aae,0x326079):
  sp=guest.reg_read(UC_X86_REG_ESP);ret=struct.unpack('<I',guest.mem_read(sp,4))[0]
  guest.reg_write(UC_X86_REG_EAX,supplied_time if a==0x321aae else STATE+0x1000)
  guest.reg_write(UC_X86_REG_ESP,sp+4);guest.reg_write(UC_X86_REG_EIP,ret)
  return
 if a in (0x219171,0x219239):
  reg=UC_X86_REG_ECX if a==0x219171 else UC_X86_REG_EDX
  p=guest.reg_read(reg);before=struct.unpack('<I',guest.mem_read(p,4))[0]
  val=guest.reg_read(UC_X86_REG_EAX if a==0x219171 else UC_X86_REG_ECX)
  stores.append((a,before,val))
u.hook_add(UC_HOOK_CODE,hook)
def case(ident,chances,used,previous,seed,mirror):
 count=len(chances);u.mem_write(STATE+0x100,bytes(12));u.mem_write(STATE+0x10a,struct.pack('<H',count))
 for i,chance in enumerate(chances):u.mem_write(STATE+0x200+i*16,struct.pack('<HH12x',0,chance))
 u.mem_write(STATE+0x600,struct.pack('<II',used,seed));u.mem_write(STATE+0x608,b'\xa5')
 u.mem_write(STACK,struct.pack('<5I',STOP,previous&0xffffffff,STATE+0x604,STATE+0x608,int(mirror)))
 u.reg_write(UC_X86_REG_EAX,STATE+0x500);u.reg_write(UC_X86_REG_ECX,0);u.reg_write(UC_X86_REG_ESI,STATE+0x600)
 u.reg_write(UC_X86_REG_ESP,STACK);u.reg_write(UC_X86_REG_FPCW,0x027f);u.reg_write(UC_X86_REG_MXCSR,0x1f80);stores.clear()
 u.emu_start(0x219110,STOP,count=1000000);assert u.reg_read(UC_X86_REG_EIP)==STOP
 selected=u.reg_read(UC_X86_REG_EAX)&65535;after_used,after_seed=struct.unpack('<II',u.mem_read(STATE+0x600,8))
 all_used=u.mem_read(STATE+0x608,1)[0]
 rows.append(['C',ident,count,','.join(map(str,chances)),f'{used:08x}',previous,f'{seed:08x}',int(mirror),selected,f'{after_used:08x}',f'{after_seed:08x}',all_used])
 for a,before,after in stores:rows.append(['R',ident,f'{a:08x}',f'{before:08x}',f'{after:08x}'])
 return selected,after_used,after_seed
rng=random.Random(0x219110)
# Carry the original returned bank/previous/seed across 128 authentic four-track boundaries.
used,previous,seed=0,-1,0x146240
for i in range(128):previous,used,seed=case(f'retail-{i}',actual_chances,used,previous,seed,False)
# Real music path starts with 21B940's fixed-pitch draw and one intro selection.
u.mem_write(STATE+0x604,struct.pack('<I',0x146240));u.reg_write(UC_X86_REG_ECX,STATE+0x604)
u.reg_write(UC_X86_REG_ESP,STACK);u.mem_write(STACK,struct.pack('<3I',STOP,0,0))
u.emu_start(0x259d0,STOP,count=1000)
u.mem_write(STOP,b'\xdd\xd8');u.emu_start(STOP,STOP+2,count=1)
pitch_seed=struct.unpack('<I',u.mem_read(STATE+0x604,4))[0]
_,_,seed=case('intro', [0], 0, 0, pitch_seed, False)
used,previous=0,-1
for i in range(128):previous,used,seed=case(f'play-{i}',actual_chances,used,previous,seed,False)
for i in range(1024):
 count=1+i%31;chances=[rng.randrange(65536)for _ in range(count)]
 if i%7==0:chances=[0]*count
 if i%11==0:chances=[65535]*count
 used=rng.getrandbits(32);previous=-1 if i%3==0 else rng.randrange(count)
 case(f'probe-{i}',chances,used,previous,rng.getrandbits(32),bool(i%2))
# Original CRT and 1462B0 integer seed formula. Only platform time and CRT
# thread-storage access are supplied; original rand and GetTickCount execute.
u.mem_write(0x411690,struct.pack('<I',STATE+0x2000))
for i in range(128):
 crt=1 if i==0 else rng.getrandbits(32);supplied_time=rng.getrandbits(32);ticks=rng.getrandbits(32)
 u.mem_write(STATE+0x1014,struct.pack('<I',crt));u.mem_write(STATE+0x2000,struct.pack('<I',ticks))
 u.mem_write(STACK,struct.pack('<I',STOP));u.reg_write(UC_X86_REG_ESP,STACK)
 u.emu_start(0x1462b0,STOP,count=10000)
 after=struct.unpack('<I',u.mem_read(STATE+0x1014,4))[0];draw=(after>>16)&0x7fff
 rows.append(['I',f'{crt:08x}',f'{supplied_time:08x}',f'{ticks:08x}',draw,f'{after:08x}',f'{u.reg_read(UC_X86_REG_EAX):08x}'])
supplied_time=None
# Direct 12A810 fade kernel, retaining every mutated relative/absolute time.
u.mem_write(0x4e637c,struct.pack('<I',STATE+0x7000));u.mem_write(STATE+0x7044,struct.pack('<I',STATE+0x4000))
u.mem_write(0x4e6380,struct.pack('<I',STATE+0x5000))
def fade(ident,start,end,gain,curve,fading,time):
 record=bytearray(0xbc);record[4]=128 if fading else 0
 struct.pack_into('<Hfii',record,0xae,curve,gain,start,end)
 u.mem_write(STATE+0x4000,bytes(record));u.mem_write(STATE+0x5084,struct.pack('<i',time))
 u.mem_write(STACK,struct.pack('<I',STOP));u.reg_write(UC_X86_REG_ESP,STACK);u.reg_write(UC_X86_REG_EAX,0)
 u.reg_write(UC_X86_REG_FPCW,0x027f);u.reg_write(UC_X86_REG_MXCSR,0x1f80)
 u.emu_start(0x12a810,STOP,count=10000)
 actual_start,actual_end=struct.unpack('<ii',u.mem_read(STATE+0x40b4,8))
 rows.append(['F',ident,start,end,struct.pack('<f',gain).hex(),curve,int(fading),time,f'{u.reg_read(UC_X86_REG_EAX):08x}',actual_start,actual_end])
 return actual_start,actual_end
start,end=5000,-1
for i in range(5003):start,end=fade(f'retail-fade-{i}',start,end,0.,1,True,i)
for i in range(1024):
 duration=rng.randrange(10001);start,end=(duration,-1)if i%3==0 else (0,duration)
 time=rng.randrange(-2000,15001)
 fade(f'fade-probe-{i}',start,end,-64.*rng.random(),i%2,bool(i%7),time)
OUT.mkdir(parents=True,exist_ok=True)
p=OUT/'menu-music-original.tsv';p.write_text('\n'.join(' '.join(map(str,row))for row in rows)+'\n')
report=dict(xbe_sha256=EXPECTED,map_sha256=hashlib.sha256(c.data).hexdigest(),entry='00219110',fade_entry='0012a810',seed_entry='001462b0',fpcw='027f',mxcsr='1f80',authored_chances=actual_chances,creation_pitch_seed=f'{pitch_seed:08x}',cases=sum(row[0]=='C'for row in rows),seed_initializations=sum(row[0]=='I'for row in rows),fade_cases=sum(row[0]=='F'for row in rows),rng_stores=sum(row[0]=='R'for row in rows),fixture_sha256=hashlib.sha256(p.read_bytes()).hexdigest(),scope='Original unchanged instructions on supplied range/chances, used mask, previous permutation, seed, seed-initialization platform inputs and fade state/time. Global RNG order, audio-driver update cadence and audible waveform parity are unproven.')
(OUT/'menu-music-original.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
